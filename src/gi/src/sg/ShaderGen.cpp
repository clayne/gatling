//
// Copyright (C) 2019-2022 Pablo Delgado Krämer
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include "ShaderGen.h"

#include "MtlxMdlCodeGen.h"
#include "MdlRuntime.h"
#include "MdlMaterialCompiler.h"
#include "MdlGlslCodeGen.h"
#include "GlslangShaderCompiler.h"
#include "GlslSourceStitcher.h"

#include <string>
#include <sstream>
#include <limits>
#include <iomanip>
#include <fstream>
#include <cassert>

namespace gi::sg
{
  struct Material
  {
    mi::base::Handle<mi::neuraylib::ICompiled_material> compiledMaterial;
    bool isEmissive;
    bool isOpaque;
  };

  bool ShaderGen::init(const InitParams& params)
  {
    m_shaderPath = fs::path(params.shaderPath);

    m_mdlRuntime = new sg::MdlRuntime();
    if (!m_mdlRuntime->init(params.resourcePath.data()))
    {
      return false;
    }

    m_mdlGlslCodeGen = new sg::MdlGlslCodeGen();
    if (!m_mdlGlslCodeGen->init(*m_mdlRuntime))
    {
      return false;
    }

    m_mdlMaterialCompiler = new sg::MdlMaterialCompiler(*m_mdlRuntime, params.mdlLibPath.data());

    m_mtlxMdlCodeGen = new sg::MtlxMdlCodeGen(params.mtlxLibPath.data());

    if (!sg::GlslangShaderCompiler::init())
    {
      return false;
    }
    m_shaderCompiler = new sg::GlslangShaderCompiler(m_shaderPath);

    return true;
  }

  ShaderGen::~ShaderGen()
  {
    sg::GlslangShaderCompiler::deinit();
    delete m_mtlxMdlCodeGen;
    delete m_shaderCompiler;
    delete m_mdlMaterialCompiler;
    delete m_mdlGlslCodeGen;
    delete m_mdlRuntime;
  }

  bool _sgIsMaterialEmissive(mi::base::Handle<mi::neuraylib::ICompiled_material> compiledMaterial)
  {
    mi::base::Handle<const mi::neuraylib::IExpression> expr(compiledMaterial->lookup_sub_expression("surface.emission.intensity"));

    if (expr->get_kind() != mi::neuraylib::IExpression::Kind::EK_CONSTANT)
    {
      return true;
    }

    mi::base::Handle<const mi::neuraylib::IExpression_constant> constExpr(expr.get_interface<const mi::neuraylib::IExpression_constant>());
    mi::base::Handle<const mi::neuraylib::IValue> value(constExpr->get_value());

    if (value->get_kind() != mi::neuraylib::IValue::Kind::VK_COLOR)
    {
      assert(false);
      return true;
    }

    mi::base::Handle<const mi::neuraylib::IValue_color> color(value.get_interface<const mi::neuraylib::IValue_color>());

    if (color->get_size() != 3)
    {
      assert(false);
      return true;
    }

    mi::base::Handle<const mi::neuraylib::IValue_float> v0(color->get_value(0));
    mi::base::Handle<const mi::neuraylib::IValue_float> v1(color->get_value(1));
    mi::base::Handle<const mi::neuraylib::IValue_float> v2(color->get_value(2));

    return v0->get_value() != 0.0f || v1->get_value() != 0.0f || v2->get_value() != 0.0f;
  }

  bool _sgIsMaterialOpaque(mi::base::Handle<mi::neuraylib::ICompiled_material> compiledMaterial)
  {
    float opacity = -1.0f;

    return compiledMaterial->get_cutout_opacity(&opacity) && opacity >= 1.0f;
  }

  Material* _sgMakeMaterial(mi::base::Handle<mi::neuraylib::ICompiled_material> compiledMaterial)
  {
    Material* m = new Material();
    m->compiledMaterial = compiledMaterial;
    m->isEmissive = _sgIsMaterialEmissive(compiledMaterial);
    m->isOpaque = _sgIsMaterialOpaque(compiledMaterial);
    return m;
  }

  Material* ShaderGen::createMaterialFromMtlx(std::string_view docStr)
  {
    std::string mdlSrc;
    std::string subIdentifier;
    if (!m_mtlxMdlCodeGen->translate(docStr, mdlSrc, subIdentifier))
    {
      return nullptr;
    }

    mi::base::Handle<mi::neuraylib::ICompiled_material> compiledMaterial;
    if (!m_mdlMaterialCompiler->compileFromString(mdlSrc, subIdentifier, compiledMaterial))
    {
      return nullptr;
    }

    return _sgMakeMaterial(compiledMaterial);
  }

  Material* ShaderGen::createMaterialFromMdlFile(std::string_view filePath, std::string_view subIdentifier)
  {
    mi::base::Handle<mi::neuraylib::ICompiled_material> compiledMaterial;
    if (!m_mdlMaterialCompiler->compileFromFile(filePath, subIdentifier, compiledMaterial))
    {
      return nullptr;
    }

    return _sgMakeMaterial(compiledMaterial);
  }

