module;

#include <memory>
#include <unordered_map>

#include <imgui.h>
#include <imgui_impl_sdl2.h>

#include <vulkan/vulkan.h>

export module Application.Graphics.ImGuiService;

import Application.Graphics.GPUDevice;
import Application.Graphics.CommandBuffer;

import Foundation.Memory.Allocators.Allocator;
import Foundation.Services.Service;
import Foundation.Services.MemoryService;
import Foundation.Services.ServiceManager;
import Foundation.Platform;
import Foundation.Log;

export namespace Caustix {

    enum ImGuiStyles {
        Default = 0,
        GreenBlue,
        DarkRed,
        DarkGold
    };

    struct ImGuiServiceConfiguration {
        GpuDevice*                      m_gpu;
        void*                           m_windowHandle;
    };

    #define FlatHashMap(key, value) std::unordered_map<key, value, std::hash<key>, std::equal_to<key>, STLAdaptor<std::pair<const key, value>>>

    struct ImGuiService : public Service {

        ImGuiService( const ImGuiServiceConfiguration& configuration );
        void                            Shutdown();

        void                            NewFrame();
        void                            Render( CommandBuffer& commands );

        // Removes the Texture from the Cache and destroy the associated Descriptor Set.
        void                            RemoveCachedTexture( TextureHandle& texture );

        void                            SetStyle( ImGuiStyles style );

        GpuDevice*                      m_gpu;

        static ImGuiService* Create(const ImGuiServiceConfiguration& configuration) {
            static std::unique_ptr<ImGuiService> instance{new ImGuiService(configuration)};
            return instance.get();
        }

        static constexpr cstring        m_name = "caustix_imgui_service";

    private:
        FlatHashMap(ResourceHandle, ResourceHandle) m_textureToDescriptorSet;
    };
}

namespace Caustix {
    // Graphics Data
    TextureHandle g_font_texture;
    PipelineHandle g_imgui_pipeline;
    BufferHandle g_vb, g_ib;
    BufferHandle g_ui_cb;
    DescriptorSetLayoutHandle g_descriptor_set_layout;
    DescriptorSetHandle g_ui_descriptor_set;  // Font descriptor set

    static uint32_t g_vb_size = 665536, g_ib_size = 665536;

    static const char* g_vertex_shader_code = {
            "#version 450\n"
            "layout( location = 0 ) in vec2 Position;\n"
            "layout( location = 1 ) in vec2 UV;\n"
            "layout( location = 2 ) in uvec4 Color;\n"
            "layout( location = 0 ) out vec2 Frag_UV;\n"
            "layout( location = 1 ) out vec4 Frag_Color;\n"
            "layout( std140, binding = 0 ) uniform LocalConstants { mat4 ProjMtx; };\n"
            "void main()\n"
            "{\n"
            "    Frag_UV = UV;\n"
            "    Frag_Color = Color / 255.0f;\n"
            "    gl_Position = ProjMtx * vec4( Position.xy,0,1 );\n"
            "}\n"
    };

    static const char* g_vertex_shader_code_bindless = {
            "#version 450\n"
            "layout( location = 0 ) in vec2 Position;\n"
            "layout( location = 1 ) in vec2 UV;\n"
            "layout( location = 2 ) in uvec4 Color;\n"
            "layout( location = 0 ) out vec2 Frag_UV;\n"
            "layout( location = 1 ) out vec4 Frag_Color;\n"
            "layout (location = 2) flat out uint texture_id;\n"
            "layout( std140, binding = 0 ) uniform LocalConstants { mat4 ProjMtx; };\n"
            "void main()\n"
            "{\n"
            "    Frag_UV = UV;\n"
            "    Frag_Color = Color / 255.0f;\n"
            "    texture_id = gl_InstanceIndex;\n"
            "    gl_Position = ProjMtx * vec4( Position.xy,0,1 );\n"
            "}\n"
    };

    static const char* g_fragment_shader_code = {
            "#version 450\n"
            "#extension GL_EXT_nonuniform_qualifier : enable\n"
            "layout (location = 0) in vec2 Frag_UV;\n"
            "layout (location = 1) in vec4 Frag_Color;\n"
            "layout (location = 0) out vec4 Out_Color;\n"
            "layout (binding = 1) uniform sampler2D Texture;\n"
            "void main()\n"
            "{\n"
            "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
            "}\n"
    };

    static const char* g_fragment_shader_code_bindless = {
            "#version 450\n"
            "#extension GL_EXT_nonuniform_qualifier : enable\n"
            "layout (location = 0) in vec2 Frag_UV;\n"
            "layout (location = 1) in vec4 Frag_Color;\n"
            "layout (location = 2) flat in uint texture_id;\n"
            "layout (location = 0) out vec4 Out_Color;\n"
            "#extension GL_EXT_nonuniform_qualifier : enable\n"
            "layout (set = 1, binding = 10) uniform sampler2D textures[];\n"
            "void main()\n"
            "{\n"
            "    Out_Color = Frag_Color * texture(textures[nonuniformEXT(texture_id)], Frag_UV.st);\n"
            "}\n"
    };

    ImGuiService::ImGuiService( const ImGuiServiceConfiguration& configuration )
    : m_gpu(configuration.m_gpu)
    , m_textureToDescriptorSet(ServiceManager::GetInstance()->Get<MemoryService>()->m_systemAllocator){

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer bindings
        ImGui_ImplSDL2_InitForVulkan( (SDL_Window*)configuration.m_windowHandle );

        ImGuiIO& io = ImGui::GetIO();
        io.BackendRendererName = "Caustix_ImGui";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

        // Load font texture atlas //////////////////////////////////////////////////
        unsigned char* pixels;
        int width, height;
        // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be
        // compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id,
        // consider calling GetTexDataAsAlpha8() instead to save on GPU memory.
        io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height );

        TextureCreation texture_creation;// = { pixels, ( u16 )width, ( u16 )height, 1, 1, 0, TextureFormat::R8G8B8A8_UNORM, TextureType::Texture2D };
        texture_creation.SetFormatType( VK_FORMAT_R8G8B8A8_UNORM, TextureType::Texture2D ).SetData( pixels ).SetSize( width, height, 1 ).SetFlags( 1, 0 ).SetName( "ImGui_Font" );
        g_font_texture = m_gpu->create_texture( texture_creation );

        // Store our identifier
        io.Fonts->TexID = (ImTextureID)&g_font_texture;

        // Manual code. Used to remove dependency from that.
        ShaderStateCreation shader_creation{};

