module;

#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

export module Application.Graphics.GPUResources;

import Application.Graphics.GPUDefines;

import Foundation.Memory.Allocators.Allocator;
import Foundation.Platform;

export namespace Caustix {
    constexpr u32   k_invalid_index = 0xffffffff;

    using ResourceHandle = u32;

    struct BufferHandle {
        ResourceHandle                   m_index;
    };

    struct TextureHandle {
        ResourceHandle                   m_index;
    };

    struct ShaderStateHandle {
        ResourceHandle                   m_index;
    };

    struct SamplerHandle {
        ResourceHandle                   m_index;
    };

    struct DescriptorSetLayoutHandle {
        ResourceHandle                   m_index;
    };

    struct DescriptorSetHandle {
        ResourceHandle                   m_index;
    };

    struct PipelineHandle {
        ResourceHandle                   m_index;
    };

    struct RenderPassHandle {
        ResourceHandle                   m_index;
    };

    // Invalid handles
    constexpr BufferHandle                 k_invalid_buffer    { k_invalid_index };
    constexpr TextureHandle                k_invalid_texture   { k_invalid_index };
    constexpr ShaderStateHandle            k_invalid_shader    { k_invalid_index };
    constexpr SamplerHandle                k_invalid_sampler   { k_invalid_index };
    constexpr DescriptorSetLayoutHandle    k_invalid_layout    { k_invalid_index };
    constexpr DescriptorSetHandle          k_invalid_set       { k_invalid_index };
    constexpr PipelineHandle               k_invalid_pipeline  { k_invalid_index };
    constexpr RenderPassHandle             k_invalid_pass      { k_invalid_index };



    // Consts /////////////////////////////////////////////////////////////////
    constexpr const u8                     k_max_image_outputs = 8;                // Maximum number of images/render_targets/fbo attachments usable.
    constexpr const u8                     k_max_descriptor_set_layouts = 8;       // Maximum number of layouts in the pipeline.
    constexpr const u8                     k_max_shader_stages = 5;                // Maximum simultaneous shader stages. Applicable to all different type of pipelines.
    constexpr const u8                     k_max_descriptors_per_set = 16;         // Maximum list elements for both descriptor set layout and descriptor sets.
    constexpr const u8                     k_max_vertex_streams = 16;
    constexpr const u8                     k_max_vertex_attributes = 16;

    constexpr const u32                    k_submit_header_sentinel = 0xfefeb7ba;
    constexpr const u32                    k_max_resource_deletions = 64;

    // Resource creation structs //////////////////////////////////////////////
    
    struct Rect2D {
        f32                             m_x = 0.0f;
        f32                             m_y = 0.0f;
        f32                             m_width = 0.0f;
        f32                             m_height = 0.0f;
    };
    
    struct Rect2DInt {
        i16                             m_x = 0;
        i16                             m_y = 0;
        u16                             m_width = 0;
        u16                             m_height = 0;
    };

    struct Viewport {
        Rect2DInt                       m_rect;
        f32                             m_minDepth = 0.0f;
        f32                             m_maxDepth = 0.0f;
    };

    struct ViewportState {
        u32                             m_numViewports = 0;
        u32                             m_numScissors = 0;

        Viewport*                       m_viewport = nullptr;
        Rect2DInt*                      m_scissors = nullptr;
    };

    struct StencilOperationState {
        VkStencilOp                     m_fail = VK_STENCIL_OP_KEEP;
        VkStencilOp                     m_pass = VK_STENCIL_OP_KEEP;
        VkStencilOp                     m_depthFail = VK_STENCIL_OP_KEEP;
        VkCompareOp                     m_compare = VK_COMPARE_OP_ALWAYS;
        u32                             m_compareMask = 0xff;
        u32                             m_writeMask = 0xff;
        u32                             m_reference = 0xff;
    };

    struct DepthStencilCreation {
        StencilOperationState           m_front;
        StencilOperationState           m_back;
        VkCompareOp                     m_depthComparison = VK_COMPARE_OP_ALWAYS;

        u8                              m_depthEnable        : 1;
        u8                              m_depthWriteEnable   : 1;
        u8                              m_stencilEnable      : 1;
        u8                              m_pad                : 5;

        // Default constructor
        DepthStencilCreation() : m_depthEnable( 0 ), m_depthWriteEnable( 0 ), m_stencilEnable( 0 ) {
        }

        DepthStencilCreation&           SetDepth( bool write, VkCompareOp comparison_test );
    };

    struct BlendState {
        VkBlendFactor                   m_sourceColor        = VK_BLEND_FACTOR_ONE;
        VkBlendFactor                   m_destinationColor   = VK_BLEND_FACTOR_ONE;
        VkBlendOp                       m_colorOperation     = VK_BLEND_OP_ADD;

        VkBlendFactor                   m_sourceAlpha        = VK_BLEND_FACTOR_ONE;
        VkBlendFactor                   m_destinationAlpha   = VK_BLEND_FACTOR_ONE;
        VkBlendOp                       m_alphaOperation     = VK_BLEND_OP_ADD;

        ColorWriteEnabled::Mask         m_colorWriteMask    = ColorWriteEnabled::All_mask;

        u8                              m_blendEnabled   : 1;
        u8                              m_separateBlend  : 1;
        u8                              m_pad             : 6;


        BlendState() : m_blendEnabled( 0 ), m_separateBlend( 0 ) {
        }

