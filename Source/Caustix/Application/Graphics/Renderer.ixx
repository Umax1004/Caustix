module;

#include <unordered_map>
#include <memory>

#include <vulkan/vulkan.h>

#include <wyhash.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

export module Application.Graphics.Renderer;

import Foundation.Memory.Allocators.Allocator;
import Foundation.Services.Service;
import Foundation.ResourceManager;
import Foundation.Platform;
import Foundation.DataStructures;
import Foundation.Log;

import Application.Graphics.GPUResources;
import Application.Graphics.GPUDevice;
import Application.Graphics.CommandBuffer;

export namespace Caustix {
    struct Renderer;

    struct BufferResource : public Resource {
        BufferHandle                    m_handle;
        u32                             m_poolIndex;
        BufferDescription               m_desc;

        static constexpr cstring        k_type = "caustix_buffer_type";
        static u64                      k_type_hash;
    };

    struct TextureResource : public Resource {
        TextureHandle                   m_handle;
        u32                             m_poolIndex;
        TextureDescription              m_desc;

        static constexpr cstring        k_type = "caustix_texture_type";
        static u64                      k_type_hash;
    };

    struct SamplerResource : public Resource {
        SamplerHandle                   m_handle;
        u32                             m_poolIndex;
        SamplerDescription              m_desc;

        static constexpr cstring        k_type = "caustix_sampler_type";
        static u64                      k_type_hash;
    };

    #define FlatHashMap(key, value) std::unordered_map<key, value, std::hash<key>, std::equal_to<key>, STLAdaptor<std::pair<const key, value>>>

    struct ResourceCache {
        ResourceCache( Allocator* allocator );
        void    Shutdown( Renderer* renderer );

        FlatHashMap(u64, TextureResource*) m_textures;
        FlatHashMap(u64, BufferResource*)  m_buffers;
        FlatHashMap(u64, SamplerResource*) m_samplers;
    };

    struct RendererCreation {
        GpuDevice*          gpu;
        Allocator*                  allocator;
    };

    struct Renderer : public Service {
        Renderer( const RendererCreation& creation );
        void                        Shutdown();

        void                        SetLoaders( ResourceManager* manager );

        void                        BeginFrame();
        void                        EndFrame();

        void                        ResizeSwapchain( u32 width, u32 height );

        f32                         AspectRatio() const;

        // Creation/destruction
        BufferResource*             CreateBuffer( const BufferCreation& creation );
        BufferResource*             CreateBuffer( VkBufferUsageFlags type, ResourceUsageType::Enum usage, u32 size, void* data, cstring name );

        TextureResource*            CreateTexture( const TextureCreation& creation );
        TextureResource*            CreateTexture( cstring name, cstring filename );

        SamplerResource*            CreateSampler( const SamplerCreation& creation );

        void                        DestroyBuffer( BufferResource* buffer );
        void                        DestroyTexture( TextureResource* texture );
        void                        DestroySampler( SamplerResource* sampler );

        // Update resources
        void*                       MapBuffer( BufferResource* buffer, u32 offset = 0, u32 size = 0 );
        void                        UnmapBuffer( BufferResource* buffer );

        CommandBuffer*              GetCommandBuffer( QueueType::Enum type, bool begin )  { return m_gpu->get_command_buffer( type, begin ); }
        void                        QueueCommandBuffer( CommandBuffer* commands ) { m_gpu->queue_command_buffer( commands ); }

        ResourcePoolTyped<TextureResource>  m_textures;
        ResourcePoolTyped<BufferResource>   m_buffers;
        ResourcePoolTyped<SamplerResource>  m_samplers;

        ResourceCache               m_resourceCache;

        GpuDevice*                  m_gpu;

        u16                         m_width;
        u16                         m_height;

        static Renderer* Create(const RendererCreation& creation) {
            static std::unique_ptr<Renderer> instance{new Renderer(creation)};
            return instance.get();
        }

        static constexpr cstring    m_name = "caustix_rendering_service";

    };
}