        if ( m_gpu->bindless_supported ) {
            shader_creation.SetName( "ImGui" ).AddStage( g_vertex_shader_code_bindless, ( uint32_t )strlen( g_vertex_shader_code_bindless ), VK_SHADER_STAGE_VERTEX_BIT )
                    .AddStage( g_fragment_shader_code_bindless, ( uint32_t )strlen( g_fragment_shader_code_bindless ), VK_SHADER_STAGE_FRAGMENT_BIT );
        }
        else {
            shader_creation.SetName( "ImGui" ).AddStage( g_vertex_shader_code, ( uint32_t )strlen( g_vertex_shader_code ), VK_SHADER_STAGE_VERTEX_BIT )
                    .AddStage( g_fragment_shader_code, ( uint32_t )strlen( g_fragment_shader_code ), VK_SHADER_STAGE_FRAGMENT_BIT );
        }


        PipelineCreation pipeline_creation = {};
        pipeline_creation.m_name = "Pipeline_ImGui";
        pipeline_creation.m_shaders = shader_creation;

        pipeline_creation.m_blendState.AddBlendState().SetColor( VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD );

        pipeline_creation.m_vertexInput.AddVertexAttribute( { 0, 0, 0, VertexComponentFormat::Float2 } )
                .AddVertexAttribute( { 1, 0, 8, VertexComponentFormat::Float2 } )
                .AddVertexAttribute( { 2, 0, 16, VertexComponentFormat::UByte4N } );

        pipeline_creation.m_vertexInput.AddVertexStream( { 0, 20, VertexInputRate::PerVertex } );
        pipeline_creation.m_renderPass = m_gpu->get_swapchain_output();

        DescriptorSetLayoutCreation descriptor_set_layout_creation{};
        if ( m_gpu->bindless_supported ) {
            descriptor_set_layout_creation.AddBinding( { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, "LocalConstants" } ).AddBinding( { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10, 1, "Texture" } ).SetName( "RLL_ImGui" );
        }
        else {
            descriptor_set_layout_creation.AddBinding( { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, "LocalConstants" } ).SetName( "RLL_ImGui" );
            descriptor_set_layout_creation.AddBinding( { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, 1, "LocalConstants" } ).SetName( "RLL_ImGui" );
        }


        g_descriptor_set_layout = m_gpu->create_descriptor_set_layout( descriptor_set_layout_creation );

        pipeline_creation.AddDescriptorSetLayout( g_descriptor_set_layout );

        g_imgui_pipeline = m_gpu->create_pipeline( pipeline_creation );

        // Create constant buffer
        BufferCreation cb_creation;
        cb_creation.Set( VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, ResourceUsageType::Dynamic, 64 ).SetName( "CB_ImGui" );
        g_ui_cb = m_gpu->create_buffer( cb_creation );

        // Create descriptor set
        DescriptorSetCreation ds_creation{};
        if ( !m_gpu->bindless_supported ) {
            ds_creation.SetLayout( pipeline_creation.m_descriptorSetLayout[0] ).Buffer( g_ui_cb, 0 ).Texture( g_font_texture, 1 ).SetName( "RL_ImGui" );
        } else {
            ds_creation.SetLayout( pipeline_creation.m_descriptorSetLayout[0] ).Buffer( g_ui_cb, 0 ).SetName( "RL_ImGui" );
        }
        g_ui_descriptor_set = m_gpu->create_descriptor_set( ds_creation );

        // Add descriptor set to the map
        // Old Map
        m_textureToDescriptorSet.reserve( 4 );
        m_textureToDescriptorSet.emplace( g_font_texture.m_index, g_ui_descriptor_set.m_index );

        // Create vertex and index buffers //////////////////////////////////////////
        BufferCreation vb_creation;
        vb_creation.Set( VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, ResourceUsageType::Dynamic, g_vb_size ).SetName( "VB_ImGui" );
        g_vb = m_gpu->create_buffer( vb_creation );

