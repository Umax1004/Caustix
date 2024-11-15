module;

#include <vulkan/vulkan.h>

module Application.Graphics.CommandBuffer;

import Application.Graphics.GPUDevice;

namespace Caustix {
    void CommandBuffer::Reset() {

        m_isRecording = false;
        m_currentRenderPass = nullptr;
        m_currentPipeline = nullptr;
        m_currentCommand = 0;
    }


    void CommandBuffer::Initialize(QueueType::Enum type_, u32 buffer_size_, u32 submit_size, bool baked_) {
        this->m_type = type_;
        this->m_bufferSize = buffer_size_;
        this->m_baked = baked_;

        Reset();
    }

    CommandBuffer::~CommandBuffer() {
        m_isRecording = false;
    }

    void CommandBuffer::BindPass(RenderPassHandle handle_) {

        //if ( !m_isRecording )
        {
            m_isRecording = true;

            RenderPass *render_pass = m_device->access_render_pass(handle_);

            // Begin/End render pass are valid only for graphics render passes.
            if (m_currentRenderPass && (m_currentRenderPass->m_type != RenderPassType::Compute) &&
                (render_pass != m_currentRenderPass)) {
                vkCmdEndRenderPass(vk_command_buffer);
            }

            if (render_pass != m_currentRenderPass && (render_pass->m_type != RenderPassType::Compute)) {
                VkRenderPassBeginInfo render_pass_begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
                render_pass_begin.framebuffer = render_pass->m_type == RenderPassType::Swapchain
                                                ? m_device->vulkan_swapchain_framebuffers[m_device->vulkan_image_index]
                                                : render_pass->vk_frame_buffer;
                render_pass_begin.renderPass = render_pass->vk_render_pass;

                render_pass_begin.renderArea.offset = {0, 0};
                render_pass_begin.renderArea.extent = {render_pass->m_width, render_pass->m_height};

                // TODO: this breaks.
                render_pass_begin.clearValueCount = 2;// render_pass->output.color_operation ? 2 : 0;
                render_pass_begin.pClearValues = m_clears;

                vkCmdBeginRenderPass(vk_command_buffer, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);
            }

            // Cache render pass
            m_currentRenderPass = render_pass;
        }
    }

    void CommandBuffer::BindPipeline(PipelineHandle handle_) {

        Pipeline *pipeline = m_device->access_pipeline(handle_);
        vkCmdBindPipeline(vk_command_buffer, pipeline->vk_bind_point, pipeline->vk_pipeline);

        // Cache pipeline
        m_currentPipeline = pipeline;
    }

    void CommandBuffer::BindVertexBuffer(BufferHandle handle_, u32 binding, u32 offset) {

        Buffer *buffer = m_device->access_buffer(handle_);
        VkDeviceSize offsets[] = {offset};

        VkBuffer vk_buffer = buffer->vk_buffer;
        // TODO: add global vertex buffer ?
        if (buffer->m_parentBuffer.m_index != k_invalid_index) {
            Buffer *parent_buffer = m_device->access_buffer(buffer->m_parentBuffer);
            vk_buffer = parent_buffer->vk_buffer;
            offsets[0] = buffer->m_globalOffset;
        }

        vkCmdBindVertexBuffers(vk_command_buffer, binding, 1, &vk_buffer, offsets);
    }

    void CommandBuffer::BindIndexBuffer(BufferHandle handle_, u32 offset_, VkIndexType index_type) {

        Buffer *buffer = m_device->access_buffer(handle_);

        VkBuffer vk_buffer = buffer->vk_buffer;
        VkDeviceSize offset = offset_;
        if (buffer->m_parentBuffer.m_index != k_invalid_index) {
            Buffer *parent_buffer = m_device->access_buffer(buffer->m_parentBuffer);
            vk_buffer = parent_buffer->vk_buffer;
            offset = buffer->m_globalOffset;
        }
        vkCmdBindIndexBuffer(vk_command_buffer, vk_buffer, offset, index_type);
    }

