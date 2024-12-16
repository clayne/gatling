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

#include <pxr/imaging/hd/mesh.h>
#include <pxr/base/gf/vec2f.h>

#include <gtl/gi/Gi.h>

#include <optional>

using namespace gtl;

PXR_NAMESPACE_OPEN_SCOPE

class HdGatlingMesh final : public HdMesh
{
public:
  HdGatlingMesh(const SdfPath& id,
                GiScene* scene,
                const GiMaterial* defaultMaterial);

  ~HdGatlingMesh() override;

public:
  void Sync(HdSceneDelegate* delegate,
            HdRenderParam* renderParam,
            HdDirtyBits* dirtyBits,
            const TfToken& reprToken) override;

  HdDirtyBits GetInitialDirtyBitsMask() const override;

protected:
  HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

  void _InitRepr(const TfToken& reprName,
                 HdDirtyBits *dirtyBits) override;

private:
  void _AnalyzePrimvars(HdSceneDelegate* sceneDelegate,
                        bool& foundNormals,
                        bool& indexingAllowed);

  struct ProcessedPrimvar
  {
    HdInterpolation interpolation;
    HdType type;
    TfToken role;
    VtValue indexMatchingData;
  };

  using PrimvarMap = std::unordered_map<TfToken, ProcessedPrimvar, TfToken::HashFunctor>;

  std::vector<GiPrimvarData> _CollectSecondaryPrimvars(const PrimvarMap& primvarMap);

  std::optional<ProcessedPrimvar> _ProcessPrimvar(HdSceneDelegate* sceneDelegate,
                                                  const VtIntArray& primitiveParams,
                                                  const HdPrimvarDescriptor& primvarDesc,
                                                  const VtVec3iArray& faces,
                                                  uint32_t vertexCount,
                                                  bool indexingAllowed,
                                                  bool constantAllowed);

  PrimvarMap _ProcessPrimvars(HdSceneDelegate* sceneDelegate,
                              const VtIntArray& primitiveParams,
                              const VtVec3iArray& faces,
                              uint32_t vertexCount,
                              bool indexingAllowed);

  void _CreateGiMeshes(HdSceneDelegate* sceneDelegate);

private:
  GiMesh* _baseMesh = nullptr;
  std::vector<GiMesh*> _subMeshes;

  GiScene* _scene;
  const GiMaterial* _defaultMaterial;
};

PXR_NAMESPACE_CLOSE_SCOPE