        BufferCreation ib_creation;
        ib_creation.Set( VK_BUFFER_USAGE_INDEX_BUFFER_BIT, ResourceUsageType::Dynamic, g_ib_size ).SetName( "IB_ImGui" );
        g_ib = m_gpu->create_buffer( ib_creation );
    }

    void ImGuiService::Shutdown() {

        for (auto it = m_textureToDescriptorSet.begin(); it != m_textureToDescriptorSet.end(); it++) {
            ResourceHandle handle = it->second;
            m_gpu->destroy_descriptor_set( { handle } );
        }

        m_textureToDescriptorSet.clear();

        m_gpu->destroy_buffer( g_vb );
        m_gpu->destroy_buffer( g_ib );
        m_gpu->destroy_buffer( g_ui_cb );
        m_gpu->destroy_descriptor_set_layout( g_descriptor_set_layout );

        m_gpu->destroy_pipeline( g_imgui_pipeline );
        m_gpu->destroy_texture( g_font_texture );


        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    void ImGuiService::NewFrame() {
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
    }
    void ImGuiService::Render( CommandBuffer& commands ) {

        ImGui::Render();

        ImDrawData* draw_data = ImGui::GetDrawData();

        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fb_width = (int)( draw_data->DisplaySize.x * draw_data->FramebufferScale.x );
        int fb_height = (int)( draw_data->DisplaySize.y * draw_data->FramebufferScale.y );
        if ( fb_width <= 0 || fb_height <= 0 )
            return;

        // Vulkan backend has a different origin than OpenGL.
        bool clip_origin_lower_left = false;

#if defined(GL_CLIP_ORIGIN) && !defined(__APPLE__)
        GLenum last_clip_origin = 0; glGetIntegerv( GL_CLIP_ORIGIN, (GLint*)&last_clip_origin ); // Support for GL 4.5's glClipControl(GL_UPPER_LEFT)
    if ( last_clip_origin == GL_UPPER_LEFT )
        clip_origin_lower_left = false;
#endif
        size_t vertex_size = draw_data->TotalVtxCount * sizeof( ImDrawVert );
        size_t index_size = draw_data->TotalIdxCount * sizeof( ImDrawIdx );

        if ( vertex_size >= g_vb_size || index_size >= g_ib_size ) {
            error( "ImGui Backend Error: vertex/index overflow!" );
            return;
        }

        if ( vertex_size == 0 && index_size == 0 ) {
            return;
        }

        // Upload data
        ImDrawVert* vtx_dst = NULL;
        ImDrawIdx* idx_dst = NULL;

        MapBufferParameters map_parameters_vb = { g_vb, 0, (u32)vertex_size };
        vtx_dst = (ImDrawVert*)m_gpu->map_buffer( map_parameters_vb );

        if ( vtx_dst ) {
            for ( int n = 0; n < draw_data->CmdListsCount; n++ ) {

                const ImDrawList* cmd_list = draw_data->CmdLists[n];
                memcpy( vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof( ImDrawVert ) );
                vtx_dst += cmd_list->VtxBuffer.Size;
            }

            m_gpu->unmap_buffer( map_parameters_vb );
        }

        MapBufferParameters map_parameters_ib = { g_ib, 0, (u32)index_size };
        idx_dst = (ImDrawIdx*)m_gpu->map_buffer( map_parameters_ib );

        if ( idx_dst ) {
            for ( int n = 0; n < draw_data->CmdListsCount; n++ ) {

                const ImDrawList* cmd_list = draw_data->CmdLists[n];
                memcpy( idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof( ImDrawIdx ) );
                idx_dst += cmd_list->IdxBuffer.Size;
            }

            m_gpu->unmap_buffer( map_parameters_ib );
        }

        // TODO_KS: Add the sorting.
        commands.PushMarker( "ImGUI" );

        // todo: key
        commands.BindPass( m_gpu->get_swapchain_pass() );
        commands.BindPipeline( g_imgui_pipeline );
        commands.BindVertexBuffer( g_vb, 0, 0 );
        commands.BindIndexBuffer( g_ib, 0, VK_INDEX_TYPE_UINT16 );

        const Viewport viewport = { 0, 0, (u16)fb_width, (u16)fb_height, 0.0f, 1.0f };
        commands.SetViewport( &viewport );

        // Setup viewport, orthographic projection matrix
        // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayMin is typically (0,0) for single viewport apps.
        float L = draw_data->DisplayPos.x;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        float T = draw_data->DisplayPos.y;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        const float ortho_projection[4][4] =
                {
                        { 2.0f / ( R - L ),   0.0f,         0.0f,   0.0f },
                        { 0.0f,         2.0f / ( T - B ),   0.0f,   0.0f },
                        { 0.0f,         0.0f,        -1.0f,   0.0f },
                        { ( R + L ) / ( L - R ),  ( T + B ) / ( B - T ),  0.0f,   1.0f },
                };

        MapBufferParameters cb_map = { g_ui_cb, 0, 0 };
        float* cb_data = (float*)m_gpu->map_buffer( cb_map );
        if ( cb_data ) {
            memcpy( cb_data, &ortho_projection[0][0], 64 );
            m_gpu->unmap_buffer( cb_map );
        }

        // Will project scissor/clipping rectangles into framebuffer space
        ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
        ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        // Render command lists
        //
        int counts = draw_data->CmdListsCount;

        TextureHandle last_texture = g_font_texture;
        // todo:map
        DescriptorSetHandle last_descriptor_set = { m_textureToDescriptorSet[ last_texture.m_index ] };

        commands.BindDescriptorSet( &last_descriptor_set, 1, nullptr, 0 );

        uint32_t vtx_buffer_offset = 0, index_buffer_offset = 0;
        for ( int n = 0; n < counts; n++ )
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];

            for ( int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++ )
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if ( pcmd->UserCallback )
                {
                    // User callback (registered via ImDrawList::AddCallback)
                    pcmd->UserCallback( cmd_list, pcmd );
                }
                else
                {
                    // Project scissor/clipping rectangles into framebuffer space
                    ImVec4 clip_rect;
                    clip_rect.x = ( pcmd->ClipRect.x - clip_off.x ) * clip_scale.x;
                    clip_rect.y = ( pcmd->ClipRect.y - clip_off.y ) * clip_scale.y;
                    clip_rect.z = ( pcmd->ClipRect.z - clip_off.x ) * clip_scale.x;
                    clip_rect.w = ( pcmd->ClipRect.w - clip_off.y ) * clip_scale.y;

                    if ( clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f )
                    {
                        // Apply scissor/clipping rectangle
                        if ( clip_origin_lower_left ) {
                            Rect2DInt scissor_rect = { (int16_t)clip_rect.x, (int16_t)( fb_height - clip_rect.w ), (uint16_t)( clip_rect.z - clip_rect.x ), (uint16_t)( clip_rect.w - clip_rect.y ) };
                            commands.SetScissor( &scissor_rect );
                        }
                        else {
                            Rect2DInt scissor_rect = { (int16_t)clip_rect.x, (int16_t)clip_rect.y, (uint16_t)( clip_rect.z - clip_rect.x ), (uint16_t)( clip_rect.w - clip_rect.y ) };
                            commands.SetScissor( &scissor_rect );
                        }

                        // Retrieve
                        TextureHandle new_texture = *(TextureHandle*)( pcmd->TextureId );
                        if ( !m_gpu->bindless_supported ) {
                            if ( new_texture.m_index != last_texture.m_index && new_texture.m_index != k_invalid_texture.m_index ) {
                                last_texture = new_texture;
                                auto it = m_textureToDescriptorSet.find( last_texture.m_index );

                                // TODO: invalidate handles and update descriptor set when needed ?
                                // Found this problem when reusing the handle from a previous
                                // If not present
                                if ( it == m_textureToDescriptorSet.end() ) {
                                    // Create new descriptor set
                                    DescriptorSetCreation ds_creation{};

                                    ds_creation.SetLayout( g_descriptor_set_layout ).Buffer( g_ui_cb, 0 ).Texture( last_texture, 1 ).SetName( "RL_Dynamic_ImGUI" );
                                    last_descriptor_set = m_gpu->create_descriptor_set( ds_creation );

                                    m_textureToDescriptorSet.emplace( new_texture.m_index, last_descriptor_set.m_index );
                                } else {
                                    last_descriptor_set.m_index = it->second;
                                }
                                commands.BindDescriptorSet( &last_descriptor_set, 1, nullptr, 0 );
                            }
                        }

                        commands.DrawIndexed( TopologyType::Triangle, pcmd->ElemCount, 1, index_buffer_offset + pcmd->IdxOffset, vtx_buffer_offset + pcmd->VtxOffset, new_texture.m_index );
                    }
                }

            }
            index_buffer_offset += cmd_list->IdxBuffer.Size;
            vtx_buffer_offset += cmd_list->VtxBuffer.Size;
        }

        commands.PopMarker();
    }

    static void set_style_dark_gold();
    static void set_style_green_blue();
    static void set_style_dark_red();

    void ImGuiService::SetStyle( ImGuiStyles style ) {

        switch ( style ) {
            case GreenBlue:
            {
                set_style_green_blue();
                break;
            }

            case DarkRed:
            {
                set_style_dark_red();
                break;
            }

            case DarkGold:
            {
                set_style_dark_gold();
                break;
            }

            default:
            case Default:
            {
                ImGui::StyleColorsDark();
                break;
            }
        }
    }

    void ImGuiService::RemoveCachedTexture( TextureHandle& texture ) {
        auto it = m_textureToDescriptorSet.find( texture.m_index );
        if ( it != m_textureToDescriptorSet.end() ) {

            // Destroy descriptor set
            DescriptorSetHandle descriptor_set{ it->second };
            m_gpu->destroy_descriptor_set( descriptor_set );

            // Remove from cache
            m_textureToDescriptorSet.erase( texture.m_index );
        }

    }

    void set_style_dark_red() {
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ ImGuiCol_Text ] = ImVec4( 0.75f, 0.75f, 0.75f, 1.00f );
        colors[ ImGuiCol_TextDisabled ] = ImVec4( 0.35f, 0.35f, 0.35f, 1.00f );
        colors[ ImGuiCol_WindowBg ] = ImVec4( 0.00f, 0.00f, 0.00f, 0.94f );
        colors[ ImGuiCol_ChildBg ] = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
        colors[ ImGuiCol_PopupBg ] = ImVec4( 0.08f, 0.08f, 0.08f, 0.94f );
        colors[ ImGuiCol_Border ] = ImVec4( 0.00f, 0.00f, 0.00f, 0.50f );
        colors[ ImGuiCol_BorderShadow ] = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
        colors[ ImGuiCol_FrameBg ] = ImVec4( 0.00f, 0.00f, 0.00f, 0.54f );
        colors[ ImGuiCol_FrameBgHovered ] = ImVec4( 0.37f, 0.14f, 0.14f, 0.67f );
        colors[ ImGuiCol_FrameBgActive ] = ImVec4( 0.39f, 0.20f, 0.20f, 0.67f );
        colors[ ImGuiCol_TitleBg ] = ImVec4( 0.04f, 0.04f, 0.04f, 1.00f );
        colors[ ImGuiCol_TitleBgActive ] = ImVec4( 0.48f, 0.16f, 0.16f, 1.00f );
        colors[ ImGuiCol_TitleBgCollapsed ] = ImVec4( 0.48f, 0.16f, 0.16f, 1.00f );
        colors[ ImGuiCol_MenuBarBg ] = ImVec4( 0.14f, 0.14f, 0.14f, 1.00f );
        colors[ ImGuiCol_ScrollbarBg ] = ImVec4( 0.02f, 0.02f, 0.02f, 0.53f );
        colors[ ImGuiCol_ScrollbarGrab ] = ImVec4( 0.31f, 0.31f, 0.31f, 1.00f );
        colors[ ImGuiCol_ScrollbarGrabHovered ] = ImVec4( 0.41f, 0.41f, 0.41f, 1.00f );
        colors[ ImGuiCol_ScrollbarGrabActive ] = ImVec4( 0.51f, 0.51f, 0.51f, 1.00f );
        colors[ ImGuiCol_CheckMark ] = ImVec4( 0.56f, 0.10f, 0.10f, 1.00f );
        colors[ ImGuiCol_SliderGrab ] = ImVec4( 1.00f, 0.19f, 0.19f, 0.40f );
        colors[ ImGuiCol_SliderGrabActive ] = ImVec4( 0.89f, 0.00f, 0.19f, 1.00f );
        colors[ ImGuiCol_Button ] = ImVec4( 1.00f, 0.19f, 0.19f, 0.40f );
        colors[ ImGuiCol_ButtonHovered ] = ImVec4( 0.80f, 0.17f, 0.00f, 1.00f );
        colors[ ImGuiCol_ButtonActive ] = ImVec4( 0.89f, 0.00f, 0.19f, 1.00f );
        colors[ ImGuiCol_Header ] = ImVec4( 0.33f, 0.35f, 0.36f, 0.53f );
        colors[ ImGuiCol_HeaderHovered ] = ImVec4( 0.76f, 0.28f, 0.44f, 0.67f );
        colors[ ImGuiCol_HeaderActive ] = ImVec4( 0.47f, 0.47f, 0.47f, 0.67f );
        colors[ ImGuiCol_Separator ] = ImVec4( 0.32f, 0.32f, 0.32f, 1.00f );
        colors[ ImGuiCol_SeparatorHovered ] = ImVec4( 0.32f, 0.32f, 0.32f, 1.00f );
        colors[ ImGuiCol_SeparatorActive ] = ImVec4( 0.32f, 0.32f, 0.32f, 1.00f );
        colors[ ImGuiCol_ResizeGrip ] = ImVec4( 1.00f, 1.00f, 1.00f, 0.85f );
        colors[ ImGuiCol_ResizeGripHovered ] = ImVec4( 1.00f, 1.00f, 1.00f, 0.60f );
        colors[ ImGuiCol_ResizeGripActive ] = ImVec4( 1.00f, 1.00f, 1.00f, 0.90f );
        colors[ ImGuiCol_Tab ] = ImVec4( 0.07f, 0.07f, 0.07f, 0.51f );
        colors[ ImGuiCol_TabHovered ] = ImVec4( 0.86f, 0.23f, 0.43f, 0.67f );
        colors[ ImGuiCol_TabActive ] = ImVec4( 0.19f, 0.19f, 0.19f, 0.57f );
        colors[ ImGuiCol_TabUnfocused ] = ImVec4( 0.05f, 0.05f, 0.05f, 0.90f );
        colors[ ImGuiCol_TabUnfocusedActive ] = ImVec4( 0.13f, 0.13f, 0.13f, 0.74f );