        BlendState&                     SetColor( VkBlendFactor sourceColor, VkBlendFactor destinationColor, VkBlendOp colorOperation );
        BlendState&                     SetAlpha( VkBlendFactor sourceColor, VkBlendFactor destinationColor, VkBlendOp colorOperation );
        BlendState&                     SetColorWriteMask( ColorWriteEnabled::Mask value );
    };

    struct BlendStateCreation {
        BlendState                      m_blendStates[ k_max_image_outputs ];
        u32                             m_activeStates = 0;

        BlendStateCreation&             Reset();
        BlendState&                     AddBlendState();
    };

    struct RasterizationCreation {

        VkCullModeFlagBits              m_cullMode = VK_CULL_MODE_NONE;
        VkFrontFace                     m_front = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        FillMode::Enum                  m_fill = FillMode::Solid;
    };

    struct BufferCreation {
        VkBufferUsageFlags              m_typeFlags = 0;
        ResourceUsageType::Enum         m_usage   = ResourceUsageType::Immutable;
        u32                             m_size    = 0;
        void*                           m_initialData = nullptr;

        const char*                     m_name    = nullptr;

        BufferCreation&                 Reset();
        BufferCreation&                 Set( VkBufferUsageFlags flags, ResourceUsageType::Enum usage, u32 size );
        BufferCreation&                 SetData( void* data );
        BufferCreation&                 SetName( const char* name );
    };

    struct TextureCreation {
        void*                           m_initialData    = nullptr;
        u16                             m_width          = 1;
        u16                             m_height         = 1;
        u16                             m_depth          = 1;
        u8                              m_mipmaps        = 1;
        u8                              m_flags          = 0;    // TextureFlags bitmasks

        VkFormat                        m_format         = VK_FORMAT_UNDEFINED;
        TextureType::Enum               m_type           = TextureType::Texture2D;

        const char*                     m_name           = nullptr;

        TextureCreation&                SetSize( u16 width, u16 height, u16 depth );
        TextureCreation&                SetFlags( u8 mipmaps, u8 flags );
        TextureCreation&                SetFormatType( VkFormat format, TextureType::Enum type );
        TextureCreation&                SetName( const char* name );
        TextureCreation&                SetData( void* data );
    };

    struct SamplerCreation {
        VkFilter                        m_minFilter = VK_FILTER_NEAREST;
        VkFilter                        m_magFilter = VK_FILTER_NEAREST;
        VkSamplerMipmapMode             m_mipFilter = VK_SAMPLER_MIPMAP_MODE_NEAREST;

        VkSamplerAddressMode            m_addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode            m_addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode            m_addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        const char*                     m_name = nullptr;

        SamplerCreation&                SetMinMagMip( VkFilter min, VkFilter mag, VkSamplerMipmapMode mip );
        SamplerCreation&                SetAddressModeU( VkSamplerAddressMode u );
        SamplerCreation&                SetAddressModeUV( VkSamplerAddressMode u, VkSamplerAddressMode v );
        SamplerCreation&                SetAddressModeUVW( VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w );
        SamplerCreation&                SetName( const char* name );
    };

    struct ShaderStage {
        const char*                     m_code        = nullptr;
        u32                             m_codeSize    = 0;
        VkShaderStageFlagBits           m_type        = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    };

    struct ShaderStateCreation {
        ShaderStage                     m_stages[ k_max_shader_stages ];

        const char*                     m_name            = nullptr;

        u32                             m_stagesCount    = 0;
        u32                             m_spvInput       = 0;

        // Building helpers
        ShaderStateCreation&            Reset();
        ShaderStateCreation&            SetName( const char* name );
        ShaderStateCreation&            AddStage( const char* code, u32 code_size, VkShaderStageFlagBits type );
        ShaderStateCreation&            SetSpvInput( bool value );
    };

    struct DescriptorSetLayoutCreation {

        //
        // A single descriptor binding. It can be relative to one or more resources of the same type.
        //
        struct Binding {

            VkDescriptorType            m_type    = VK_DESCRIPTOR_TYPE_MAX_ENUM;
            u16                         m_start   = 0;
            u16                         m_count   = 0;
            cstring                     m_name    = nullptr;  // Comes from external memory.
        };

        Binding                         m_bindings[ k_max_descriptors_per_set ];
        u32                             m_numBindings    = 0;
        u32                             m_setIndex       = 0;

        cstring                         m_name           = nullptr;

        // Building helpers
        DescriptorSetLayoutCreation&    Reset();
        DescriptorSetLayoutCreation&    AddBinding( const Binding& binding );
        DescriptorSetLayoutCreation&    SetName( cstring name );
        DescriptorSetLayoutCreation&    SetSetIndex( u32 index );
    };

    struct DescriptorSetCreation {
        ResourceHandle                  m_resources[ k_max_descriptors_per_set ];
        SamplerHandle                   m_samplers[ k_max_descriptors_per_set ];
        u16                             m_bindings[ k_max_descriptors_per_set ];

        DescriptorSetLayoutHandle       m_layout;
        u32                             m_numResources    = 0;

        cstring                         m_name            = nullptr;

        // Building helpers
        DescriptorSetCreation&          Reset();
        DescriptorSetCreation&          SetLayout( DescriptorSetLayoutHandle layout );
        DescriptorSetCreation&          Texture( TextureHandle texture, u16 binding );
        DescriptorSetCreation&          Buffer( BufferHandle buffer, u16 binding );
        DescriptorSetCreation&          TextureSampler( TextureHandle texture, SamplerHandle sampler, u16 binding );   // TODO: separate samplers from textures
        DescriptorSetCreation&          SetName( cstring name );
    };

