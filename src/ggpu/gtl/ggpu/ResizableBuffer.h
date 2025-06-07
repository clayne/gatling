﻿//
// Copyright (C) 2023 Pablo Delgado Krämer
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

#include <stdint.h>

#include <gtl/cgpu/Cgpu.h>

namespace gtl
{
  class GgpuDelayedResourceDestroyer;

  class GgpuResizableBuffer
  {
  public:
    GgpuResizableBuffer(CgpuDevice device,
                        GgpuDelayedResourceDestroyer& delayedResourceDestroyer,
                        CgpuBufferUsage usageFlags,
                        CgpuMemoryProperties memoryProperties);

    ~GgpuResizableBuffer();

  public:
    CgpuBuffer buffer() const;

    uint64_t size() const;

    bool resize(uint64_t newSize, CgpuCommandBuffer commandBuffer);

  private:
    CgpuDevice m_device;
    GgpuDelayedResourceDestroyer& m_delayedResourceDestroyer;
    CgpuBufferUsage m_usageFlags;
    CgpuMemoryProperties m_memoryProperties;

    CgpuBuffer m_buffer;
    uint64_t m_size = 0;
  };
}