#if defined(IMGUI_HAS_DOCK)
        colors[ ImGuiCol_DockingPreview ] = ImVec4( 0.47f, 0.47f, 0.47f, 0.47f );
    colors[ ImGuiCol_DockingEmptyBg ] = ImVec4( 0.20f, 0.20f, 0.20f, 1.00f );
#endif // IMGUI_HAS_DOCK
        colors[ ImGuiCol_PlotLines ] = ImVec4( 0.61f, 0.61f, 0.61f, 1.00f );
        colors[ ImGuiCol_PlotLinesHovered ] = ImVec4( 1.00f, 0.43f, 0.35f, 1.00f );
        colors[ ImGuiCol_PlotHistogram ] = ImVec4( 0.90f, 0.70f, 0.00f, 1.00f );
        colors[ ImGuiCol_PlotHistogramHovered ] = ImVec4( 1.00f, 0.60f, 0.00f, 1.00f );
#if defined(IMGUI_HAS_TABLE)
        colors[ ImGuiCol_TableHeaderBg ] = ImVec4( 0.19f, 0.19f, 0.20f, 1.00f );
        colors[ ImGuiCol_TableBorderStrong ] = ImVec4( 0.31f, 0.31f, 0.35f, 1.00f );
        colors[ ImGuiCol_TableBorderLight ] = ImVec4( 0.23f, 0.23f, 0.25f, 1.00f );
        colors[ ImGuiCol_TableRowBg ] = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
        colors[ ImGuiCol_TableRowBgAlt ] = ImVec4( 1.00f, 1.00f, 1.00f, 0.07f );
