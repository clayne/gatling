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

#include <pxr/usd/sdf/path.h>

#include <MaterialXCore/Document.h>

namespace gtl
{
  struct GiMaterial;
  struct GiScene;
}

using namespace gtl;

PXR_NAMESPACE_OPEN_SCOPE

struct HdMaterialNetwork2;

class MaterialNetworkCompiler
{
public:
  MaterialNetworkCompiler(const MaterialX::DocumentPtr mtlxStdLib);

  GiMaterial* CompileNetwork(GiScene* scene, const SdfPath& id, const HdMaterialNetwork2& network) const;

private:
  GiMaterial* _TryCompileMdlNetwork(GiScene* scene, const SdfPath& id, const HdMaterialNetwork2& network) const;

  GiMaterial* _TryCompileMtlxNetwork(GiScene* scene, const SdfPath& id, const HdMaterialNetwork2& network) const;

  MaterialX::DocumentPtr _CreateMaterialXDocumentFromNetwork(const SdfPath& id,
                                                             const HdMaterialNetwork2& network) const;

private:
  MaterialX::DocumentPtr _mtlxStdLib;
};

PXR_NAMESPACE_CLOSE_SCOPE