    struct DescriptorSetUpdate {
        DescriptorSetHandle             m_descriptorSet;

        u32                             m_frameIssued = 0;
    };

    struct VertexAttribute {
        u16                             m_location = 0;
        u16                             m_binding = 0;
        u32                             m_offset = 0;
        VertexComponentFormat::Enum     m_format = VertexComponentFormat::Count;
    };

    struct VertexStream {
        u16                             m_binding = 0;
        u16                             m_stride = 0;
        VertexInputRate::Enum           m_inputRate = VertexInputRate::Count;

    };

    struct VertexInputCreation {
        u32                             m_numVertexStreams = 0;
        u32                             m_numVertexAttributes = 0;

        VertexStream                    m_vertexStreams[ k_max_vertex_streams ];
        VertexAttribute                 m_vertexAttributes[ k_max_vertex_attributes ];

        VertexInputCreation&            Reset();
        VertexInputCreation&            AddVertexStream( const VertexStream& stream );
        VertexInputCreation&            AddVertexAttribute( const VertexAttribute& attribute );
    };

    struct RenderPassOutput {
        VkFormat                        m_colorFormats[ k_max_image_outputs ];
        VkFormat                        m_depthStencilFormat;
        u32                             m_numColorFormats;

        RenderPassOperation::Enum       m_colorOperation         = RenderPassOperation::DontCare;
        RenderPassOperation::Enum       m_depthOperation         = RenderPassOperation::DontCare;
        RenderPassOperation::Enum       m_stencilOperation       = RenderPassOperation::DontCare;

        RenderPassOutput&               Reset();
        RenderPassOutput&               Color( VkFormat format );
        RenderPassOutput&               Depth( VkFormat format );
        RenderPassOutput&               SetOperations( RenderPassOperation::Enum color, RenderPassOperation::Enum depth, RenderPassOperation::Enum stencil );
    };

    struct RenderPassCreation {

        u16                             m_numRenderTargets  = 0;
        RenderPassType::Enum            m_type              = RenderPassType::Geometry;

        TextureHandle                   m_outputTextures[ k_max_image_outputs ];
        TextureHandle                   m_depthStencilTexture;

        f32                             m_scaleX              = 1.f;
        f32                             m_scaleY              = 1.f;
        u8                              m_resize              = 1;

        RenderPassOperation::Enum       m_colorOperation         = RenderPassOperation::DontCare;
        RenderPassOperation::Enum       m_depthOperation         = RenderPassOperation::DontCare;
        RenderPassOperation::Enum       m_stencilOperation       = RenderPassOperation::DontCare;

        const char*                     m_name                = nullptr;

        RenderPassCreation&             Reset();
        RenderPassCreation&             AddRenderTexture( TextureHandle texture );
        RenderPassCreation&             SetScaling( f32 scaleX, f32 scaleY, u8 resize );
        RenderPassCreation&             SetDepthStencilTexture( TextureHandle texture );
        RenderPassCreation&             SetName( const char* name );
        RenderPassCreation&             SetType( RenderPassType::Enum type );
        RenderPassCreation&             SetOperations( RenderPassOperation::Enum color, RenderPassOperation::Enum depth, RenderPassOperation::Enum stencil );
    };

    struct PipelineCreation {
        RasterizationCreation           m_rasterization;
        DepthStencilCreation            m_depthStencil;
        BlendStateCreation              m_blendState;
        VertexInputCreation             m_vertexInput;
        ShaderStateCreation             m_shaders;

        RenderPassOutput                m_renderPass;
        DescriptorSetLayoutHandle       m_descriptorSetLayout[ k_max_descriptor_set_layouts ];
        const ViewportState*            m_viewport = nullptr;

        u32                             m_numActiveLayouts = 0;

        const char*                     m_name = nullptr;

        PipelineCreation&               AddDescriptorSetLayout( DescriptorSetLayoutHandle handle );
        RenderPassOutput&               RenderPassOutput();
    };

    // API-agnostic structs ///////////////////////////////////////////////////

    //
    // Helper methods for texture formats
    //
    namespace TextureFormat {
        inline bool                     IsDepthStencil( VkFormat value ) {
            return value == VK_FORMAT_D16_UNORM_S8_UINT || value == VK_FORMAT_D24_UNORM_S8_UINT || value == VK_FORMAT_D32_SFLOAT_S8_UINT;
        }
        inline bool                     IsDepthOnly( VkFormat value ) {
            return value >= VK_FORMAT_D16_UNORM && value < VK_FORMAT_D32_SFLOAT;
        }
        inline bool                     IsStencilOnly( VkFormat value ) {
            return value == VK_FORMAT_S8_UINT;
        }
        inline bool                     HasDepth( VkFormat value ) {
            return (value >= VK_FORMAT_D16_UNORM && value < VK_FORMAT_S8_UINT ) || (value >= VK_FORMAT_D16_UNORM_S8_UINT && value <= VK_FORMAT_D32_SFLOAT_S8_UINT);
        }
        inline bool                     HasStencil( VkFormat value ) {
            return value >= VK_FORMAT_S8_UINT && value <= VK_FORMAT_D32_SFLOAT_S8_UINT;
        }
        inline bool                     HasDepthOrStencil( VkFormat value ) {
            return value >= VK_FORMAT_D16_UNORM && value <= VK_FORMAT_D32_SFLOAT_S8_UINT;
        }
    }