#endif // IMGUI_HAS_TABLE
        colors[ ImGuiCol_TextSelectedBg ] = ImVec4( 0.26f, 0.59f, 0.98f, 0.35f );
        colors[ ImGuiCol_DragDropTarget ] = ImVec4( 1.00f, 1.00f, 0.00f, 0.90f );
        colors[ ImGuiCol_NavHighlight ] = ImVec4( 0.26f, 0.59f, 0.98f, 1.00f );
        colors[ ImGuiCol_NavWindowingHighlight ] = ImVec4( 1.00f, 1.00f, 1.00f, 0.70f );
        colors[ ImGuiCol_NavWindowingDimBg ] = ImVec4( 0.80f, 0.80f, 0.80f, 0.20f );
        colors[ ImGuiCol_ModalWindowDimBg ] = ImVec4( 0.80f, 0.80f, 0.80f, 0.35f );
    }


    void set_style_green_blue() {
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ ImGuiCol_Text ] = ImVec4( 1.00f, 1.00f, 1.00f, 1.00f );
        colors[ ImGuiCol_TextDisabled ] = ImVec4( 0.50f, 0.50f, 0.50f, 1.00f );
        colors[ ImGuiCol_WindowBg ] = ImVec4( 0.06f, 0.06f, 0.06f, 0.94f );
        colors[ ImGuiCol_ChildBg ] = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
        colors[ ImGuiCol_PopupBg ] = ImVec4( 0.08f, 0.08f, 0.08f, 0.94f );
        colors[ ImGuiCol_Border ] = ImVec4( 0.43f, 0.43f, 0.50f, 0.50f );
        colors[ ImGuiCol_BorderShadow ] = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
        colors[ ImGuiCol_FrameBg ] = ImVec4( 0.44f, 0.44f, 0.44f, 0.60f );
        colors[ ImGuiCol_FrameBgHovered ] = ImVec4( 0.57f, 0.57f, 0.57f, 0.70f );
        colors[ ImGuiCol_FrameBgActive ] = ImVec4( 0.76f, 0.76f, 0.76f, 0.80f );
        colors[ ImGuiCol_TitleBg ] = ImVec4( 0.04f, 0.04f, 0.04f, 1.00f );
        colors[ ImGuiCol_TitleBgActive ] = ImVec4( 0.16f, 0.16f, 0.16f, 1.00f );
        colors[ ImGuiCol_TitleBgCollapsed ] = ImVec4( 0.00f, 0.00f, 0.00f, 0.60f );
        colors[ ImGuiCol_MenuBarBg ] = ImVec4( 0.14f, 0.14f, 0.14f, 1.00f );
        colors[ ImGuiCol_ScrollbarBg ] = ImVec4( 0.02f, 0.02f, 0.02f, 0.53f );
        colors[ ImGuiCol_ScrollbarGrab ] = ImVec4( 0.31f, 0.31f, 0.31f, 1.00f );
        colors[ ImGuiCol_ScrollbarGrabHovered ] = ImVec4( 0.41f, 0.41f, 0.41f, 1.00f );
        colors[ ImGuiCol_ScrollbarGrabActive ] = ImVec4( 0.51f, 0.51f, 0.51f, 1.00f );
        colors[ ImGuiCol_CheckMark ] = ImVec4( 0.13f, 0.75f, 0.55f, 0.80f );
        colors[ ImGuiCol_SliderGrab ] = ImVec4( 0.13f, 0.75f, 0.75f, 0.80f );
        colors[ ImGuiCol_SliderGrabActive ] = ImVec4( 0.13f, 0.75f, 1.00f, 0.80f );
        colors[ ImGuiCol_Button ] = ImVec4( 0.13f, 0.75f, 0.55f, 0.40f );
        colors[ ImGuiCol_ButtonHovered ] = ImVec4( 0.13f, 0.75f, 0.75f, 0.60f );
        colors[ ImGuiCol_ButtonActive ] = ImVec4( 0.13f, 0.75f, 1.00f, 0.80f );
        colors[ ImGuiCol_Header ] = ImVec4( 0.13f, 0.75f, 0.55f, 0.40f );
        colors[ ImGuiCol_HeaderHovered ] = ImVec4( 0.13f, 0.75f, 0.75f, 0.60f );
        colors[ ImGuiCol_HeaderActive ] = ImVec4( 0.13f, 0.75f, 1.00f, 0.80f );
        colors[ ImGuiCol_Separator ] = ImVec4( 0.13f, 0.75f, 0.55f, 0.40f );
        colors[ ImGuiCol_SeparatorHovered ] = ImVec4( 0.13f, 0.75f, 0.75f, 0.60f );
        colors[ ImGuiCol_SeparatorActive ] = ImVec4( 0.13f, 0.75f, 1.00f, 0.80f );
        colors[ ImGuiCol_ResizeGrip ] = ImVec4( 0.13f, 0.75f, 0.55f, 0.40f );
        colors[ ImGuiCol_ResizeGripHovered ] = ImVec4( 0.13f, 0.75f, 0.75f, 0.60f );
        colors[ ImGuiCol_ResizeGripActive ] = ImVec4( 0.13f, 0.75f, 1.00f, 0.80f );
        colors[ ImGuiCol_Tab ] = ImVec4( 0.13f, 0.75f, 0.55f, 0.80f );
        colors[ ImGuiCol_TabHovered ] = ImVec4( 0.13f, 0.75f, 0.75f, 0.80f );
        colors[ ImGuiCol_TabActive ] = ImVec4( 0.13f, 0.75f, 1.00f, 0.80f );
        colors[ ImGuiCol_TabUnfocused ] = ImVec4( 0.18f, 0.18f, 0.18f, 1.00f );
        colors[ ImGuiCol_TabUnfocusedActive ] = ImVec4( 0.36f, 0.36f, 0.36f, 0.54f );
#if defined(IMGUI_HAS_DOCK)
        colors[ ImGuiCol_DockingPreview ] = ImVec4( 0.13f, 0.75f, 0.55f, 0.80f );
    colors[ ImGuiCol_DockingEmptyBg ] = ImVec4( 0.13f, 0.13f, 0.13f, 0.80f );
#endif // IMGUI_HAS_DOCK
        colors[ ImGuiCol_PlotLines ] = ImVec4( 0.61f, 0.61f, 0.61f, 1.00f );
        colors[ ImGuiCol_PlotLinesHovered ] = ImVec4( 1.00f, 0.43f, 0.35f, 1.00f );
        colors[ ImGuiCol_PlotHistogram ] = ImVec4( 0.90f, 0.70f, 0.00f, 1.00f );
        colors[ ImGuiCol_PlotHistogramHovered ] = ImVec4( 1.00f, 0.60f, 0.00f, 1.00f );
#if defined (IMGUI_HAS_TABLE)
        colors[ ImGuiCol_TableHeaderBg ] = ImVec4( 0.19f, 0.19f, 0.20f, 1.00f );
        colors[ ImGuiCol_TableBorderStrong ] = ImVec4( 0.31f, 0.31f, 0.35f, 1.00f );
        colors[ ImGuiCol_TableBorderLight ] = ImVec4( 0.23f, 0.23f, 0.25f, 1.00f );
        colors[ ImGuiCol_TableRowBg ] = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
        colors[ ImGuiCol_TableRowBgAlt ] = ImVec4( 1.00f, 1.00f, 1.00f, 0.07f );
