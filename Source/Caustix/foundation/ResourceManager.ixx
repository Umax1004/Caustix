module;

#include <wyhash.h>
#include <unordered_map>

export module Foundation.ResourceManager;

import Foundation.Memory.Allocators.Allocator;
import Foundation.Platform;
import Foundation.Assert;

export namespace Caustix {
    struct ResourceManager;

    struct Resource {
        void            AddReference()     { ++m_references; }
        void            RemoveReference()  { CASSERT( m_references != 0 ); --m_references; }

        u64             m_references  = 0;
        cstring         m_name        = nullptr;
    };

    struct ResourceCompiler {

    };

    struct ResourceLoader {
        virtual Resource*   Get( cstring name ) = 0;
        virtual Resource*   Get( u64 hashedName ) = 0;

        virtual Resource*   Unload( cstring name ) = 0;

        virtual Resource*   CreateFromFile( cstring name, cstring filename, ResourceManager* resourceManager ) { return nullptr; }
    };

    struct ResourceFilenameResolver {
        virtual cstring GetBinaryPathFromName( cstring name ) = 0;
    };

    #define FlatHashMap(key, value) std::unordered_map<key, value, std::hash<key>, std::equal_to<key>, STLAdaptor<std::pair<const key, value>>>

    struct ResourceManager {
        ResourceManager( Allocator* allocator, ResourceFilenameResolver* resolver );
        ~ResourceManager();

        template <typename T>
        T*              Load( cstring name );

        template <typename T>
        T*              Get( cstring name );

        template <typename T>
        T*              Get( u64 hashedName );

        template <typename T>
        T*              Reload( cstring name );

        void            SetLoader( cstring resourceType, ResourceLoader* loader );
        void            SetCompiler( cstring resourceType, ResourceCompiler* compiler );

        FlatHashMap(u64, ResourceLoader*)       m_loaders;
        FlatHashMap(u64, ResourceCompiler*)     m_compilers;

        Allocator*      m_allocator;
        ResourceFilenameResolver* m_filenameResolver;
    };

    template<typename T>
    inline T* ResourceManager::Load( cstring name ) {
        ResourceLoader* loader = m_loaders[ T::k_type_hash ];
        if ( loader ) {
            // Search if the resource is already in cache
            T* resource = ( T* )loader->Get( name );
            if ( resource )
                return resource;

            // Resource not in cache, create from file
            cstring path = m_filenameResolver->GetBinaryPathFromName( name );
            return (T*)loader->CreateFromFile( name, path, this );
        }
        return nullptr;
    }

    template<typename T>
    inline T* ResourceManager::Get( cstring name ) {
        ResourceLoader* loader = m_loaders[ T::k_type_hash ];
        if ( loader ) {
            return ( T* )loader->Get( name );
        }
        return nullptr;
    }

    template<typename T>
    inline T* ResourceManager::Get( u64 hashedName ) {
        ResourceLoader* loader = m_loaders[ T::k_type_hash ];
        if ( loader ) {
            return ( T* )loader->Get( hashedName );
        }
        return nullptr;
    }

    template<typename T>
    inline T* ResourceManager::Reload(cstring name) {
        ResourceLoader* loader = m_loaders[ T::k_type_hash ];
        if ( loader ) {
            T* resource = ( T* )loader->Get( name );
            if ( resource ) {
                loader->Unload( name );

                // Resource not in cache, create from file
                cstring path = m_filenameResolver->GetBinaryPathFromName( name );
                return ( T* )loader->CreateFromFile( name, path, this );
            }
        }
        return nullptr;
    }
}

namespace Caustix {
    ResourceManager::ResourceManager( Allocator* allocator, ResourceFilenameResolver* resolver )
    : m_allocator(allocator)
    , m_compilers(*allocator)
    , m_loaders(*allocator) {

        m_loaders.reserve( 8 );
        m_compilers.reserve( 8 );
    }

    ResourceManager::~ResourceManager() {

        m_loaders.clear();
        m_compilers.clear();
    }

    void ResourceManager::SetLoader( cstring resourceType, ResourceLoader* loader ) {
        const u64 hashedName = HashCalculate( resourceType );
        m_loaders.emplace( hashedName, loader );
    }

    void ResourceManager::SetCompiler( cstring resourceType, ResourceCompiler* compiler ) {
        const u64 hashName = HashCalculate( resourceType );
        m_compilers.emplace( hashName, compiler );
    }
}