namespace Caustix {
    struct TextureLoader : public ResourceLoader {
        Resource*                       Get( cstring name ) override;
        Resource*                       Get( u64 hashedName ) override;

        Resource*                       Unload( cstring name ) override;

        Resource*                       CreateFromFile( cstring name, cstring filename, ResourceManager* resourceManager ) override;

        Renderer*                       m_renderer;
    };

    struct BufferLoader : public ResourceLoader {
        Resource*                       Get( cstring name ) override;
        Resource*                       Get( u64 hashedName ) override;

        Resource*                       Unload( cstring name ) override;

        Renderer*                       m_renderer;
    };

    struct SamplerLoader : public ResourceLoader {
        Resource*                       Get( cstring name ) override;
        Resource*                       Get( u64 hashedName ) override;

        Resource*                       Unload( cstring name ) override;

        Renderer*                       m_renderer;
    };

    static TextureHandle CreateTextureFromFile( GpuDevice& gpu, cstring filename, cstring name ) {

        if ( filename ) {
            int comp, width, height;
            uint8_t* image_data = stbi_load( filename, &width, &height, &comp, 4 );
            if ( !image_data ) {
                error( "Error loading texture {}", filename );
                return k_invalid_texture;
            }

            TextureCreation creation;
            creation.SetData( image_data ).SetFormatType( VK_FORMAT_R8G8B8A8_UNORM, TextureType::Texture2D ).SetFlags( 1, 0 ).SetSize( ( u16 )width, ( u16 )height, 1 ).SetName( name );

            TextureHandle new_texture = gpu.create_texture( creation );

            // IMPORTANT:
            // Free memory loaded from file, it should not matter!
            free( image_data );

            return new_texture;
        }

        return k_invalid_texture;
    }

    u64 TextureResource::k_type_hash = 0;
    u64 BufferResource::k_type_hash = 0;
    u64 SamplerResource::k_type_hash = 0;

    static TextureLoader s_texture_loader;
    static BufferLoader s_buffer_loader;
    static SamplerLoader s_sampler_loader;

    Renderer::Renderer(const RendererCreation &creation)
    : m_textures(creation.allocator, 512)
    , m_buffers(creation.allocator, 4096)
    , m_samplers(creation.allocator, 128)
    , m_resourceCache(creation.allocator) {

        info( "Renderer init\n" );

        m_gpu = creation.gpu;

        m_width = m_gpu->swapchain_width;
        m_height = m_gpu->swapchain_height;

        // Init resource hashes
        TextureResource::k_type_hash = HashCalculate( TextureResource::k_type );
        BufferResource::k_type_hash = HashCalculate( BufferResource::k_type );
        SamplerResource::k_type_hash = HashCalculate( SamplerResource::k_type );

        s_texture_loader.m_renderer = this;
        s_buffer_loader.m_renderer = this;
        s_sampler_loader.m_renderer = this;
    }

    void Renderer::Shutdown() {
        m_resourceCache.Shutdown( this );

        info( "Renderer shutdown" );

        m_gpu->Shutdown();
    }

    void Renderer::SetLoaders( ResourceManager* manager ) {

        manager->SetLoader( TextureResource::k_type, &s_texture_loader );
        manager->SetLoader( BufferResource::k_type, &s_buffer_loader );
        manager->SetLoader( SamplerResource::k_type, &s_sampler_loader );
    }

    void Renderer::BeginFrame() {
        m_gpu->new_frame();
    }

    void Renderer::EndFrame() {
        // Present
        m_gpu->present();
    }

    void Renderer::ResizeSwapchain(u32 width, u32 height) {
        m_gpu->resize( (u16)width, (u16)height );

        m_width = m_gpu->swapchain_width;
        m_height = m_gpu->swapchain_height;
    }

    f32 Renderer::AspectRatio() const {
        return m_gpu->swapchain_width * 1.f / m_gpu->swapchain_height;
    }

