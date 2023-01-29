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

#pragma once

#include <cstdint>
#include <string_view>
#include <vector>
#include <string>
#include <memory>
#include <filesystem>

namespace fs = std::filesystem;

namespace gi::sg
{
  struct Material;

  struct TextureResource
  {
    bool is3dImage;
    uint32_t binding;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    std::vector<uint8_t> data;
    std::string filePath;
  };

  class ShaderGen
  {
  public:
    struct InitParams
    {
      std::string_view resourcePath;
      std::string_view shaderPath;
      std::string_view mtlxLibPath;
      std::string_view mdlLibPath;
    };

  public:
    bool init(const InitParams& params);
    ~ShaderGen();

  public:
    Material* createMaterialFromMtlx(std::string_view docStr);
    Material* createMaterialFromMdlFile(std::string_view filePath, std::string_view subIdentifier);
    void destroyMaterial(Material* mat);
    bool isMaterialEmissive(const struct Material* mat);

  public:
    struct MaterialGlslGenInfo
    {
      std::string glslSource;
      std::vector<TextureResource> textureResources;
    };

    bool generateMaterialGlslGenInfo(const Material* material, MaterialGlslGenInfo& genInfo);

    struct RaygenShaderParams
    {
      int32_t aovId;
      bool shaderClockExts;
      const std::vector<TextureResource>* textureResources;
    };
    struct MissShaderParams
    {
      const std::vector<TextureResource>* textureResources;
    };
    struct ClosestHitShaderParams
    {
      int32_t aovId;
      std::string_view baseFileName;
      std::string_view materialGlslSource;
      uint32_t textureIndexOffset;
      std::vector<TextureResource>* textureResources;
    };

    bool generateRgenSpirv(std::string_view fileName, const RaygenShaderParams& params, std::vector<uint8_t>& spv);
    bool generateMissSpirv(std::string_view fileName, const MissShaderParams& params, std::vector<uint8_t>& spv);
    bool generateClosestHitSpirv(const ClosestHitShaderParams& params, std::vector<uint8_t>& spv);

  private:
    class MdlRuntime* m_mdlRuntime = nullptr;
    class MdlMaterialCompiler* m_mdlMaterialCompiler = nullptr;
    class MdlGlslCodeGen* m_mdlGlslCodeGen = nullptr;
    class MtlxMdlCodeGen* m_mtlxMdlCodeGen = nullptr;
    class GlslangShaderCompiler* m_shaderCompiler = nullptr;
    fs::path m_shaderPath;
  };
}