    void CommandBuffer::BindDescriptorSet(DescriptorSetHandle *handles, u32 num_lists, u32 *offsets, u32 num_offsets) {

        // TODO:
        u32 offsets_cache[8];
        num_offsets = 0;

        for (u32 l = 0; l < num_lists; ++l) {
            DesciptorSet *descriptor_set = m_device->access_descriptor_set(handles[l]);
            vk_descriptor_sets[l] = descriptor_set->vk_descriptor_set;

            // Search for dynamic buffers
            const DesciptorSetLayout *descriptor_set_layout = descriptor_set->m_layout;
            for (u32 i = 0; i < descriptor_set_layout->m_numBindings; ++i) {
                const DescriptorBinding &rb = descriptor_set_layout->m_bindings[i];

                if (rb.m_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                    // Search for the actual buffer offset
                    const u32 resource_index = descriptor_set->m_bindings[i];
                    ResourceHandle buffer_handle = descriptor_set->m_resources[resource_index];
                    Buffer *buffer = m_device->access_buffer({buffer_handle});

                    offsets_cache[num_offsets++] = buffer->m_globalOffset;
                }
            }
        }

        const u32 k_first_set = 0;
        vkCmdBindDescriptorSets(vk_command_buffer, m_currentPipeline->vk_bind_point,
                                m_currentPipeline->vk_pipeline_layout, k_first_set,
                                num_lists, vk_descriptor_sets, num_offsets, offsets_cache);
    }

    void CommandBuffer::SetViewport(const Viewport *viewport) {

        VkViewport vk_viewport;

        if (viewport) {
            vk_viewport.x = viewport->m_rect.m_x * 1.f;
            vk_viewport.width = viewport->m_rect.m_width * 1.f;
            // Invert Y with negative height and proper offset - Vulkan has unique Clipping Y.
            vk_viewport.y = viewport->m_rect.m_height * 1.f - viewport->m_rect.m_y;
            vk_viewport.height = -viewport->m_rect.m_height * 1.f;
            vk_viewport.minDepth = viewport->m_minDepth;
            vk_viewport.maxDepth = viewport->m_maxDepth;
        } else {
            vk_viewport.x = 0.f;

            if (m_currentRenderPass) {
                vk_viewport.width = m_currentRenderPass->m_width * 1.f;
                // Invert Y with negative height and proper offset - Vulkan has unique Clipping Y.
                vk_viewport.y = m_currentRenderPass->m_height * 1.f;
                vk_viewport.height = -m_currentRenderPass->m_height * 1.f;
            } else {
                vk_viewport.width = m_device->swapchain_width * 1.f;
                // Invert Y with negative height and proper offset - Vulkan has unique Clipping Y.
                vk_viewport.y = m_device->swapchain_height * 1.f;
                vk_viewport.height = -m_device->swapchain_height * 1.f;
            }
            vk_viewport.minDepth = 0.0f;
            vk_viewport.maxDepth = 1.0f;
        }

        vkCmdSetViewport(vk_command_buffer, 0, 1, &vk_viewport);
    }

    void CommandBuffer::SetScissor(const Rect2DInt *rect) {

        VkRect2D vk_scissor;

        if (rect) {
            vk_scissor.offset.x = rect->m_x;
            vk_scissor.offset.y = rect->m_y;
            vk_scissor.extent.width = rect->m_width;
            vk_scissor.extent.height = rect->m_height;
        } else {
            vk_scissor.offset.x = 0;
            vk_scissor.offset.y = 0;
            vk_scissor.extent.width = m_device->swapchain_width;
            vk_scissor.extent.height = m_device->swapchain_height;
        }

        vkCmdSetScissor(vk_command_buffer, 0, 1, &vk_scissor);
    }

    void CommandBuffer::clear(f32 red, f32 green, f32 blue, f32 alpha) {
        m_clears[0].color = {red, green, blue, alpha};
    }

