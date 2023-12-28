module;

#include <string>

export module Foundation.DataStructures;

import Foundation.Memory.Allocators.Allocator;
import Foundation.Platform;
import Foundation.Assert;
import Foundation.Log;

export namespace Caustix {

    struct ResourcePool {
        ResourcePool(Allocator* allocator, u32 poolSize, u32 resourceSize);
        ~ResourcePool();

        u32     ObtainResource();      // Returns an index to the resource
        void    ReleaseResource( u32 index );
        void    FreeAllResources();

        void*           AccessResource( u32 index );
        const void*     AccessResource( u32 index ) const;

        u8*         m_memory        = nullptr;
        u32*        m_freeIndices   = nullptr;
        Allocator*  m_allocator     = nullptr;

        u32     m_freeIndicesHead   = 0;
        u32     m_poolSize          = 16;
        u32     m_resourceSize      = 4;
        u32     m_usedIndices       = 0;
    };
}

namespace Caustix {

    static const u32                    k_invalid_index = 0xffffffff;

// Resource Pool ////////////////////////////////////////////////////////////////
    ResourcePool::ResourcePool(Caustix::Allocator *allocator, u32 poolSize, u32 resourceSize)
    : m_allocator(allocator)
    , m_poolSize(poolSize)
    , m_resourceSize(resourceSize) {
        // Group allocate ( resource size + u32 )
        sizet allocation_size = poolSize * (resourceSize + sizeof(u32));
        m_memory = (u8*)allocator->allocate( allocation_size, 1 );
        memset( m_memory, 0, allocation_size );

        // Allocate and add free indices
        m_freeIndices = ( u32* )( m_memory + m_poolSize * m_resourceSize );
        m_freeIndicesHead = 0;

        for ( u32 i = 0; i < m_poolSize; ++i ) {
            m_freeIndices[i] = i;
        }

        m_usedIndices = 0;
    }

    ResourcePool::~ResourcePool() {

        if ( m_freeIndicesHead != 0 ) {
            info( "Resource pool has unfreed resources.\n" );

            for ( u32 i = 0; i < m_freeIndicesHead; ++i ) {
                info( "\tResource %u\n", m_freeIndices[ i ] );
            }
        }

        CASSERT( m_usedIndices == 0 );

        m_allocator->deallocate( m_memory );
    }

    void ResourcePool::FreeAllResources() {
        m_freeIndicesHead = 0;
        m_usedIndices = 0;

        for ( uint32_t i = 0; i < m_poolSize; ++i ) {
            m_freeIndices[i] = i;
        }
    }

    u32 ResourcePool::ObtainResource() {
        // TODO: add bits for checking if resource is alive and use bitmasks.
        if ( m_freeIndicesHead < m_poolSize ) {
            const u32 free_index = m_freeIndices[m_freeIndicesHead++];
            ++m_usedIndices;
            return free_index;
        }
        // Error: no more resources left!
        CASSERT( false );
        return k_invalid_index;
    }

    void ResourcePool::ReleaseResource(u32 index) {
        // TODO: add bits for checking if resource is alive and use bitmasks.
        m_freeIndices[--m_freeIndicesHead] = index;
        --m_usedIndices;
    }

    void* ResourcePool::AccessResource( u32 index ) {
        if ( index != k_invalid_index ) {
            return &m_memory[index * m_resourceSize];
        }
        return nullptr;
    }

    const void* ResourcePool::AccessResource( u32 index ) const {
        if ( index != k_invalid_index ) {
            return &m_memory[index * m_resourceSize];
        }
        return nullptr;
    }
}