    struct ResourceData {
        void* m_data = nullptr;
    };

    struct ResourceBinding {
        u16                             m_type = 0;    // ResourceType
        u16                             m_start = 0;
        u16                             m_count = 0;
        u16                             m_set = 0;

        const char*                     m_name = nullptr;
    };

    // API-agnostic descriptions //////////////////////////////////////////////

    struct ShaderStateDescription {

        void*                           m_nativeHandle = nullptr;
        cstring                         m_name         = nullptr;

    };

    struct BufferDescription {
        void*                           m_nativeHandle = nullptr;
        cstring                         m_name         = nullptr;

        VkBufferUsageFlags              m_typeFlags  = 0;
        ResourceUsageType::Enum         m_usage      = ResourceUsageType::Immutable;
        u32                             m_size       = 0;
        BufferHandle                    m_parentHandle;
    };

    struct TextureDescription {
        void*                           m_nativeHandle = nullptr;
        cstring                         m_name         = nullptr;

        u16                             m_width        = 1;
        u16                             m_height       = 1;
        u16                             m_depth        = 1;
        u8                              m_mipmaps      = 1;
        u8                              m_renderTarget  = 0;
        u8                              m_computeAccess = 0;

        VkFormat                        m_format = VK_FORMAT_UNDEFINED;
        TextureType::Enum               m_type = TextureType::Texture2D;
    };

    struct SamplerDescription {
        cstring                         m_name        = nullptr;

        VkFilter                        m_minFilter = VK_FILTER_NEAREST;
        VkFilter                        m_magFilter = VK_FILTER_NEAREST;
        VkSamplerMipmapMode             m_mipFilter = VK_SAMPLER_MIPMAP_MODE_NEAREST;

        VkSamplerAddressMode            m_addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode            m_addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode            m_addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    };

    struct DescriptorSetLayoutDescription {
        ResourceBinding                 m_bindings[ k_max_descriptors_per_set ];
        u32                             m_numActiveBindings = 0;
    };

    struct DesciptorSetDescription {
        ResourceData                    m_resources[ k_max_descriptors_per_set ];
        u32                             m_numActiveResources = 0;
    };

    struct PipelineDescription {
        ShaderStateHandle               m_shader;
    };

    // API-agnostic resource modifications ////////////////////////////////////

    struct MapBufferParameters {
        BufferHandle                    m_buffer;
        u32                             m_offset = 0;
        u32                             m_size = 0;
    };

    // Synchronization ////////////////////////////////////////////////////////

    struct ImageBarrier {
        TextureHandle                   m_texture;
    };

    struct MemoryBarrier {
        BufferHandle                    m_buffer;
    };

    struct ExecutionBarrier {

        PipelineStage::Enum             m_sourcePipelineStage;
        PipelineStage::Enum             m_destinationPipelineStage;

        u32                             m_newBarrierExperimental  = u32_max;
        u32                             m_loadOperation = 0;

        u32                             m_numImageBarriers;
        u32                             m_numMemoryBarriers;

        ImageBarrier                    m_imageBarriers[ 8 ];
        MemoryBarrier                   m_memoryBarriers[ 8 ];

        ExecutionBarrier&               Reset();
        ExecutionBarrier&               Set( PipelineStage::Enum source, PipelineStage::Enum destination );
        ExecutionBarrier&               AddImageBarrier( const ImageBarrier& image_barrier );
        ExecutionBarrier&               AddMemoryBarrier( const MemoryBarrier& memory_barrier );

    };

    struct ResourceUpdate {
        ResourceDeletionType::Enum      m_type;
        ResourceHandle                  m_handle;
        u32                             m_currentFrame;
    };

// Resources //////////////////////////////////////////////////////////////

    constexpr const u32            k_max_swapchain_images = 3;

    struct DeviceStateVulkan;

    struct Buffer {

        VkBuffer                        vk_buffer;
        VmaAllocation                   vma_allocation;
        VkDeviceMemory                  vk_device_memory;
        VkDeviceSize                    vk_device_size;

        VkBufferUsageFlags              m_typeFlags      = 0;
        ResourceUsageType::Enum         m_usage          = ResourceUsageType::Immutable;
        u32                             m_size           = 0;
        u32                             m_globalOffset   = 0;    // Offset into global constant, if dynamic

        BufferHandle                    m_handle;
        BufferHandle                    m_parentBuffer;

        const char*                     m_name   = nullptr;
    };

    struct Sampler {
        VkSampler                       vk_sampler;

        VkFilter                        m_minFilter = VK_FILTER_NEAREST;
        VkFilter                        m_magFilter = VK_FILTER_NEAREST;
        VkSamplerMipmapMode             m_mipFilter = VK_SAMPLER_MIPMAP_MODE_NEAREST;

        VkSamplerAddressMode            m_addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode            m_addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode            m_addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        const char*                     m_name    = nullptr;
    };

    struct Texture {
        VkImage                         vk_image;
        VkImageView                     vk_image_view;
        VkFormat                        vk_format;
        VkImageLayout                   vk_image_layout;
        VmaAllocation                   vma_allocation;

        u16                             m_width = 1;
        u16                             m_height = 1;
        u16                             m_depth = 1;
        u8                              m_mipmaps = 1;
        u8                              m_flags = 0;

        TextureHandle                   m_handle;
        TextureType::Enum               m_type    = TextureType::Texture2D;

