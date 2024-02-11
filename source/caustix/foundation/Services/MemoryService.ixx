module;

#include <memory>

#include <imgui.h>

export module Foundation.Services.MemoryService;

import Foundation.Memory.Allocators.LinearAllocator;
import Foundation.Memory.Allocators.HeapAllocator;
import Foundation.Memory.MemoryDefines;
import Foundation.Services.Service;
import Foundation.Platform;

export namespace Caustix {
    struct MemoryService : public Service {
        MemoryService() = delete;
        MemoryService(MemoryServiceConfiguration configuration);
        // Frame allocator
        LinearAllocator m_scratchAllocator;
        HeapAllocator m_systemAllocator;

        void ImguiDraw();

        static MemoryService* Create(MemoryServiceConfiguration configuration) {
            static std::unique_ptr<MemoryService> instance{new MemoryService(configuration)};
            return instance.get();
        }

        static constexpr cstring m_name = "caustix_memory_service";
    };
}

namespace Caustix {
    MemoryService::MemoryService(Caustix::MemoryServiceConfiguration configuration)
    : m_systemAllocator(configuration.m_maximumDynamicSize) {}

    void MemoryService::ImguiDraw() {
        if ( ImGui::Begin( "Memory Service" ) ) {
            m_systemAllocator.debug_ui();
        }
        ImGui::End();
    }
}