  void ShaderGen::destroyMaterial(Material* mat)
  {
    delete mat;
  }

  bool ShaderGen::isMaterialEmissive(const struct Material* mat)
  {
    return mat->isEmissive;
  }

  void _sgGenerateCommonDefines(GlslSourceStitcher& stitcher, const std::vector<TextureResource>* textureResources)
  {
#if defined(NDEBUG) || defined(__APPLE__)
    stitcher.appendDefine("NDEBUG");
#endif

    int textureCount2d = 0;
    int textureCount3d = 0;
    for (const auto& texResource : *textureResources)
    {
      (texResource.is3dImage ? textureCount3d : textureCount2d)++;
    }
    if (textureCount2d > 0)
    {
      stitcher.appendDefine("HAS_TEXTURES_2D");
      stitcher.appendDefine("TEXTURE_COUNT_2D", textureCount2d);
    }
    if (textureCount3d > 0)
    {
      stitcher.appendDefine("HAS_TEXTURES_3D");
      stitcher.appendDefine("TEXTURE_COUNT_3D", textureCount3d);
    }
  }

  bool ShaderGen::generateRgenSpirv(std::string_view fileName, const RaygenShaderParams& params, std::vector<uint8_t>& spv)
  {
    GlslSourceStitcher stitcher;
    stitcher.appendVersion();

    _sgGenerateCommonDefines(stitcher, params.textureResources);

    // FIXME: 'enable' instead?
    if (params.shaderClockExts)
    {
      stitcher.appendRequiredExtension("GL_EXT_shader_explicit_arithmetic_types_int64");
      stitcher.appendRequiredExtension("GL_ARB_shader_clock");
    }

    stitcher.appendDefine("AOV_ID", params.aovId);

    fs::path filePath = m_shaderPath / fileName;
    if (!stitcher.appendSourceFile(filePath))
    {
      return false;
    }

    std::string source = stitcher.source();
    return m_shaderCompiler->compileGlslToSpv(GlslangShaderCompiler::ShaderStage::RayGen, source, spv);
  }

  bool ShaderGen::generateMissSpirv(std::string_view fileName, const MissShaderParams& params, std::vector<uint8_t>& spv)
  {
    GlslSourceStitcher stitcher;
    stitcher.appendVersion();

    _sgGenerateCommonDefines(stitcher, params.textureResources);

    fs::path filePath = m_shaderPath / fileName;
    if (!stitcher.appendSourceFile(filePath))
    {
      return false;
    }

    std::string source = stitcher.source();
    return m_shaderCompiler->compileGlslToSpv(GlslangShaderCompiler::ShaderStage::Miss, source, spv);
  }

  bool ShaderGen::generateMaterialGlslGenInfo(const Material* material, MaterialGlslGenInfo& genInfo)
  {
    const mi::neuraylib::ICompiled_material* compiledMaterial = material->compiledMaterial.get();

    std::string mdlGeneratedGlsl;
    if (!m_mdlGlslCodeGen->translate(compiledMaterial,
                                     mdlGeneratedGlsl,
                                     genInfo.textureResources))
    {
      return false;
    }

    // Remove MDL struct definitions because they're too bloated. We know more about the
    // data from which the code is generated from and can reduce the memory footprint.
    size_t mdlCodeOffset = mdlGeneratedGlsl.find("// user defined structs");
    assert(mdlCodeOffset != std::string::npos);
    mdlGeneratedGlsl = mdlGeneratedGlsl.substr(mdlCodeOffset, mdlGeneratedGlsl.size() - mdlCodeOffset);

    GlslSourceStitcher stitcher;
    if (!stitcher.appendSourceFile(m_shaderPath / "mdl_types.glsl"))
    {
      return false;
    }
    if (!stitcher.appendSourceFile(m_shaderPath / "mdl_interface.glsl"))
    {
      return false;
    }
    stitcher.appendString(mdlGeneratedGlsl);

    genInfo.glslSource = stitcher.source();

    return true;
  }

  bool ShaderGen::generateClosestHitSpirv(const ClosestHitShaderParams& params, std::vector<uint8_t>& spv)
  {
    GlslSourceStitcher stitcher;
    stitcher.appendVersion();

    _sgGenerateCommonDefines(stitcher, params.textureResources);

    stitcher.appendDefine("AOV_ID", params.aovId);
    stitcher.appendDefine("TEXTURE_INDEX_OFFSET", params.textureIndexOffset);

    fs::path filePath = m_shaderPath / params.baseFileName;
    if (!stitcher.appendSourceFile(filePath))
    {
      return false;
    }

    stitcher.replaceFirst("#pragma MDL_GENERATED_CODE", params.materialGlslSource);

    std::string source = stitcher.source();
    return m_shaderCompiler->compileGlslToSpv(GlslangShaderCompiler::ShaderStage::ClosestHit, source, spv);
  }
}