#endif // IMGUI_HAS_TABLE
        colors[ ImGuiCol_TextSelectedBg ] = ImVec4( 0.26f, 0.59f, 0.98f, 0.35f );
        colors[ ImGuiCol_DragDropTarget ] = ImVec4( 1.00f, 1.00f, 0.00f, 0.90f );
        colors[ ImGuiCol_NavHighlight ] = ImVec4( 0.26f, 0.59f, 0.98f, 1.00f );
        colors[ ImGuiCol_NavWindowingHighlight ] = ImVec4( 1.00f, 1.00f, 1.00f, 0.70f );
        colors[ ImGuiCol_NavWindowingDimBg ] = ImVec4( 0.80f, 0.80f, 0.80f, 0.20f );
        colors[ ImGuiCol_ModalWindowDimBg ] = ImVec4( 0.80f, 0.80f, 0.80f, 0.35f );
    }

    static void set_style_dark_gold() {
        ImGuiStyle* style = &ImGui::GetStyle();
        ImVec4* colors = style->Colors;

        colors[ ImGuiCol_Text ] = ImVec4( 0.92f, 0.92f, 0.92f, 1.00f );
        colors[ ImGuiCol_TextDisabled ] = ImVec4( 0.44f, 0.44f, 0.44f, 1.00f );
        colors[ ImGuiCol_WindowBg ] = ImVec4( 0.06f, 0.06f, 0.06f, 1.00f );
        colors[ ImGuiCol_ChildBg ] = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
        colors[ ImGuiCol_PopupBg ] = ImVec4( 0.08f, 0.08f, 0.08f, 0.94f );
        colors[ ImGuiCol_Border ] = ImVec4( 0.51f, 0.36f, 0.15f, 1.00f );
        colors[ ImGuiCol_BorderShadow ] = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
        colors[ ImGuiCol_FrameBg ] = ImVec4( 0.11f, 0.11f, 0.11f, 1.00f );
        colors[ ImGuiCol_FrameBgHovered ] = ImVec4( 0.51f, 0.36f, 0.15f, 1.00f );
        colors[ ImGuiCol_FrameBgActive ] = ImVec4( 0.78f, 0.55f, 0.21f, 1.00f );
        colors[ ImGuiCol_TitleBg ] = ImVec4( 0.51f, 0.36f, 0.15f, 1.00f );
        colors[ ImGuiCol_TitleBgActive ] = ImVec4( 0.91f, 0.64f, 0.13f, 1.00f );
        colors[ ImGuiCol_TitleBgCollapsed ] = ImVec4( 0.00f, 0.00f, 0.00f, 0.51f );
        colors[ ImGuiCol_MenuBarBg ] = ImVec4( 0.11f, 0.11f, 0.11f, 1.00f );
        colors[ ImGuiCol_ScrollbarBg ] = ImVec4( 0.06f, 0.06f, 0.06f, 0.53f );
        colors[ ImGuiCol_ScrollbarGrab ] = ImVec4( 0.21f, 0.21f, 0.21f, 1.00f );
        colors[ ImGuiCol_ScrollbarGrabHovered ] = ImVec4( 0.47f, 0.47f, 0.47f, 1.00f );
        colors[ ImGuiCol_ScrollbarGrabActive ] = ImVec4( 0.81f, 0.83f, 0.81f, 1.00f );
        colors[ ImGuiCol_CheckMark ] = ImVec4( 0.78f, 0.55f, 0.21f, 1.00f );
        colors[ ImGuiCol_SliderGrab ] = ImVec4( 0.91f, 0.64f, 0.13f, 1.00f );
        colors[ ImGuiCol_SliderGrabActive ] = ImVec4( 0.91f, 0.64f, 0.13f, 1.00f );
        colors[ ImGuiCol_Button ] = ImVec4( 0.51f, 0.36f, 0.15f, 1.00f );
        colors[ ImGuiCol_ButtonHovered ] = ImVec4( 0.91f, 0.64f, 0.13f, 1.00f );
        colors[ ImGuiCol_ButtonActive ] = ImVec4( 0.78f, 0.55f, 0.21f, 1.00f );
        colors[ ImGuiCol_Header ] = ImVec4( 0.51f, 0.36f, 0.15f, 1.00f );
        colors[ ImGuiCol_HeaderHovered ] = ImVec4( 0.91f, 0.64f, 0.13f, 1.00f );
        colors[ ImGuiCol_HeaderActive ] = ImVec4( 0.93f, 0.65f, 0.14f, 1.00f );
        colors[ ImGuiCol_Separator ] = ImVec4( 0.21f, 0.21f, 0.21f, 1.00f );
        colors[ ImGuiCol_SeparatorHovered ] = ImVec4( 0.91f, 0.64f, 0.13f, 1.00f );
        colors[ ImGuiCol_SeparatorActive ] = ImVec4( 0.78f, 0.55f, 0.21f, 1.00f );
        colors[ ImGuiCol_ResizeGrip ] = ImVec4( 0.21f, 0.21f, 0.21f, 1.00f );
        colors[ ImGuiCol_ResizeGripHovered ] = ImVec4( 0.91f, 0.64f, 0.13f, 1.00f );
        colors[ ImGuiCol_ResizeGripActive ] = ImVec4( 0.78f, 0.55f, 0.21f, 1.00f );
        colors[ ImGuiCol_Tab ] = ImVec4( 0.51f, 0.36f, 0.15f, 1.00f );
        colors[ ImGuiCol_TabHovered ] = ImVec4( 0.91f, 0.64f, 0.13f, 1.00f );
        colors[ ImGuiCol_TabActive ] = ImVec4( 0.78f, 0.55f, 0.21f, 1.00f );
        colors[ ImGuiCol_TabUnfocused ] = ImVec4( 0.07f, 0.10f, 0.15f, 0.97f );
        colors[ ImGuiCol_TabUnfocusedActive ] = ImVec4( 0.14f, 0.26f, 0.42f, 1.00f );
        colors[ ImGuiCol_PlotLines ] = ImVec4( 0.61f, 0.61f, 0.61f, 1.00f );
        colors[ ImGuiCol_PlotLinesHovered ] = ImVec4( 1.00f, 0.43f, 0.35f, 1.00f );
        colors[ ImGuiCol_PlotHistogram ] = ImVec4( 0.90f, 0.70f, 0.00f, 1.00f );
        colors[ ImGuiCol_PlotHistogramHovered ] = ImVec4( 1.00f, 0.60f, 0.00f, 1.00f );
        colors[ ImGuiCol_TextSelectedBg ] = ImVec4( 0.26f, 0.59f, 0.98f, 0.35f );
        colors[ ImGuiCol_DragDropTarget ] = ImVec4( 1.00f, 1.00f, 0.00f, 0.90f );
        colors[ ImGuiCol_NavHighlight ] = ImVec4( 0.26f, 0.59f, 0.98f, 1.00f );
        colors[ ImGuiCol_NavWindowingHighlight ] = ImVec4( 1.00f, 1.00f, 1.00f, 0.70f );
        colors[ ImGuiCol_NavWindowingDimBg ] = ImVec4( 0.80f, 0.80f, 0.80f, 0.20f );
        colors[ ImGuiCol_ModalWindowDimBg ] = ImVec4( 0.80f, 0.80f, 0.80f, 0.35f );

        style->FramePadding = ImVec2( 4, 2 );
        style->ItemSpacing = ImVec2( 10, 2 );
        style->IndentSpacing = 12;
        style->ScrollbarSize = 10;

        style->WindowRounding = 4;
        style->FrameRounding = 4;
        style->PopupRounding = 4;
        style->ScrollbarRounding = 6;
        style->GrabRounding = 4;
        style->TabRounding = 4;

        style->WindowTitleAlign = ImVec2( 1.0f, 0.5f );
        style->WindowMenuButtonPosition = ImGuiDir_Right;

        style->DisplaySafeAreaPadding = ImVec2( 4, 4 );
    }

