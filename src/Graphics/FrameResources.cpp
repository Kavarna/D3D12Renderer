#include "FrameResources.h"

#include "Direct3D.h"

bool FrameResources::Init(uint32_t numObjects, uint32_t numPasses, uint32_t numMaterials)
{
    auto d3d = Direct3D::Get();

    auto allocatorResult = d3d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
    CHECK(allocatorResult.Valid(), false, "Unable to create a command allocator for frame resources");
    CommandAllocator = allocatorResult.Get();

    CHECK(PerObjectBuffers.Init(numObjects, true), false,
          "Unable to initialize per object buffer with {} elements", numObjects);

    CHECK(PerPassBuffers.Init(numPasses, true), false,
          "Unable to initialize per pass buffer with {} elements", numPasses);

    CHECK(MaterialsBuffers.Init(numMaterials, true), false,
          "Unable to initialize material buffer with {} elements", numMaterials);

    CHECK(LightsBuffer.Init(1, true), false,
          "Unable to initialize lights buffer with {} elements", 1);

    return true;
}