    BufferResource* Renderer::CreateBuffer( const BufferCreation& creation ) {

        BufferResource* buffer = m_buffers.Obtain();
        if ( buffer ) {
            BufferHandle handle = m_gpu->create_buffer( creation );
            buffer->m_handle = handle;
            buffer->m_name = creation.m_name;
            m_gpu->query_buffer( handle, buffer->m_desc );

            if ( creation.m_name != nullptr ) {
                m_resourceCache.m_buffers.emplace( HashCalculate( creation.m_name ), buffer );
            }

            buffer->m_references = 1;

            return buffer;
        }
        return nullptr;
    }

    BufferResource* Renderer::CreateBuffer( VkBufferUsageFlags type, ResourceUsageType::Enum usage, u32 size, void* data, cstring name ) {
        BufferCreation creation{ type, usage, size, data, name };
        return CreateBuffer( creation );
    }

    TextureResource* Renderer::CreateTexture( const TextureCreation& creation ) {
        TextureResource* texture = m_textures.Obtain();

        if ( texture ) {
            TextureHandle handle = m_gpu->create_texture( creation );
            texture->m_handle = handle;
            texture->m_name = creation.m_name;
            m_gpu->query_texture( handle, texture->m_desc );

            if ( creation.m_name != nullptr ) {
                m_resourceCache.m_textures.emplace( HashCalculate( creation.m_name ), texture );
            }

            texture->m_references = 1;

            return texture;
        }
        return nullptr;
    }

    TextureResource* Renderer::CreateTexture( cstring name, cstring filename ) {
        TextureResource* texture = m_textures.Obtain();

        if ( texture ) {
            TextureHandle handle = CreateTextureFromFile( *m_gpu, filename, name );
            texture->m_handle = handle;
            m_gpu->query_texture( handle, texture->m_desc );
            texture->m_references = 1;
            texture->m_name = name;

            m_resourceCache.m_textures.emplace( HashCalculate( name ), texture );

            return texture;
        }
        return nullptr;
    }

    SamplerResource* Renderer::CreateSampler( const SamplerCreation& creation ) {
        SamplerResource* sampler = m_samplers.Obtain();
        if ( sampler ) {
            SamplerHandle handle = m_gpu->create_sampler( creation );
            sampler->m_handle = handle;
            sampler->m_name = creation.m_name;
            m_gpu->query_sampler( handle, sampler->m_desc );

            if ( creation.m_name != nullptr ) {
                m_resourceCache.m_samplers.emplace( HashCalculate( creation.m_name ), sampler );
            }

            sampler->m_references = 1;

            return sampler;
        }
        return nullptr;
    }

    void Renderer::DestroyBuffer( BufferResource* buffer ) {
        if ( !buffer ) {
            return;
        }

        buffer->RemoveReference();
        if ( buffer->m_references ) {
            return;
        }

        m_resourceCache.m_buffers.erase( HashCalculate( buffer->m_desc.m_name ) );
        m_gpu->destroy_buffer( buffer->m_handle );
        m_buffers.Release( buffer );
    }

    void Renderer::DestroyTexture( TextureResource* texture ) {
        if ( !texture ) {
            return;
        }

        texture->RemoveReference();
        if ( texture->m_references ) {
            return;
        }

        m_resourceCache.m_textures.erase( HashCalculate( texture->m_desc.m_name ) );
        m_gpu->destroy_texture( texture->m_handle );
        m_textures.Release( texture );
    }

    void Renderer::DestroySampler( SamplerResource* sampler ) {
        if ( !sampler ) {
            return;
        }

        sampler->RemoveReference();
        if ( sampler->m_references ) {
            return;
        }

        m_resourceCache.m_samplers.erase( HashCalculate( sampler->m_desc.m_name ) );
        m_gpu->destroy_sampler( sampler->m_handle );
        m_samplers.Release( sampler );
    }

    void* Renderer::MapBuffer( BufferResource* buffer, u32 offset, u32 size ) {

        MapBufferParameters cb_map = { buffer->m_handle, offset, size };
        return m_gpu->map_buffer( cb_map );
    }

