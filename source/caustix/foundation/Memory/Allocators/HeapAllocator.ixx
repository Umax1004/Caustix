module;

#include "External/tlsf/tlsf.h"
#include <cstdlib>

#include <imgui.h>

export module Foundation.Memory.Allocators.HeapAllocator;

import Foundation.Memory.Allocators.Allocator;
import Foundation.Memory.MemoryDefines;
import Foundation.Log;
import Foundation.Assert;

export namespace Caustix {
    struct HeapAllocator : public Allocator {
        ~HeapAllocator() override;

        HeapAllocator() = delete;
        HeapAllocator(sizet size);

        void*   allocate(sizet size, sizet alignment) override;
        void*   allocate(sizet size, sizet alignment, cstring file, i32 line) override;

        void    deallocate(void *pointer) override;

        void    debug_ui();

        void*   m_tlsfHandle;
        void*   m_memory;
        sizet   m_allocatedSize = 0;
        sizet   m_maxSize = 0;
    };
}

namespace Caustix {
    void exitWalker( void* ptr, size_t size, int used, void* user ) {
        MemoryStatistics* stats = ( MemoryStatistics* )user;
        stats->add( used ? size : 0 );

        if ( used )
            info( "Found active allocation %p, %llu\n", ptr, size );
    }

    HeapAllocator::~HeapAllocator() {
        // Check memory at the application exit.
        MemoryStatistics stats{ 0, m_maxSize };
        pool_t pool = tlsf_get_pool( m_tlsfHandle );
        tlsf_walk_pool( pool, exitWalker, ( void* )&stats );

        if ( stats.m_allocatedBytes ) {
            error( "HeapAllocator Shutdown.\n===============\nFAILURE! Allocated memory detected. allocated %llu, total %llu\n===============\n\n", stats.m_allocatedBytes, stats.m_totalBytes );
        } else {
            info( "HeapAllocator Shutdown - all memory free!" );
        }

        if (stats.m_allocatedBytes != 0)
        {
            error("Allocations still present. Check your code!");
        }
        CASSERT( stats.m_allocatedBytes == 0);

        tlsf_destroy( m_tlsfHandle );

        free( m_memory );
    }

    HeapAllocator::HeapAllocator( sizet size ) {
        // Allocate
        m_memory = malloc( size );
        m_maxSize = size;
        m_allocatedSize = 0;

        m_tlsfHandle = tlsf_create_with_pool( m_memory, size );

        info( "HeapAllocator of size {} created", size );
    }

    void* HeapAllocator::allocate( sizet size, sizet alignment ) {
        return tlsf_malloc( m_tlsfHandle, size );
    }


    void* HeapAllocator::allocate( sizet size, sizet alignment, cstring file, i32 line ) {
        return allocate( size, alignment );
    }

    void HeapAllocator::deallocate( void* pointer ) {
        tlsf_free( m_tlsfHandle, pointer );
    }

    void imgui_walker( void* ptr, size_t size, int used, void* user ) {

        u32 memory_size = ( u32 )size;
        cstring memory_unit = "b";
        if ( memory_size > 1024 * 1024 ) {
            memory_size /= 1024 * 1024;
            memory_unit = "Mb";
        }
        else if ( memory_size > 1024 ) {
            memory_size /= 1024;
            memory_unit = "kb";
        }
        ImGui::Text( "\t%p %s size: %4llu %s\n", ptr, used ? "used" : "free", memory_size, memory_unit );

        MemoryStatistics* stats = ( MemoryStatistics* )user;
        stats->add( used ? size : 0 );
    }

    void HeapAllocator::debug_ui() {

        ImGui::Separator();
        ImGui::Text( "Heap Allocator" );
        ImGui::Separator();
        MemoryStatistics stats{ 0, m_maxSize };
        pool_t pool = tlsf_get_pool( m_tlsfHandle );
        tlsf_walk_pool( pool, imgui_walker, ( void* )&stats );

        ImGui::Separator();
        ImGui::Text( "\tAllocation count %d", stats.m_allocationCount );
        ImGui::Text( "\tAllocated %llu K, free %llu Mb, total %llu Mb", stats.m_allocatedBytes / (1024 * 1024), ( m_maxSize - stats.m_allocatedBytes ) / ( 1024 * 1024 ), m_maxSize / ( 1024 * 1024 ) );
    }
}