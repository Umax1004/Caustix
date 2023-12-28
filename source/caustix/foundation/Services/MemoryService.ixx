module;

#include <memory>

export module Foundation.Memory.MemoryService;

import Foundation.Memory.Allocators.LinearAllocator;
import Foundation.Memory.Allocators.HeapAllocator;
import Foundation.Memory.MemoryDefines;
import Foundation.Services.Service;
import Foundation.Platform;

export namespace Caustix {
    struct MemoryService : public Service {
        void Initialize(MemoryServiceConfiguration configuration);
        // Frame allocator
        LinearAllocator m_scratchAllocator;
        HeapAllocator m_systemAllocator;

        static MemoryService* GetInstance() {
            static std::unique_ptr<MemoryService> instance;
            return instance.get();
        }

        static constexpr cstring m_name = "caustix_memory_service";
    };
}

namespace Caustix {
    void MemoryService::Initialize(Caustix::MemoryServiceConfiguration configuration)
    {
        m_systemAllocator.Initialize(configuration.m_maximumDynamicSize);
    }
}