    void CommandBuffer::ClearDepthStencil(f32 depth, u8 value) {
        m_clears[1].depthStencil.depth = depth;
        m_clears[1].depthStencil.stencil = value;
    }

    void CommandBuffer::draw(TopologyType::Enum topology, u32 first_vertex, u32 vertex_count, u32 first_instance,
                             u32 instance_count) {
        vkCmdDraw(vk_command_buffer, vertex_count, instance_count, first_vertex, first_instance);
    }

    void CommandBuffer::DrawIndexed(TopologyType::Enum topology, u32 index_count, u32 instance_count, u32 first_index,
                                    i32 vertex_offset, u32 first_instance) {
        vkCmdDrawIndexed(vk_command_buffer, index_count, instance_count, first_index, vertex_offset, first_instance);
    }

    void CommandBuffer::Dispatch(u32 group_x, u32 group_y, u32 group_z) {
        vkCmdDispatch(vk_command_buffer, group_x, group_y, group_z);
    }

    void CommandBuffer::DrawIndirect(BufferHandle buffer_handle, u32 offset, u32 stride) {

        Buffer *buffer = m_device->access_buffer(buffer_handle);

        VkBuffer vk_buffer = buffer->vk_buffer;
        VkDeviceSize vk_offset = offset;

        vkCmdDrawIndirect(vk_command_buffer, vk_buffer, vk_offset, 1, sizeof(VkDrawIndirectCommand));
    }

    void CommandBuffer::DrawIndexedIndirect(BufferHandle buffer_handle, u32 offset, u32 stride) {
        Buffer *buffer = m_device->access_buffer(buffer_handle);

        VkBuffer vk_buffer = buffer->vk_buffer;
        VkDeviceSize vk_offset = offset;

        vkCmdDrawIndexedIndirect(vk_command_buffer, vk_buffer, vk_offset, 1, sizeof(VkDrawIndirectCommand));
    }

    void CommandBuffer::DispatchIndirect(BufferHandle buffer_handle, u32 offset) {
        Buffer *buffer = m_device->access_buffer(buffer_handle);

        VkBuffer vk_buffer = buffer->vk_buffer;
        VkDeviceSize vk_offset = offset;

        vkCmdDispatchIndirect(vk_command_buffer, vk_buffer, vk_offset);
    }

// DrawIndirect = 0, VertexInput = 1, VertexShader = 2, FragmentShader = 3, RenderTarget = 4, ComputeShader = 5, Transfer = 6
    static ResourceState to_resource_state(PipelineStage::Enum stage) {
        static ResourceState s_states[] = {RESOURCE_STATE_INDIRECT_ARGUMENT, RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                                           RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                           RESOURCE_STATE_PIXEL_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET,
                                           RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_COPY_DEST};
        return s_states[stage];
    }

