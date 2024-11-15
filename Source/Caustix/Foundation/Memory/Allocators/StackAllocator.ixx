module;

#include <cstdlib>

export module Foundation.Memory.Allocators.StackAllocator;

import Foundation.Memory.Allocators.Allocator;
import Foundation.Platform;

export namespace Caustix {
    struct StackAllocator : public Allocator {
        ~StackAllocator() override;

        StackAllocator() = delete;
        StackAllocator(sizet size);

        void*   allocate(sizet size, sizet alignment) override;
        void*   allocate(sizet size, sizet alignment, cstring file, i32 line) override;

        void    deallocate(void* pointer) override;

        [[nodiscard]] sizet   GetMarker() const;
        void    FreeMarker(sizet marker);

        u8*     m_memory        = nullptr;
        sizet   m_totalSize     = 0;
        sizet   m_allocatedSize = 0;
    };
}

namespace Caustix {

    StackAllocator::~StackAllocator() {
        m_allocatedSize = 0;
        free(m_memory);
    }

    StackAllocator::StackAllocator(sizet size) {
        m_memory = (u8*)malloc( size );
        m_allocatedSize = 0;
        m_totalSize = size;
    }

    void* StackAllocator::allocate(sizet size, sizet alignment) {
        CASSERT(size > 0);
        const sizet new_start = MemoryAlign(m_allocatedSize, alignment);
        CASSERT(new_start < m_totalSize);
        const sizet new_allocated_size = new_start + size;
        if (new_allocated_size > m_totalSize) {
            return nullptr;
        }

        m_allocatedSize = new_allocated_size;
        return m_memory + new_start;
    }

    void* StackAllocator::allocate(sizet size, sizet alignment, cstring file, i32 line) {
        return allocate( size, alignment );
    }

    void StackAllocator::deallocate(void *pointer) {
        CASSERT( pointer >= m_memory );
        CASSERT( pointer < m_memory + m_totalSize );
        CASSERT( pointer < m_memory + m_allocatedSize );
//        RASSERTM( pointer < memory + total_size, "Out of bound free on linear allocator (outside bounds). Tempting to free %p, %llu after beginning of buffer (memory %p size %llu, allocated %llu)", ( u8* )pointer, ( u8* )pointer - memory, memory, total_size, allocated_size );
//        RASSERTM( pointer < memory + allocated_size, "Out of bound free on linear allocator (inside bounds, after allocated). Tempting to free %p, %llu after beginning of buffer (memory %p size %llu, allocated %llu)", ( u8* )pointer, ( u8* )pointer - memory, memory, total_size, allocated_size );

        const sizet size_at_pointer = ( u8* )pointer - m_memory;

        m_allocatedSize = size_at_pointer;
    }

    sizet StackAllocator::GetMarker() const {
        return m_allocatedSize;
    }

    void StackAllocator::FreeMarker(sizet marker) {
        const sizet difference = marker - m_allocatedSize;
        if ( difference > 0 ) {
            m_allocatedSize = marker;
        }
    }
}