// File Dialog //////////////////////////////////////////////////////////////////
/*

//
//
struct FileDialogOpenMap {

    char*                           key;
    bool                            value;

}; // struct FileDialogOpenMap

static string_hash( FileDialogOpenMap ) file_dialog_open_map = nullptr;

static raptor::Directory             directory;
static char                         filename[MAX_PATH];
static char                         last_path[MAX_PATH];
static char                         last_extension[16];
static bool                         scan_folder             = true;
static bool                         init                    = false;

static raptor::StringArray           files;
static raptor::StringArray           directories;

bool imgui_file_dialog_open( const char* button_name, const char* path, const char* extension ) {

    bool opened = string_hash_get( file_dialog_open_map, button_name );
    if ( ImGui::Button( button_name ) ) {
        opened = true;
    }

    bool selected = false;

    if ( opened && ImGui::Begin( "raptor_imgui_file_dialog", &opened, ImGuiWindowFlags_AlwaysAutoResize ) ) {

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 20, 20 ) );
        ImGui::Text( directory.path );
        ImGui::PopStyleVar();

        ImGui::Separator();

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 20, 4 ) );

        raptor::MemoryAllocator* allocator = raptor::memory_get_system_allocator();

        if ( !init ) {
            init = true;

            files.init( 10000, allocator );
            directories.init( 10000, allocator );

            string_hash_init_dynamic( file_dialog_open_map );

            filename[0] = 0;
            last_path[0] = 0;
            last_extension[0] = 0;
        }

        if ( strcmp( path, last_path ) != 0 ) {
            strcpy( last_path, path );

            raptor::file_open_directory( path, &directory );

            scan_folder = true;
        }

        if ( strcmp( extension, last_extension ) != 0 ) {
            strcpy( last_extension, extension );

            scan_folder = true;
        }

        // Search files
        if ( scan_folder ) {
            scan_folder = false;

            raptor::file_find_files_in_path( extension, directory.path, files, directories );
        }

        for ( size_t d = 0; d < directories.get_string_count(); ++d ) {

            const char* directory_name = directories.get_string( d );
            if ( ImGui::Selectable( directory_name, selected, ImGuiSelectableFlags_AllowDoubleClick ) ) {

                if ( strcmp( directory_name, ".." ) == 0 ) {
                    raptor::file_parent_directory( &directory );
                } else {
                    raptor::file_sub_directory( &directory, directory_name );
                }

                scan_folder = true;
            }
        }

        for ( size_t f = 0; f < files.get_string_count(); ++f ) {
            const char* file_name = files.get_string( f );
            if ( ImGui::Selectable( file_name, selected, ImGuiSelectableFlags_AllowDoubleClick ) ) {

                strcpy( filename, directory.path );
                filename[strlen( filename ) - 1] = 0;
                strcat( filename, file_name );

                selected = true;
                opened = false;
            }
        }

        ImGui::PopStyleVar();

        ImGui::End();
    }

    // Update opened map
    string_hash_put( file_dialog_open_map, button_name, opened );

    return selected;
}

const char * imgui_file_dialog_get_filename() {
    return filename;
}

bool imgui_path_dialog_open( const char* button_name, const char* path ) {
    bool opened = string_hash_get( file_dialog_open_map, button_name );
    if ( ImGui::Button( button_name ) ) {
        opened = true;
    }

    bool selected = false;

    if ( opened && ImGui::Begin( "raptor_imgui_file_dialog", &opened, ImGuiWindowFlags_AlwaysAutoResize ) ) {

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 20, 20 ) );
        ImGui::Text( directory.path );
        ImGui::PopStyleVar();

        ImGui::Separator();

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 20, 4 ) );

        raptor::MemoryAllocator* allocator = raptor::memory_get_system_allocator();

        if ( !init ) {
            init = true;

            files.init( 10000, allocator );
            directories.init( 10000, allocator );

            string_hash_init_dynamic( file_dialog_open_map );

            filename[0] = 0;
            last_path[0] = 0;
            last_extension[0] = 0;
        }

        if ( strcmp( path, last_path ) != 0 ) {
            strcpy( last_path, path );

            raptor::file_open_directory( path, &directory );

            scan_folder = true;
        }

        // Search files
        if ( scan_folder ) {
            scan_folder = false;

            raptor::file_find_files_in_path( ".", directory.path, files, directories );
        }

        for ( size_t d = 0; d < directories.get_string_count(); ++d ) {

            const char* directory_name = directories.get_string( d );
            if ( ImGui::Selectable( directory_name, selected, ImGuiSelectableFlags_AllowDoubleClick ) ) {

                if ( strcmp( directory_name, ".." ) == 0 ) {
                    raptor::file_parent_directory( &directory );
                } else {
                    raptor::file_sub_directory( &directory, directory_name );
                }

                scan_folder = true;
            }
        }

        if ( ImGui::Button( "Choose Current Folder" ) ) {
            strcpy( last_path, directory.path );
            // Remove the asterisk
            last_path[strlen( last_path ) - 1] = 0;

            selected = true;
            opened = false;
        }
        ImGui::SameLine();
        if ( ImGui::Button( "Cancel" ) ) {
            opened = false;
        }

        ImGui::PopStyleVar();

        ImGui::End();
    }

    // Update opened map
    string_hash_put( file_dialog_open_map, button_name, opened );

    return selected;
}

const char* imgui_path_dialog_get_path() {
    return last_path;
}

////////////////////////////////////////////////////////////
*/
// Usage:
//  static ExampleAppLog my_log;
//  my_log.AddLog("Hello %d world\n", 123);
//  my_log.Draw("title");
    struct ExampleAppLog {
        ImGuiTextBuffer     Buf;
        ImGuiTextFilter     Filter;
        ImVector<int>       LineOffsets;        // Index to lines offset. We maintain this with AddLog() calls, allowing us to have a random access on lines
        bool                AutoScroll;     // Keep scrolling if already at the bottom

        ExampleAppLog() {
            AutoScroll = true;
            Clear();
        }

        void    Clear() {
            Buf.clear();
            LineOffsets.clear();
            LineOffsets.push_back( 0 );
        }

        void    AddLog( const char* fmt, ... ) IM_FMTARGS( 2 ) {
            int old_size = Buf.size();
            va_list args;
                    va_start( args, fmt );
            Buf.appendfv( fmt, args );
                    va_end( args );
            for ( int new_size = Buf.size(); old_size < new_size; old_size++ )
                if ( Buf[ old_size ] == '\n' )
                    LineOffsets.push_back( old_size + 1 );
        }

        void    Draw( const char* title, bool* p_open = NULL ) {
            if ( !ImGui::Begin( title, p_open ) ) {
                ImGui::End();
                return;
            }

            // Options menu
            if ( ImGui::BeginPopup( "Options" ) ) {
                ImGui::Checkbox( "Auto-scroll", &AutoScroll );
                ImGui::EndPopup();
            }

            // Main window
            if ( ImGui::Button( "Options" ) )
                ImGui::OpenPopup( "Options" );
            ImGui::SameLine();
            bool clear = ImGui::Button( "Clear" );
            ImGui::SameLine();
            bool copy = ImGui::Button( "Copy" );
            ImGui::SameLine();
            Filter.Draw( "Filter", -100.0f );

            ImGui::Separator();
            ImGui::BeginChild( "scrolling", ImVec2( 0, 0 ), false, ImGuiWindowFlags_HorizontalScrollbar );

            if ( clear )
                Clear();
            if ( copy )
                ImGui::LogToClipboard();

            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
            const char* buf = Buf.begin();
            const char* buf_end = Buf.end();
            if ( Filter.IsActive() ) {
                // In this example we don't use the clipper when Filter is enabled.
                // This is because we don't have a random access on the result on our filter.
                // A real application processing logs with ten of thousands of entries may want to store the result of search/filter.
                // especially if the filtering function is not trivial (e.g. reg-exp).
                for ( int line_no = 0; line_no < LineOffsets.Size; line_no++ ) {
                    const char* line_start = buf + LineOffsets[ line_no ];
                    const char* line_end = ( line_no + 1 < LineOffsets.Size ) ? ( buf + LineOffsets[ line_no + 1 ] - 1 ) : buf_end;
                    if ( Filter.PassFilter( line_start, line_end ) )
                        ImGui::TextUnformatted( line_start, line_end );
                }
            } else {
                // The simplest and easy way to display the entire buffer:
                //   ImGui::TextUnformatted(buf_begin, buf_end);
                // And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward to skip non-visible lines.
                // Here we instead demonstrate using the clipper to only process lines that are within the visible area.
                // If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them on your side is recommended.
                // Using ImGuiListClipper requires A) random access into your data, and B) items all being the  same height,
                // both of which we can handle since we an array pointing to the beginning of each line of text.
                // When using the filter (in the block of code above) we don't have random access into the data to display anymore, which is why we don't use the clipper.
                // Storing or skimming through the search result would make it possible (and would be recommended if you want to search through tens of thousands of entries)
                ImGuiListClipper clipper;
                clipper.Begin( LineOffsets.Size );
                while ( clipper.Step() ) {
                    for ( int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++ ) {
                        const char* line_start = buf + LineOffsets[ line_no ];
                        const char* line_end = ( line_no + 1 < LineOffsets.Size ) ? ( buf + LineOffsets[ line_no + 1 ] - 1 ) : buf_end;
                        ImGui::TextUnformatted( line_start, line_end );
                    }
                }
                clipper.End();
            }
            ImGui::PopStyleVar();

            if ( AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
                ImGui::SetScrollHereY( 1.0f );

            ImGui::EndChild();
            ImGui::End();
        }
    }; // struct ExampleAppLog

    static ExampleAppLog        s_imgui_log;
    static bool                 s_imgui_log_open = true;

    static void imgui_print( const char* text ) {
        s_imgui_log.AddLog( "%s", text );
    }

    void imgui_log_init() {
        // TODO : Do something about this
//        LogService::instance()->set_callback( &imgui_print );
    }

    void imgui_log_shutdown() {
        // TODO : Do something about this
//        LogService::instance()->set_callback( nullptr );
    }

    void imgui_log_draw() {
        s_imgui_log.Draw( "Log", &s_imgui_log_open );
    }


// Plot with ringbuffer

// https://github.com/leiradel/ImGuiAl
    template<typename T, size_t L>
    class Sparkline {
    public:
        Sparkline() {
            setLimits( 0, 1 );
            clear();
        }

        void setLimits( T const min, T const max ) {
            _min = static_cast< float >( min );
            _max = static_cast< float >( max );
        }

        void add( T const value ) {
            _offset = ( _offset + 1 ) % L;
            _values[ _offset ] = value;
        }

        void clear() {
            memset( _values, 0, L * sizeof( T ) );
            _offset = L - 1;
        }

        void draw( char const* const label = "", ImVec2 const size = ImVec2() ) const {
            char overlay[ 32 ];
            print( overlay, sizeof( overlay ), _values[ _offset ] );

            ImGui::PlotLines( label, getValue, const_cast< Sparkline* >( this ), L, 0, overlay, _min, _max, size );
        }

    protected:
        float _min, _max;
        T _values[ L ];
        size_t _offset;

        static float getValue( void* const data, int const idx ) {
            Sparkline const* const self = static_cast< Sparkline* >( data );
            size_t const index = ( idx + self->_offset + 1 ) % L;
            return static_cast< float >( self->_values[ index ] );
        }

        static void print( char* const buffer, size_t const bufferLen, int const value ) {
            snprintf( buffer, bufferLen, "%d", value );
        }

        static void print( char* const buffer, size_t const bufferLen, double const value ) {
            snprintf( buffer, bufferLen, "%f", value );
        }
    };

    static Sparkline<f32, 100> s_fps_line;

    void imgui_fps_init() {
        s_fps_line.clear();
        s_fps_line.setLimits( 0.0f, 33.f );
    }

    void imgui_fps_shutdown() {
    }

    void imgui_fps_add( f32 dt ) {
        s_fps_line.add( dt );
    }

    void imgui_fps_draw() {
        s_fps_line.draw( "FPS", { 0,100 } );
    }
}