    void CommandBuffer::Barrier(const ExecutionBarrier &Barrier) {

        if (m_currentRenderPass && (m_currentRenderPass->m_type != RenderPassType::Compute)) {
            vkCmdEndRenderPass(vk_command_buffer);

            m_currentRenderPass = nullptr;
        }

        static VkImageMemoryBarrier image_Barriers[8];
        // TODO: subpass
        if (Barrier.m_newBarrierExperimental != u32_max) {

            VkPipelineStageFlags source_stage_mask = 0;
            VkPipelineStageFlags destination_stage_mask = 0;
            VkAccessFlags source_access_flags = VK_ACCESS_NONE_KHR, destination_access_flags = VK_ACCESS_NONE_KHR;

            for (u32 i = 0; i < Barrier.m_numImageBarriers; ++i) {

                Texture *texture_vulkan = m_device->access_texture(Barrier.m_imageBarriers[i].m_texture);

                VkImageMemoryBarrier &vk_Barrier = image_Barriers[i];
                const bool is_color = !TextureFormat::HasDepthOrStencil(texture_vulkan->vk_format);

                {
                    VkImageMemoryBarrier *pImageBarrier = &vk_Barrier;
                    pImageBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    pImageBarrier->pNext = NULL;

                    ResourceState current_state =
                            Barrier.m_sourcePipelineStage == PipelineStage::RenderTarget ? RESOURCE_STATE_RENDER_TARGET
                                                                                         : RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                    ResourceState next_state = Barrier.m_destinationPipelineStage == PipelineStage::RenderTarget
                                               ? RESOURCE_STATE_RENDER_TARGET : RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                    if (!is_color) {
                        current_state = Barrier.m_sourcePipelineStage == PipelineStage::RenderTarget
                                        ? RESOURCE_STATE_DEPTH_WRITE : RESOURCE_STATE_DEPTH_READ;
                        next_state = Barrier.m_destinationPipelineStage == PipelineStage::RenderTarget
                                     ? RESOURCE_STATE_DEPTH_WRITE : RESOURCE_STATE_DEPTH_READ;
                    }

                    pImageBarrier->srcAccessMask = UtilTovkAccessFlags(current_state);
                    pImageBarrier->dstAccessMask = UtilTovkAccessFlags(next_state);
                    pImageBarrier->oldLayout = UtilTovkImageLayout(current_state);
                    pImageBarrier->newLayout = UtilTovkImageLayout(next_state);

                    pImageBarrier->image = texture_vulkan->vk_image;
                    pImageBarrier->subresourceRange.aspectMask = is_color ? VK_IMAGE_ASPECT_COLOR_BIT :
                                                                 VK_IMAGE_ASPECT_DEPTH_BIT |
                                                                 VK_IMAGE_ASPECT_STENCIL_BIT;
                    pImageBarrier->subresourceRange.baseMipLevel = 0;
                    pImageBarrier->subresourceRange.levelCount = 1;
                    pImageBarrier->subresourceRange.baseArrayLayer = 0;
                    pImageBarrier->subresourceRange.layerCount = 1;

                    {
                        pImageBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        pImageBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    }

                    source_access_flags |= pImageBarrier->srcAccessMask;
                    destination_access_flags |= pImageBarrier->dstAccessMask;
                }

                vk_Barrier.oldLayout = texture_vulkan->vk_image_layout;
                texture_vulkan->vk_image_layout = vk_Barrier.newLayout;
            }

            static VkBufferMemoryBarrier buffer_memory_Barriers[8];
            for (u32 i = 0; i < Barrier.m_numMemoryBarriers; ++i) {
                VkBufferMemoryBarrier &vk_Barrier = buffer_memory_Barriers[i];
                vk_Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

                Buffer *buffer = m_device->access_buffer(Barrier.m_memoryBarriers[i].m_buffer);

                vk_Barrier.buffer = buffer->vk_buffer;
                vk_Barrier.offset = 0;
                vk_Barrier.size = buffer->m_size;

                ResourceState current_state = to_resource_state(Barrier.m_sourcePipelineStage);
                ResourceState next_state = to_resource_state(Barrier.m_destinationPipelineStage);
                vk_Barrier.srcAccessMask = UtilTovkAccessFlags(current_state);
                vk_Barrier.dstAccessMask = UtilTovkAccessFlags(next_state);

                source_access_flags |= vk_Barrier.srcAccessMask;
                destination_access_flags |= vk_Barrier.dstAccessMask;

                vk_Barrier.srcQueueFamilyIndex = 0;
                vk_Barrier.dstQueueFamilyIndex = 0;
            }

            source_stage_mask = UtilDeterminePipelineStageFlags(source_access_flags, Barrier.m_sourcePipelineStage ==
                                                                                     PipelineStage::ComputeShader
                                                                                     ? QueueType::Compute
                                                                                     : QueueType::Graphics);
            destination_stage_mask = UtilDeterminePipelineStageFlags(destination_access_flags,
                                                                     Barrier.m_destinationPipelineStage ==
                                                                     PipelineStage::ComputeShader ? QueueType::Compute
                                                                                                  : QueueType::Graphics);

            vkCmdPipelineBarrier(vk_command_buffer, source_stage_mask, destination_stage_mask, 0, 0, nullptr,
                                 Barrier.m_numMemoryBarriers, buffer_memory_Barriers, Barrier.m_numImageBarriers,
                                 image_Barriers);
            return;
        }

        VkImageLayout new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkImageLayout new_depth_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        VkAccessFlags source_access_mask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        VkAccessFlags source_buffer_access_mask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        VkAccessFlags source_depth_access_mask =
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        VkAccessFlags destination_access_mask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        VkAccessFlags destination_buffer_access_mask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        VkAccessFlags destination_depth_access_mask =
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        switch (Barrier.m_destinationPipelineStage) {

            case PipelineStage::FragmentShader: {
                //new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                break;
            }

            case PipelineStage::ComputeShader: {
                new_layout = VK_IMAGE_LAYOUT_GENERAL;


                break;
            }

            case PipelineStage::RenderTarget: {
                new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                new_depth_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                destination_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                destination_depth_access_mask =
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

                break;
            }

            case PipelineStage::DrawIndirect: {
                destination_buffer_access_mask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
                break;
            }
        }

        switch (Barrier.m_sourcePipelineStage) {

            case PipelineStage::FragmentShader: {
                //source_access_mask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                break;
            }

            case PipelineStage::ComputeShader: {

                break;
            }

            case PipelineStage::RenderTarget: {
                source_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                source_depth_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                break;
            }

            case PipelineStage::DrawIndirect: {
                source_buffer_access_mask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
                break;
            }
        }

        bool has_depth = false;

        for (u32 i = 0; i < Barrier.m_numImageBarriers; ++i) {

            Texture *texture_vulkan = m_device->access_texture(Barrier.m_imageBarriers[i].m_texture);

            VkImageMemoryBarrier &vk_Barrier = image_Barriers[i];
            vk_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

            vk_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            const bool is_color = !TextureFormat::HasDepthOrStencil(texture_vulkan->vk_format);
            has_depth = has_depth || !is_color;

            vk_Barrier.image = texture_vulkan->vk_image;
            vk_Barrier.subresourceRange.aspectMask = is_color ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT |
                                                                                            VK_IMAGE_ASPECT_STENCIL_BIT;
            vk_Barrier.subresourceRange.baseMipLevel = 0;
            vk_Barrier.subresourceRange.levelCount = 1;
            vk_Barrier.subresourceRange.baseArrayLayer = 0;
            vk_Barrier.subresourceRange.layerCount = 1;

            vk_Barrier.oldLayout = texture_vulkan->vk_image_layout;

            // Transition to...
            vk_Barrier.newLayout = is_color ? new_layout : new_depth_layout;

            vk_Barrier.srcAccessMask = is_color ? source_access_mask : source_depth_access_mask;
            vk_Barrier.dstAccessMask = is_color ? destination_access_mask : destination_depth_access_mask;

            texture_vulkan->vk_image_layout = vk_Barrier.newLayout;
        }

        VkPipelineStageFlags source_stage_mask = TovkPipelineStage((PipelineStage::Enum) Barrier.m_sourcePipelineStage);
        VkPipelineStageFlags destination_stage_mask = TovkPipelineStage(
                (PipelineStage::Enum) Barrier.m_destinationPipelineStage);

        if (has_depth) {

            source_stage_mask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            destination_stage_mask |=
                    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }

        static VkBufferMemoryBarrier buffer_memory_Barriers[8];
        for (u32 i = 0; i < Barrier.m_numMemoryBarriers; ++i) {
            VkBufferMemoryBarrier &vk_Barrier = buffer_memory_Barriers[i];
            vk_Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

            Buffer *buffer = m_device->access_buffer(Barrier.m_memoryBarriers[i].m_buffer);

            vk_Barrier.buffer = buffer->vk_buffer;
            vk_Barrier.offset = 0;
            vk_Barrier.size = buffer->m_size;
            vk_Barrier.srcAccessMask = source_buffer_access_mask;
            vk_Barrier.dstAccessMask = destination_buffer_access_mask;

            vk_Barrier.srcQueueFamilyIndex = 0;
            vk_Barrier.dstQueueFamilyIndex = 0;
        }

        vkCmdPipelineBarrier(vk_command_buffer, source_stage_mask, destination_stage_mask, 0, 0, nullptr,
                             Barrier.m_numMemoryBarriers, buffer_memory_Barriers, Barrier.m_numImageBarriers,
                             image_Barriers);
    }