        Sampler*                        m_sampler = nullptr;

        const char*                     m_name    = nullptr;
    };

    struct ShaderState {
        VkPipelineShaderStageCreateInfo m_shaderStageInfo[ k_max_shader_stages ];

        const char*                     m_name = nullptr;

        u32                             m_activeShaders = 0;
        bool                            m_graphicsPipeline = false;
    };

    struct DescriptorBinding {
        VkDescriptorType                m_type;
        u16                             m_start   = 0;
        u16                             m_count   = 0;
        u16                             m_set     = 0;

        const char*                     m_name    = nullptr;
    };

    struct DesciptorSetLayout {
        VkDescriptorSetLayout           vk_descriptor_set_layout;

        VkDescriptorSetLayoutBinding*   vk_binding      = nullptr;
        DescriptorBinding*              m_bindings        = nullptr;
        u16                             m_numBindings    = 0;
        u16                             m_setIndex       = 0;

        DescriptorSetLayoutHandle       m_handle;
    };

    struct DesciptorSet {

        VkDescriptorSet                 vk_descriptor_set;

        ResourceHandle*                 m_resources       = nullptr;
        SamplerHandle*                  m_samplers        = nullptr;
        u16*                            m_bindings        = nullptr;

        const DesciptorSetLayout*       m_layout          = nullptr;
        u32                             m_numResources   = 0;
    };

    struct Pipeline {

        VkPipeline                      vk_pipeline;
        VkPipelineLayout                vk_pipeline_layout;

        VkPipelineBindPoint             vk_bind_point;

        ShaderStateHandle               m_shaderState;

        const DesciptorSetLayout*       m_descriptorSetLayout[ k_max_descriptor_set_layouts ];
        DescriptorSetLayoutHandle       m_descriptorSetLayoutHandle[ k_max_descriptor_set_layouts ];
        u32                             m_numActiveLayouts = 0;

        DepthStencilCreation            m_depthStencil;
        BlendStateCreation              m_blendState;
        RasterizationCreation           m_rasterization;

        PipelineHandle                  m_handle;
        bool                            m_graphicsPipeline = true;

    };

    struct RenderPass {

        VkRenderPass                    vk_render_pass;
        VkFramebuffer                   vk_frame_buffer;

        RenderPassOutput                m_output;

        TextureHandle                   m_outputTextures[ k_max_image_outputs ];
        TextureHandle                   m_outputDepth;

        RenderPassType::Enum            m_type;

        f32                             m_scaleX      = 1.f;
        f32                             m_scaleY      = 1.f;
        u16                             m_width       = 0;
        u16                             m_height      = 0;
        u16                             m_dispatchX   = 0;
        u16                             m_dispatchY   = 0;
        u16                             m_dispatchZ   = 0;

        u8                              m_resize      = 0;
        u8                              m_numRenderTargets = 0;

        const char*                     name        = nullptr;
    };

    // Enum translations. Use tables or switches depending on the case. ///////
    consteval cstring ToCompilerExtension( VkShaderStageFlagBits value ) {
        switch ( value ) {
            case VK_SHADER_STAGE_VERTEX_BIT:
                return "vert";
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                return "frag";
            case VK_SHADER_STAGE_COMPUTE_BIT:
                return "comp";
            default:
                return "";
        }
    }

    consteval cstring ToStageDefines( VkShaderStageFlagBits value ) {
        switch ( value ) {
            case VK_SHADER_STAGE_VERTEX_BIT:
                return "VERTEX";
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                return "FRAGMENT";
            case VK_SHADER_STAGE_COMPUTE_BIT:
                return "COMPUTE";
            default:
                return "";
        }
    }

    consteval VkImageType TovkImageType( TextureType::Enum type ) {
        constexpr VkImageType s_vk_target[ TextureType::Count ] = { VK_IMAGE_TYPE_1D, VK_IMAGE_TYPE_2D, VK_IMAGE_TYPE_3D, VK_IMAGE_TYPE_1D, VK_IMAGE_TYPE_2D, VK_IMAGE_TYPE_3D };
        return s_vk_target[ type ];
    }

    consteval VkImageViewType TovkImageViewType( TextureType::Enum type ) {
        constexpr VkImageViewType s_vk_data[] = { VK_IMAGE_VIEW_TYPE_1D, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_VIEW_TYPE_1D_ARRAY, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_VIEW_TYPE_CUBE_ARRAY };
        return s_vk_data[ type ];
    }

    consteval VkFormat TovkVertexFormat( VertexComponentFormat::Enum value ) {
        // Float, Float2, Float3, Float4, Mat4, Byte, Byte4N, UByte, UByte4N, Short2, Short2N, Short4, Short4N, Uint, Uint2, Uint4, Count
        constexpr VkFormat s_vk_vertex_formats[ VertexComponentFormat::Count ] = { VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, /*MAT4 TODO*/VK_FORMAT_R32G32B32A32_SFLOAT,
                                                                                VK_FORMAT_R8_SINT, VK_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R8_UINT, VK_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R16G16_SINT, VK_FORMAT_R16G16_SNORM,
                                                                                VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R32_UINT, VK_FORMAT_R32G32_UINT, VK_FORMAT_R32G32B32A32_UINT };

        return s_vk_vertex_formats[ value ];
    }