    void Renderer::UnmapBuffer( BufferResource* buffer ) {

        if ( buffer->m_desc.m_parentHandle.m_index == k_invalid_index ) {
            MapBufferParameters cb_map = { buffer->m_handle, 0, 0 };
            m_gpu->unmap_buffer( cb_map );
        }
    }

// Resource Loaders ///////////////////////////////////////////////////////

// Texture Loader /////////////////////////////////////////////////////////
    Resource* TextureLoader::Get( cstring name ) {
        const u64 hashedName = HashCalculate( name );
        return m_renderer->m_resourceCache.m_textures[ hashedName ];
    }

    Resource* TextureLoader::Get( u64 hashedName ) {
        return m_renderer->m_resourceCache.m_textures[ hashedName ];
    }

    Resource* TextureLoader::Unload( cstring name ) {
        const u64 hashedName = HashCalculate( name );
        TextureResource* texture = m_renderer->m_resourceCache.m_textures[ hashedName ];
        if ( texture ) {
            m_renderer->DestroyTexture( texture );
        }
        return nullptr;
    }

    Resource* TextureLoader::CreateFromFile( cstring name, cstring filename, ResourceManager* resource_manager ) {
        return m_renderer->CreateTexture( name, filename );
    }

// BufferLoader //////////////////////////////////////////////////////////
    Resource* BufferLoader::Get( cstring name ) {
        const u64 hashedName = HashCalculate( name );
        return m_renderer->m_resourceCache.m_buffers[ hashedName ];
    }

    Resource* BufferLoader::Get( u64 hashedName ) {
        return m_renderer->m_resourceCache.m_buffers[ hashedName ];
    }

    Resource* BufferLoader::Unload( cstring name ) {
        const u64 hashedName = HashCalculate( name );
        BufferResource* buffer = m_renderer->m_resourceCache.m_buffers[ hashedName ];
        if ( buffer ) {
            m_renderer->DestroyBuffer( buffer );
        }

        return nullptr;
    }

// SamplerLoader /////////////////////////////////////////////////////////
    Resource* SamplerLoader::Get( cstring name ) {
        const u64 hashedName = HashCalculate( name );
        return m_renderer->m_resourceCache.m_samplers[ hashedName ];
    }

    Resource* SamplerLoader::Get( u64 hashedName ) {
        return m_renderer->m_resourceCache.m_samplers[ hashedName ];
    }

    Resource* SamplerLoader::Unload( cstring name ) {
        const u64 hashedName = HashCalculate( name );
        SamplerResource* sampler = m_renderer->m_resourceCache.m_samplers[ hashedName ];
        if ( sampler ) {
            m_renderer->DestroySampler( sampler );
        }
        return nullptr;
    }

    // ResourceCache
    ResourceCache::ResourceCache( Allocator* allocator )
    : m_textures(*allocator)
    , m_buffers(*allocator)
    , m_samplers(*allocator){
        // Init resources caching
        m_textures.reserve( 16 );
        m_buffers.reserve( 16 );
        m_samplers.reserve( 16 );
    }

    void ResourceCache::Shutdown( Renderer* renderer ) {

//        raptor::FlatHashMapIterator it = textures.iterator_begin();
//
//        while ( it.is_valid() ) {
//            raptor::TextureResource* texture = textures.get( it );
//            renderer->destroy_texture( texture );
//
//            textures.iterator_advance( it );
//        }

        for (auto it = m_textures.begin(); it != m_textures.end(); it++) {
            TextureResource* texture = it->second;
            renderer->DestroyTexture( texture );
        }

        for (auto it = m_buffers.begin(); it != m_buffers.end(); it++) {
            BufferResource* buffer = it->second;
            renderer->DestroyBuffer( buffer );
        }

        for (auto it = m_samplers.begin(); it != m_samplers.end(); it++) {
            SamplerResource* sampler = it->second;
            renderer->DestroySampler( sampler );
        }

        m_textures.clear();
        m_buffers.clear();
        m_samplers.clear();
    }
}