    void CommandBuffer::FillBuffer(BufferHandle buffer, u32 offset, u32 size, u32 data) {
        Buffer *vk_buffer = m_device->access_buffer(buffer);

        vkCmdFillBuffer(vk_command_buffer, vk_buffer->vk_buffer, VkDeviceSize(offset),
                        size ? VkDeviceSize(size) : VkDeviceSize(vk_buffer->m_size), data);
    }

    void CommandBuffer::PushMarker(const char *name) {

        m_device->push_gpu_timestamp(this, name);

        if (!m_device->debug_utils_extension_present)
            return;

        m_device->push_marker(vk_command_buffer, name);
    }

    void CommandBuffer::PopMarker() {

        m_device->pop_gpu_timestamp(this);

        if (!m_device->debug_utils_extension_present)
            return;

        m_device->pop_marker(vk_command_buffer);
    }

#define check(result) CASSERT( result == VK_SUCCESS )

    void CommandBufferRing::Initialize(GpuDevice *gpu) {
        m_gpu = gpu;
        for (u32 i = 0; i < k_max_pools; i++) {
            VkCommandPoolCreateInfo cmd_pool_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr};
            cmd_pool_info.queueFamilyIndex = gpu->vulkan_queue_family;
            cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            check(vkCreateCommandPool(gpu->vulkan_device, &cmd_pool_info, gpu->vulkan_allocation_callbacks,
                                      &m_vulkanCommandPools[i]));
        }