    consteval VkPipelineStageFlags TovkPipelineStage( PipelineStage::Enum value ) {
        constexpr VkPipelineStageFlags s_vk_values[] = { VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
        return s_vk_values[ value ];
    }

    consteval VkAccessFlags UtilTovkAccessFlags( ResourceState state ) {
        VkAccessFlags ret = 0;
        if ( state & RESOURCE_STATE_COPY_SOURCE ) {
            ret |= VK_ACCESS_TRANSFER_READ_BIT;
        }
        if ( state & RESOURCE_STATE_COPY_DEST ) {
            ret |= VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        if ( state & RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER ) {
            ret |= VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        }
        if ( state & RESOURCE_STATE_INDEX_BUFFER ) {
            ret |= VK_ACCESS_INDEX_READ_BIT;
        }
        if ( state & RESOURCE_STATE_UNORDERED_ACCESS ) {
            ret |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        }
        if ( state & RESOURCE_STATE_INDIRECT_ARGUMENT ) {
            ret |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        }
        if ( state & RESOURCE_STATE_RENDER_TARGET ) {
            ret |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }
        if ( state & RESOURCE_STATE_DEPTH_WRITE ) {
            ret |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        }
        if ( state & RESOURCE_STATE_SHADER_RESOURCE ) {
            ret |= VK_ACCESS_SHADER_READ_BIT;
        }
        if ( state & RESOURCE_STATE_PRESENT ) {
            ret |= VK_ACCESS_MEMORY_READ_BIT;
        }
#ifdef ENABLE_RAYTRACING
        if ( state & RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE ) {
        ret |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;
    }
#endif

        return ret;
    }

    consteval VkImageLayout UtilTovkImageLayout( ResourceState usage ) {
        if ( usage & RESOURCE_STATE_COPY_SOURCE )
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        if ( usage & RESOURCE_STATE_COPY_DEST )
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        if ( usage & RESOURCE_STATE_RENDER_TARGET )
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        if ( usage & RESOURCE_STATE_DEPTH_WRITE )
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        if ( usage & RESOURCE_STATE_DEPTH_READ )
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        if ( usage & RESOURCE_STATE_UNORDERED_ACCESS )
            return VK_IMAGE_LAYOUT_GENERAL;

        if ( usage & RESOURCE_STATE_SHADER_RESOURCE )
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if ( usage & RESOURCE_STATE_PRESENT )
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        if ( usage == RESOURCE_STATE_COMMON )
            return VK_IMAGE_LAYOUT_GENERAL;

        return VK_IMAGE_LAYOUT_UNDEFINED;
    }

    // Determines pipeline stages involved for given accesses
    consteval VkPipelineStageFlags UtilDeterminePipelineStageFlags( VkAccessFlags accessFlags, QueueType::Enum queueType ) {
        VkPipelineStageFlags flags = 0;

        switch ( queueType ) {
            case QueueType::Graphics:
            {
                if ( ( accessFlags & ( VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT ) ) != 0 )
                    flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

                if ( ( accessFlags & ( VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT ) ) != 0 ) {
                    flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
                    flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    /*if ( pRenderer->pActiveGpuSettings->mGeometryShaderSupported ) {
                        flags |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
                    }
                    if ( pRenderer->pActiveGpuSettings->mTessellationSupported ) {
                        flags |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
                        flags |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
                    }*/
                    flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
#ifdef ENABLE_RAYTRACING
                    if ( pRenderer->mVulkan.mRaytracingExtension ) {
                    flags |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV;
                }
#endif
                }

                if ( ( accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT ) != 0 )
                    flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

                if ( ( accessFlags & ( VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT ) ) != 0 )
                    flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

                if ( ( accessFlags & ( VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT ) ) != 0 )
                    flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

                break;
            }
            case QueueType::Compute:
            {
                if ( ( accessFlags & ( VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT ) ) != 0 ||
                     ( accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT ) != 0 ||
                     ( accessFlags & ( VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT ) ) != 0 ||
                     ( accessFlags & ( VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT ) ) != 0 )
                    return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

                if ( ( accessFlags & ( VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT ) ) != 0 )
                    flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

                break;
            }
            case QueueType::CopyTransfer : return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            default: break;
        }

        // Compatible with both compute and graphics queues
        if ( ( accessFlags & VK_ACCESS_INDIRECT_COMMAND_READ_BIT ) != 0 )
            flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;

        if ( ( accessFlags & ( VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT ) ) != 0 )
            flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;

        if ( ( accessFlags & ( VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT ) ) != 0 )
            flags |= VK_PIPELINE_STAGE_HOST_BIT;

        if ( flags == 0 )
            flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        return flags;
    }
}

namespace Caustix {

    // DepthStencilCreation ////////////////////////////////////
    DepthStencilCreation& DepthStencilCreation::SetDepth( bool write, VkCompareOp comparison_test ) {
        m_depthWriteEnable = write;
        m_depthComparison = comparison_test;
        // Setting depth like this means we want to use the depth test.
        m_depthEnable = 1;

        return *this;
    }

    // BlendState  /////////////////////////////////////////////
    BlendState& BlendState::SetColor( VkBlendFactor source, VkBlendFactor destination, VkBlendOp operation ) {
        m_sourceColor = source;
        m_destinationColor = destination;
        m_colorOperation = operation;
        m_blendEnabled = 1;

        return *this;
    }

    BlendState& BlendState::SetAlpha( VkBlendFactor source, VkBlendFactor destination, VkBlendOp operation ) {
        m_sourceAlpha = source;
        m_destinationAlpha = destination;
        m_alphaOperation = operation;
        m_separateBlend = 1;

        return *this;
    }

    BlendState& BlendState::SetColorWriteMask( ColorWriteEnabled::Mask value ) {
        m_colorWriteMask = value;

        return *this;
    }

// BlendStateCreation //////////////////////////////////////
    BlendStateCreation& BlendStateCreation::Reset() {
        m_activeStates = 0;

        return *this;
    }

    BlendState& BlendStateCreation::AddBlendState() {
        return m_blendStates[m_activeStates++];
    }

// BufferCreation //////////////////////////////////////////
    BufferCreation& BufferCreation::Reset() {
        m_size = 0;
        m_initialData = nullptr;

        return *this;
    }

    BufferCreation& BufferCreation::Set( VkBufferUsageFlags flags, ResourceUsageType::Enum usage_, u32 size_ ) {
        m_typeFlags = flags;
        m_usage = usage_;
        m_size = size_;

        return *this;
    }

    BufferCreation& BufferCreation::SetData( void* data_ ) {
        m_initialData = data_;

        return *this;
    }

    BufferCreation& BufferCreation::SetName( const char* name_ ) {
        m_name = name_;

        return *this;
    }

// TextureCreation /////////////////////////////////////////
    TextureCreation& TextureCreation::SetSize( u16 width_, u16 height_, u16 depth_ ) {
        m_width = width_;
        m_height = height_;
        m_depth = depth_;

        return *this;
    }

    TextureCreation& TextureCreation::SetFlags( u8 mipmaps_, u8 flags_ ) {
        m_mipmaps = mipmaps_;
        m_flags = flags_;

        return *this;
    }

    TextureCreation& TextureCreation::SetFormatType( VkFormat format_, TextureType::Enum type_ ) {
        m_format = format_;
        m_type = type_;

        return *this;
    }

    TextureCreation& TextureCreation::SetName( const char* name_ ) {
        m_name = name_;

        return *this;
    }

    TextureCreation& TextureCreation::SetData( void* data_ ) {
        m_initialData = data_;

        return *this;
    }

// SamplerCreation /////////////////////////////////////////
    SamplerCreation& SamplerCreation::SetMinMagMip( VkFilter min, VkFilter mag, VkSamplerMipmapMode mip ) {
        m_minFilter = min;
        m_magFilter = mag;
        m_mipFilter = mip;

        return *this;
    }

    SamplerCreation& SamplerCreation::SetAddressModeU( VkSamplerAddressMode u ) {
        m_addressModeU = u;

        return *this;
    }

    SamplerCreation& SamplerCreation::SetAddressModeUV( VkSamplerAddressMode u, VkSamplerAddressMode v ) {
        m_addressModeU = u;
        m_addressModeV = v;

        return *this;
    }

    SamplerCreation& SamplerCreation::SetAddressModeUVW( VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w ) {
        m_addressModeU = u;
        m_addressModeV = v;
        m_addressModeW = w;

        return *this;
    }

    SamplerCreation& SamplerCreation::SetName( const char* name_ ) {
        m_name = name_;

        return *this;
    }


// ShaderStateCreation /////////////////////////////////////
    ShaderStateCreation& ShaderStateCreation::Reset() {
        m_stagesCount = 0;

        return *this;
    }

    ShaderStateCreation& ShaderStateCreation::SetName( const char* name_ ) {
        m_name = name_;

        return *this;
    }

    ShaderStateCreation& ShaderStateCreation::AddStage( const char* code, u32 code_size, VkShaderStageFlagBits type ) {
        m_stages[m_stagesCount].m_code = code;
        m_stages[m_stagesCount].m_codeSize = code_size;
        m_stages[m_stagesCount].m_type = type;
        ++m_stagesCount;

        return *this;
    }

    ShaderStateCreation& ShaderStateCreation::SetSpvInput( bool value ) {
        m_spvInput = value;
        return *this;
    }

// DescriptorSetLayoutCreation ////////////////////////////////////////////
    DescriptorSetLayoutCreation& DescriptorSetLayoutCreation::Reset() {
        m_numBindings = 0;
        m_setIndex = 0;
        return *this;
    }

    DescriptorSetLayoutCreation& DescriptorSetLayoutCreation::AddBinding( const Binding& binding ) {
        m_bindings[m_numBindings++] = binding;
        return *this;
    }

    DescriptorSetLayoutCreation& DescriptorSetLayoutCreation::SetName( cstring name_ ) {
        m_name = name_;
        return *this;
    }


    DescriptorSetLayoutCreation& DescriptorSetLayoutCreation::SetSetIndex( u32 index ) {
        m_setIndex =  index;
        return *this;
    }

    // DescriptorSetCreation //////////////////////////////////////////////////
    DescriptorSetCreation& DescriptorSetCreation::Reset() {
        m_numResources = 0;
        return *this;
    }

    DescriptorSetCreation& DescriptorSetCreation::SetLayout( DescriptorSetLayoutHandle layout_ ) {
        m_layout = layout_;
        return *this;
    }

    DescriptorSetCreation& DescriptorSetCreation::Texture( TextureHandle texture, u16 binding ) {
        // Set a default sampler
        m_samplers[ m_numResources ] = k_invalid_sampler;
        m_bindings[ m_numResources ] = binding;
        m_resources[ m_numResources++ ] = texture. m_index;
        return *this;
    }

    DescriptorSetCreation& DescriptorSetCreation::Buffer( BufferHandle buffer, u16 binding ) {
        m_samplers[ m_numResources ] = k_invalid_sampler;
        m_bindings[ m_numResources ] = binding;
        m_resources[ m_numResources++ ] = buffer. m_index;
        return *this;
    }

    DescriptorSetCreation& DescriptorSetCreation::TextureSampler( TextureHandle texture, SamplerHandle sampler, u16 binding ) {
        m_bindings[ m_numResources ] = binding;
        m_resources[ m_numResources ] = texture. m_index;
        m_samplers[ m_numResources++ ] = sampler;
        return *this;
    }

    DescriptorSetCreation& DescriptorSetCreation::SetName( cstring name_ ) {
        m_name = name_;
        return *this;
    }

// VertexInputCreation /////////////////////////////////////
    VertexInputCreation& VertexInputCreation::Reset() {
        m_numVertexStreams = m_numVertexAttributes = 0;
        return *this;
    }

    VertexInputCreation& VertexInputCreation::AddVertexStream( const VertexStream& stream ) {
        m_vertexStreams[m_numVertexStreams++] = stream;
        return *this;
    }

    VertexInputCreation& VertexInputCreation::AddVertexAttribute( const VertexAttribute& attribute ) {
        m_vertexAttributes[m_numVertexAttributes++] = attribute;
        return *this;
    }

// RenderPassOutput ////////////////////////////////////////
    RenderPassOutput& RenderPassOutput::Reset() {
        m_numColorFormats = 0;
        for ( u32 i = 0; i < k_max_image_outputs; ++i) {
            m_colorFormats[i] = VK_FORMAT_UNDEFINED;
        }
        m_depthStencilFormat = VK_FORMAT_UNDEFINED;
        m_colorOperation = m_depthOperation = m_stencilOperation = RenderPassOperation::DontCare;
        return *this;
    }

    RenderPassOutput& RenderPassOutput::Color( VkFormat format ) {
        m_colorFormats[ m_numColorFormats++ ] = format;
        return *this;
    }

    RenderPassOutput& RenderPassOutput::Depth( VkFormat format ) {
        m_depthStencilFormat = format;
        return *this;
    }

    RenderPassOutput& RenderPassOutput::SetOperations( RenderPassOperation::Enum color_, RenderPassOperation::Enum depth_, RenderPassOperation::Enum stencil_ ) {
        m_colorOperation = color_;
        m_depthOperation = depth_;
        m_stencilOperation = stencil_;

        return *this;
    }

// PipelineCreation ////////////////////////////////////////
    PipelineCreation& PipelineCreation::AddDescriptorSetLayout( DescriptorSetLayoutHandle handle ) {
        m_descriptorSetLayout[m_numActiveLayouts++] = handle;
        return *this;
    }

    RenderPassOutput& PipelineCreation::RenderPassOutput() {
        return m_renderPass;
    }

// RenderPassCreation //////////////////////////////////////
    RenderPassCreation& RenderPassCreation::Reset() {
        m_numRenderTargets = 0;
        m_depthStencilTexture = k_invalid_texture;
        m_resize = 0;
        m_scaleX = 1.f;
        m_scaleY = 1.f;
        m_colorOperation = m_depthOperation = m_stencilOperation = RenderPassOperation::DontCare;

        return *this;
    }

    RenderPassCreation& RenderPassCreation::AddRenderTexture( TextureHandle texture ) {
        m_outputTextures[m_numRenderTargets++] = texture;

        return *this;
    }

    RenderPassCreation& RenderPassCreation::SetScaling( f32 scaleX_, f32 scaleY_, u8 resize_ ) {
        m_scaleX = scaleX_;
        m_scaleY = scaleY_;
        m_resize = resize_;

        return *this;
    }

    RenderPassCreation& RenderPassCreation::SetDepthStencilTexture( TextureHandle texture ) {
        m_depthStencilTexture = texture;

        return *this;
    }

    RenderPassCreation& RenderPassCreation::SetName( const char* name_ ) {
        m_name = name_;

        return *this;
    }

    RenderPassCreation& RenderPassCreation::SetType( RenderPassType::Enum type_ ) {
        m_type = type_;

        return *this;
    }

    RenderPassCreation& RenderPassCreation::SetOperations( RenderPassOperation::Enum color_, RenderPassOperation::Enum depth_, RenderPassOperation::Enum stencil_ ) {
        m_colorOperation = color_;
        m_depthOperation = depth_;
        m_stencilOperation = stencil_;

        return *this;
    }

// ExecutionBarrier ////////////////////////////////////////
    ExecutionBarrier& ExecutionBarrier::Reset() {
        m_numImageBarriers = m_numMemoryBarriers = 0;
        m_sourcePipelineStage = PipelineStage::DrawIndirect;
        m_destinationPipelineStage = PipelineStage::DrawIndirect;
        return *this;
    }

    ExecutionBarrier& ExecutionBarrier::Set( PipelineStage::Enum source, PipelineStage::Enum destination ) {
        m_sourcePipelineStage = source;
        m_destinationPipelineStage = destination;

        return *this;
    }

    ExecutionBarrier& ExecutionBarrier::AddImageBarrier( const ImageBarrier& image_barrier ) {
        m_imageBarriers[m_numImageBarriers++] = image_barrier;

        return *this;
    }

    ExecutionBarrier& ExecutionBarrier::AddMemoryBarrier( const MemoryBarrier& memory_barrier ) {
        m_memoryBarriers[ m_numMemoryBarriers++ ] = memory_barrier;

        return *this;
    }
}