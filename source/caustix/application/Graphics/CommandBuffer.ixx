module;

#include <vulkan/vulkan.hpp>

export module Application.Graphics.CommandBuffer;

import Application.Graphics.GPUDevice;

import Foundation.Platform;

export namespace Caustix {
    struct GpuDevice;

    struct CommandBuffer {
        CommandBuffer() = default;
        void                            Initialize( QueueType::Enum type, u32 buffer_size, u32 submit_size, bool baked );
        ~CommandBuffer();

        //
        // Commands interface
        //
        void                            BindPass( RenderPassHandle handle );
        void                            BindPipeline( PipelineHandle handle );
        void                            BindVertexBuffer( BufferHandle handle, u32 binding, u32 offset );
        void                            BindIndexBuffer( BufferHandle handle, u32 offset, VkIndexType index_type );
        void                            BindDescriptorSet( DescriptorSetHandle* handles, u32 num_lists, u32* offsets, u32 num_offsets );

        void                            SetViewport( const Viewport* viewport );
        void                            SetScissor( const Rect2DInt* rect );

        void                            clear( f32 red, f32 green, f32 blue, f32 alpha );
        void                            ClearDepthStencil( f32 depth, u8 stencil );

        void                            draw( TopologyType::Enum topology, u32 first_vertex, u32 vertex_count, u32 first_instance, u32 instance_count );
        void                            DrawIndexed( TopologyType::Enum topology, u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance );
        void                            DrawIndirect( BufferHandle handle, u32 offset, u32 stride );
        void                            DrawIndexedIndirect( BufferHandle handle, u32 offset, u32 stride );

        void                            Dispatch( u32 group_x, u32 group_y, u32 group_z );
        void                            DispatchIndirect( BufferHandle handle, u32 offset );

        void                            Barrier( const ExecutionBarrier& Barrier );

        void                            FillBuffer( BufferHandle buffer, u32 offset, u32 size, u32 data );

        void                            PushMarker( const char* name );
        void                            PopMarker();

        void                            Reset();

        VkCommandBuffer                 vk_command_buffer;

        GpuDevice*                      m_device;

        VkDescriptorSet                 vk_descriptor_sets[16];

        RenderPass*                     m_currentRenderPass;
        Pipeline*                       m_currentPipeline;
        VkClearValue                    m_clears[2];          // 0 = color, 1 = depth stencil
        bool                            m_isRecording;

        u32                             m_handle;

        u32                             m_currentCommand;
        ResourceHandle                  m_resourceHandle;
        QueueType::Enum                 m_type                = QueueType::Graphics;
        u32                             m_bufferSize          = 0;

        bool                            m_baked               = false;        // If baked reset will affect only the read of the commands.
    };

    struct CommandBufferRing {
        CommandBufferRing() = default;
        ~CommandBufferRing();
        void Initialize( GpuDevice* gpu );

        void                    ResetPools( u32 frameIndex );

        CommandBuffer*          GetCommandBuffer( u32 frame, bool begin );
        CommandBuffer*          GetCommandBufferInstant( u32 frame, bool begin );

        static u16              PoolFromIndex( u32 index ) { return (u16)index / k_buffer_per_pool; }

        static const u16        k_max_threads = 1;
        static const u16        k_max_pools = k_max_swapchain_images * k_max_threads;
        static const u16        k_buffer_per_pool = 4;
        static const u16        k_max_buffers = k_buffer_per_pool * k_max_pools;

        GpuDevice*              m_gpu;
        VkCommandPool           m_vulkanCommandPools[ k_max_pools ];
        CommandBuffer           m_commandBuffers[ k_max_buffers ];
        u8                      m_nextFreePerThreadFrame[ k_max_pools ];
    };
}