        for (u32 i = 0; i < k_max_buffers; i++) {
            VkCommandBufferAllocateInfo cmd = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr};
            const u32 pool_index = PoolFromIndex(i);
            cmd.commandPool = m_vulkanCommandPools[pool_index];
            cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmd.commandBufferCount = 1;
            check(vkAllocateCommandBuffers(gpu->vulkan_device, &cmd, &m_commandBuffers[i].vk_command_buffer));

            m_commandBuffers[i].m_device = gpu;
            m_commandBuffers[i].m_handle = i;
            m_commandBuffers[i].Reset();
        }
    }

    void CommandBufferRing::Shutdown() {
        for (u32 i = 0; i < k_max_swapchain_images * k_max_threads; i++) {
            vkDestroyCommandPool(m_gpu->vulkan_device, m_vulkanCommandPools[i], m_gpu->vulkan_allocation_callbacks);
        }
    }

    void CommandBufferRing::ResetPools(u32 frameIndex) {

        for (u32 i = 0; i < k_max_threads; i++) {
            vkResetCommandPool(m_gpu->vulkan_device, m_vulkanCommandPools[frameIndex * k_max_threads + i], 0);
        }
    }

    CommandBuffer *CommandBufferRing::GetCommandBuffer(u32 frame, bool begin) {
        // TODO: take in account threads
        CommandBuffer *cb = &m_commandBuffers[frame * k_buffer_per_pool];

        if (begin) {
            cb->Reset();

            VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(cb->vk_command_buffer, &beginInfo);
        }

        return cb;
    }

    CommandBuffer *CommandBufferRing::GetCommandBufferInstant(u32 frame, bool begin) {
        CommandBuffer *cb = &m_commandBuffers[frame * k_buffer_per_pool + 1];
        return cb;
    }
}