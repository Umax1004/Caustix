module;

#include <cstdlib>

export module Foundation.Memory.Allocators.LinearAllocator;

import Foundation.Memory.Allocators.Allocator;

export namespace Caustix {
    struct LinearAllocator : public Allocator {
        ~LinearAllocator() override;

        LinearAllocator() = default;
        explicit LinearAllocator(sizet size);

        void*   allocate( sizet size, sizet alignment ) override;
        void*   allocate( sizet size, sizet alignment, cstring file, i32 line ) override;

        void    deallocate( void* pointer ) override;

        u8*     m_memory          = nullptr;
        sizet   m_totalSize      = 0;
        sizet   m_allocatedSize  = 0;
    };
}

namespace Caustix {
    LinearAllocator::~LinearAllocator() {
        m_allocatedSize = 0;
        free(m_memory);
    }

    LinearAllocator::LinearAllocator(sizet size) {
        m_memory = (u8*)malloc( size );
        m_allocatedSize = 0;
        m_totalSize = size;
    }

    void* LinearAllocator::allocate(sizet size, sizet alignment) {
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

    void* LinearAllocator::allocate(sizet size, sizet alignment, cstring file, i32 line) {
        return allocate( size, alignment );
    }

    void LinearAllocator::deallocate(void *pointer) {
        // This allocator does not allocate on a per-pointer base!
    }
}