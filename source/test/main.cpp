#include <filesystem>
#include <iostream>
#include <map>
#include <vector>
#include <vulkan/vulkan.h>

#include <cglm/struct/mat3.h>
#include <cglm/struct/mat4.h>
#include <cglm/struct/quat.h>
#include <cglm/struct/cam.h>
#include <cglm/struct/affine.h>

#include <imgui.h>

#include <tracy/Tracy.hpp>

import Foundation.Assert;
import Foundation.Memory.Allocators.Allocator;
import Foundation.Memory.Allocators.LinearAllocator;
import Foundation.Memory.Allocators.StackAllocator;
import Foundation.Memory.MemoryDefines;
import Foundation.Services.MemoryService;
import Foundation.Services.ServiceManager;
import Foundation.Services.Service;
import Foundation.glTF;
import Foundation.Log;
import Foundation.ResourceManager;
import Foundation.File;
import Foundation.Numerics;

import Application.Input;
import Application.Window;
import Application.Graphics.GPUDevice;
import Application.Graphics.GPUProfiler;
import Application.Graphics.Renderer;
import Application.Graphics.ImGuiService;
import Application.Graphics.CommandBuffer;

using StringBuffer = std::basic_string<char, std::char_traits<char>, Caustix::STLAdaptor<char>>;
#define Array(Element) std::vector<Element, Caustix::STLAdaptor<Element>>

enum MaterialFeatures {
    MaterialFeatures_ColorTexture     = 1 << 0,
    MaterialFeatures_NormalTexture    = 1 << 1,
    MaterialFeatures_RoughnessTexture = 1 << 2,
    MaterialFeatures_OcclusionTexture = 1 << 3,
    MaterialFeatures_EmissiveTexture  = 1 << 4,

    MaterialFeatures_TangentVertexAttribute = 1 << 5,
    MaterialFeatures_TexcoordVertexAttribute = 1 << 6,
};

struct alignas( 16 ) MaterialData {
    vec4s baseColorFactor;
    mat4s model;
    mat4s modelInv;

    vec3s emissiveFactor;
    f32   metallicFactor;

    f32   roughnessFactor;
    f32   occlusionFactor;
    u32   flags;
};

struct MeshDraw {
    Caustix::BufferHandle indexBuffer;
    Caustix::BufferHandle positionBuffer;
    Caustix::BufferHandle tangentBuffer;
    Caustix::BufferHandle normalBuffer;
    Caustix::BufferHandle texcoordBuffer;

    Caustix::BufferHandle materialBuffer;
    MaterialData          materialData;

    u32 indexOffset;
    u32 positionOffset;
    u32 tangentOffset;
    u32 normalOffset;
    u32 texcoordOffset;

    u32 count;

    VkIndexType indexType;

    Caustix::DescriptorSetHandle descriptorSet;
};

struct UniformData {
    mat4s m;
    mat4s vp;
    vec4s eye;
    vec4s light;
};

struct Transform {

    vec3s                   scale;
    versors                 rotation;
    vec3s                   translation;

    void                    reset();
    mat4s                   calculate_matrix() const {
        const mat4s translation_matrix = glms_translate_make( translation );
        const mat4s scale_matrix = glms_scale_make( scale );
        const mat4s local_matrix = glms_mat4_mul( glms_mat4_mul( translation_matrix, glms_quat_mat4( rotation ) ), scale_matrix );
        return local_matrix;
    }
}; // struct Transform

static void InputOsMessagesCallback(void *os_event, void *user_data) {
      Caustix::InputService *input = (Caustix::InputService *)user_data;
      input->OnEvent(os_event);
}

static u8* get_buffer_data( Caustix::glTF::BufferView* bufferViews, u32 bufferIndex, Array(void*)& buffersData, u32* bufferSize = nullptr, char** bufferName = nullptr ) {
    using namespace Caustix;

    glTF::BufferView& buffer = bufferViews[ bufferIndex ];

    i32 offset = buffer.byte_offset;
    if ( offset == glTF::INVALID_INT_VALUE ) {
        offset = 0;
    }

    if ( bufferName != nullptr ) {
        if (!buffer.name.empty()) {
            *bufferName = buffer.name.data();
        }
    }

    if ( bufferSize != nullptr ) {
        *bufferSize = buffer.byte_length;
    }

    u8* data = ( u8* )buffersData[ buffer.buffer ] + offset;

    return data;
}

int main(int argc, char **argv) {

    using namespace Caustix;
    
    if (argc < 2) {
        info("Usage: chapter1 [path to glTF model]");
        auto data = std::filesystem::current_path();
        // info("{}", data.c_str());
        InjectDefault3DModel(argc, argv);
    }

    StackAllocator scratchAllocator(cmega(8));

    MemoryServiceConfiguration configuration;
    ServiceManager::GetInstance()->AddService(MemoryService::Create(configuration), MemoryService::m_name);
    Allocator *allocator = &ServiceManager::GetInstance()->Get<MemoryService>()->m_systemAllocator;

    WindowConfiguration wconf{1280, 800, "Caustix Test", allocator};
    ServiceManager::GetInstance()->AddService(Window::Create(wconf), Window::m_name);
    Window *window = ServiceManager::GetInstance()->Get<Window>();

    ServiceManager::GetInstance()->AddService(InputService::Create(allocator), InputService::m_name);
    InputService *inputHandler = ServiceManager::GetInstance()->Get<InputService>();

    // Callback register
    window->RegisterOsMessagesCallback(InputOsMessagesCallback, inputHandler);

    // Graphics
    DeviceCreation deviceCreation;
    deviceCreation.SetWindow(window->m_width, window->m_height, window->m_platformHandle).SetAllocator(allocator).SetLinearAllocator(&scratchAllocator);
    ServiceManager::GetInstance()->AddService(GpuDevice::Create(deviceCreation), GpuDevice::m_name);
    GpuDevice *gpu = ServiceManager::GetInstance()->Get<GpuDevice>();

    ResourceManager resourceManager(allocator, nullptr);

    GPUProfiler gpuProfiler(allocator, 100);

    ServiceManager::GetInstance()->AddService(Renderer::Create({gpu, allocator}), Renderer::m_name);
    Renderer *renderer = ServiceManager::GetInstance()->Get<Renderer>();
    renderer->SetLoaders(&resourceManager);

    ImGuiServiceConfiguration imGuiServiceConfiguration{gpu, window->m_platformHandle};
    ServiceManager::GetInstance()->AddService(ImGuiService::Create(imGuiServiceConfiguration),ImGuiService::m_name);
    ImGuiService *imGuiService = ServiceManager::GetInstance()->Get<ImGuiService>();

    char gltfBasePath[512]{};
    memcpy(gltfBasePath, argv[1], strlen(argv[1]));
    FileDirectoryFromPath(gltfBasePath);

    std::filesystem::current_path(gltfBasePath);

    char gltfFile[512]{};
    memcpy(gltfFile, argv[1], strlen(argv[1]));
    FileNameFromPath(gltfFile);

    LinearAllocator linearAllocator(cmega(2));
    glTF::glTF scene = gltfLoadFile(gltfFile, &linearAllocator);

    Array(TextureResource) images(*allocator);
    images.reserve(scene.images_count);

    for (u32 image_index = 0; image_index < scene.images_count; ++image_index) {
        glTF::Image &image = scene.images[image_index];
        TextureResource *tr =
            renderer->CreateTexture(image.uri.data(), image.uri.data());
        CASSERT(tr != nullptr);

        images.push_back(*tr);
    }

    TextureCreation textureCreation{};
    u32 zeroValue = 0;
    textureCreation.SetName("dummyTexture").SetSize(1,1,1).SetFormatType(VK_FORMAT_R8G8B8A8_UNORM, TextureType::Texture2D).SetFlags(1,0).SetData(&zeroValue);
    TextureHandle dummyTexture = gpu->create_texture(textureCreation);

    SamplerCreation samplerCreation;
    samplerCreation.m_minFilter = VK_FILTER_LINEAR;
    samplerCreation.m_magFilter = VK_FILTER_LINEAR;
    samplerCreation.m_addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreation.m_addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerHandle dummySampler = gpu->create_sampler(samplerCreation);

    StringBuffer resourceNameBuffer(*allocator);
    resourceNameBuffer.reserve(ckilo(64));

    Array(SamplerResource) samplers(*allocator);
    samplers.reserve(scene.samplers_count);

    for ( u32 samplerIndex = 0; samplerIndex < scene.samplers_count; ++samplerIndex ) {
        glTF::Sampler& sampler = scene.samplers[ samplerIndex ];

        char* sampler_name = resourceNameBuffer.data() + resourceNameBuffer.size();
        resourceNameBuffer.append(std::format("sampler_{}", samplerIndex));
        resourceNameBuffer.push_back('\0');

        SamplerCreation creation;
        creation.m_minFilter = sampler.min_filter == glTF::Sampler::Filter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        creation.m_magFilter = sampler.mag_filter == glTF::Sampler::Filter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        creation.m_name = sampler_name;

        SamplerResource* sr = renderer->CreateSampler( creation );
        CASSERT( sr != nullptr );

        samplers.push_back( *sr );
    }

    Array(void*) buffersData(*allocator);
    buffersData.reserve(scene.buffers_count);

    for ( u32 bufferIndex = 0; bufferIndex < scene.buffers_count; ++bufferIndex ) {
        glTF::Buffer& buffer = scene.buffers[ bufferIndex ];

        FileReadResult bufferData = FileReadBinary(buffer.uri.data(), allocator);
        buffersData.push_back( bufferData.data );
    }

    Array(BufferResource) buffers(*allocator);
    buffers.reserve(scene.buffer_views_count);

    for ( u32 bufferIndex = 0; bufferIndex < scene.buffer_views_count; ++bufferIndex ) {
        char* bufferName = nullptr;
        u32 buffer_size = 0;
        u8* data = get_buffer_data( scene.buffer_views, bufferIndex, buffersData, &buffer_size, &bufferName );

        // NOTE(marco): the target attribute of a BufferView is not mandatory, so we prepare for both uses
        VkBufferUsageFlags flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        if ( bufferName == nullptr ) {
            bufferName = resourceNameBuffer.data() + resourceNameBuffer.size();
            resourceNameBuffer.append(std::format("buffer_{}", bufferIndex));
            resourceNameBuffer.push_back('\0');
        } else {
            // NOTE(marco); some buffers might have the same name, which causes issues in the renderer cache
            bufferName = resourceNameBuffer.data() + resourceNameBuffer.size();
            resourceNameBuffer.append(std::format("{}_{}", bufferName, bufferIndex));
            resourceNameBuffer.push_back('\0');
        }

        BufferResource* br = renderer->CreateBuffer( flags, ResourceUsageType::Immutable, buffer_size, data, bufferName );
        CASSERT( br != nullptr);

        buffers.push_back( *br );
    }

    Array(MeshDraw) meshDraws(*allocator);
    meshDraws.reserve(scene.meshes_count);

    Array(BufferHandle) customMeshBuffers(*allocator);
    customMeshBuffers.reserve(8);

    vec4s dummyData[3]{};

    BufferCreation bufferCreation;
    bufferCreation.Set(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, ResourceUsageType::Immutable, sizeof(vec4s)*3).SetData(dummyData).SetName("dummyAttributeBuffer");

    BufferHandle dummyAttributeBuffer = gpu->create_buffer(bufferCreation);

    BufferHandle                    cube_vb;
    BufferHandle                    cube_ib;
    PipelineHandle                  cube_pipeline;
    BufferHandle                    cube_cb;
    DescriptorSetHandle             cube_rl;
    DescriptorSetLayoutHandle       cube_dsl;

    {
        // Create pipeline state
        PipelineCreation pipelineCreation;

        // Vertex input
        // TODO(marco): component format should be based on buffer view type
        pipelineCreation.m_vertexInput.AddVertexAttribute({0,0,0, VertexComponentFormat::Float3}); // position
        pipelineCreation.m_vertexInput.AddVertexStream( { 0, 12, VertexInputRate::PerVertex } );

        pipelineCreation.m_vertexInput.AddVertexAttribute( { 1, 1, 0, VertexComponentFormat::Float4 } ); // tangent
        pipelineCreation.m_vertexInput.AddVertexStream( { 1, 16, VertexInputRate::PerVertex} );

        pipelineCreation.m_vertexInput.AddVertexAttribute( { 2, 2, 0, VertexComponentFormat::Float3 } ); // normal
        pipelineCreation.m_vertexInput.AddVertexStream( { 2, 12, VertexInputRate::PerVertex } );

        pipelineCreation.m_vertexInput.AddVertexAttribute( { 3, 3, 0, VertexComponentFormat::Float2 } ); // texcoord
        pipelineCreation.m_vertexInput.AddVertexStream( { 3, 8, VertexInputRate::PerVertex} );

        // Render pass
        pipelineCreation.m_renderPass = gpu->get_swapchain_output();
        // Depth
        pipelineCreation.m_depthStencil.SetDepth( true, VK_COMPARE_OP_LESS_OR_EQUAL );

        // Shader state
        const char* vs_code = R"FOO(#version 450
uint MaterialFeatures_ColorTexture     = 1 << 0;
uint MaterialFeatures_NormalTexture    = 1 << 1;
uint MaterialFeatures_RoughnessTexture = 1 << 2;
uint MaterialFeatures_OcclusionTexture = 1 << 3;
uint MaterialFeatures_EmissiveTexture =  1 << 4;
uint MaterialFeatures_TangentVertexAttribute = 1 << 5;
uint MaterialFeatures_TexcoordVertexAttribute = 1 << 6;

layout(std140, binding = 0) uniform LocalConstants {
    mat4 m;
    mat4 vp;
    vec4 eye;
    vec4 light;
};

layout(std140, binding = 1) uniform MaterialConstant {
    vec4 base_color_factor;
    mat4 model;
    mat4 model_inv;

    vec3  emissive_factor;
    float metallic_factor;

    float roughness_factor;
    float occlusion_factor;
    uint  flags;
};

layout(location=0) in vec3 position;
layout(location=1) in vec4 tangent;
layout(location=2) in vec3 normal;
layout(location=3) in vec2 texCoord0;

layout (location = 0) out vec2 vTexcoord0;
layout (location = 1) out vec3 vNormal;
layout (location = 2) out vec4 vTangent;
layout (location = 3) out vec4 vPosition;

void main() {
    gl_Position = vp * m * model * vec4(position, 1);
    vPosition = m * model * vec4(position, 1.0);

    if ( ( flags & MaterialFeatures_TexcoordVertexAttribute ) != 0 ) {
        vTexcoord0 = texCoord0;
    }
    vNormal = mat3( model_inv ) * normal;

    if ( ( flags & MaterialFeatures_TangentVertexAttribute ) != 0 ) {
        vTangent = tangent;
    }
}
)FOO";

        const char* fs_code = R"FOO(#version 450
uint MaterialFeatures_ColorTexture     = 1 << 0;
uint MaterialFeatures_NormalTexture    = 1 << 1;
uint MaterialFeatures_RoughnessTexture = 1 << 2;
uint MaterialFeatures_OcclusionTexture = 1 << 3;
uint MaterialFeatures_EmissiveTexture =  1 << 4;
uint MaterialFeatures_TangentVertexAttribute = 1 << 5;
uint MaterialFeatures_TexcoordVertexAttribute = 1 << 6;

layout(std140, binding = 0) uniform LocalConstants {
    mat4 m;
    mat4 vp;
    vec4 eye;
    vec4 light;
};

layout(std140, binding = 1) uniform MaterialConstant {
    vec4 base_color_factor;
    mat4 model;
    mat4 model_inv;

    vec3  emissive_factor;
    float metallic_factor;

    float roughness_factor;
    float occlusion_factor;
    uint  flags;
};

layout (binding = 2) uniform sampler2D diffuseTexture;
layout (binding = 3) uniform sampler2D roughnessMetalnessTexture;
layout (binding = 4) uniform sampler2D occlusionTexture;
layout (binding = 5) uniform sampler2D emissiveTexture;
layout (binding = 6) uniform sampler2D normalTexture;

layout (location = 0) in vec2 vTexcoord0;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec4 vTangent;
layout (location = 3) in vec4 vPosition;

layout (location = 0) out vec4 frag_color;

#define PI 3.1415926538

vec3 decode_srgb( vec3 c ) {
    vec3 result;
    if ( c.r <= 0.04045) {
        result.r = c.r / 12.92;
    } else {
        result.r = pow( ( c.r + 0.055 ) / 1.055, 2.4 );
    }

    if ( c.g <= 0.04045) {
        result.g = c.g / 12.92;
    } else {
        result.g = pow( ( c.g + 0.055 ) / 1.055, 2.4 );
    }

    if ( c.b <= 0.04045) {
        result.b = c.b / 12.92;
    } else {
        result.b = pow( ( c.b + 0.055 ) / 1.055, 2.4 );
    }

    return clamp( result, 0.0, 1.0 );
}

vec3 encode_srgb( vec3 c ) {
    vec3 result;
    if ( c.r <= 0.0031308) {
        result.r = c.r * 12.92;
    } else {
        result.r = 1.055 * pow( c.r, 1.0 / 2.4 ) - 0.055;
    }

    if ( c.g <= 0.0031308) {
        result.g = c.g * 12.92;
    } else {
        result.g = 1.055 * pow( c.g, 1.0 / 2.4 ) - 0.055;
    }

    if ( c.b <= 0.0031308) {
        result.b = c.b * 12.92;
    } else {
        result.b = 1.055 * pow( c.b, 1.0 / 2.4 ) - 0.055;
    }

    return clamp( result, 0.0, 1.0 );
}

float heaviside( float v ) {
    if ( v > 0.0 ) return 1.0;
    else return 0.0;
}

void main() {

    mat3 TBN = mat3( 1.0 );

    if ( ( flags & MaterialFeatures_TangentVertexAttribute ) != 0 ) {
        vec3 tangent = normalize( vTangent.xyz );
        vec3 bitangent = cross( normalize( vNormal ), tangent ) * vTangent.w;

        TBN = mat3(
            tangent,
            bitangent,
            normalize( vNormal )
        );
    }
    else {
        // NOTE(marco): taken from https://community.khronos.org/t/computing-the-tangent-space-in-the-fragment-shader/52861
        vec3 Q1 = dFdx( vPosition.xyz );
        vec3 Q2 = dFdy( vPosition.xyz );
        vec2 st1 = dFdx( vTexcoord0 );
        vec2 st2 = dFdy( vTexcoord0 );

        vec3 T = normalize(  Q1 * st2.t - Q2 * st1.t );
        vec3 B = normalize( -Q1 * st2.s + Q2 * st1.s );

        // the transpose of texture-to-eye space matrix
        TBN = mat3(
            T,
            B,
            normalize( vNormal )
        );
    }

    // vec3 V = normalize(eye.xyz - vPosition.xyz);
    // vec3 L = normalize(light.xyz - vPosition.xyz);
    // vec3 N = normalize(vNormal);
    // vec3 H = normalize(L + V);

    vec3 V = normalize( eye.xyz - vPosition.xyz );
    vec3 L = normalize( light.xyz - vPosition.xyz );
    // NOTE(marco): normal textures are encoded to [0, 1] but need to be mapped to [-1, 1] value
    vec3 N = normalize( vNormal );
    if ( ( flags & MaterialFeatures_NormalTexture ) != 0 ) {
        N = normalize( texture(normalTexture, vTexcoord0).rgb * 2.0 - 1.0 );
        N = normalize( TBN * N );
    }
    vec3 H = normalize( L + V );

    float roughness = roughness_factor;
    float metalness = metallic_factor;

    if ( ( flags & MaterialFeatures_RoughnessTexture ) != 0 ) {
        // Red channel for occlusion value
        // Green channel contains roughness values
        // Blue channel contains metalness
        vec4 rm = texture(roughnessMetalnessTexture, vTexcoord0);

        roughness *= rm.g;
        metalness *= rm.b;
    }

    float ao = 1.0f;
    if ( ( flags & MaterialFeatures_OcclusionTexture ) != 0 ) {
        ao = texture(occlusionTexture, vTexcoord0).r;
    }

    float alpha = pow(roughness, 2.0);

    vec4 base_colour = base_color_factor;
    if ( ( flags & MaterialFeatures_ColorTexture ) != 0 ) {
        vec4 albedo = texture( diffuseTexture, vTexcoord0 );
        base_colour.rgb *= decode_srgb( albedo.rgb );
        base_colour.a *= albedo.a;
    }

    vec3 emissive = vec3( 0 );
    if ( ( flags & MaterialFeatures_EmissiveTexture ) != 0 ) {
        vec4 e = texture(emissiveTexture, vTexcoord0);

        emissive += decode_srgb( e.rgb ) * emissive_factor;
    }

    // https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#specular-brdf
    float NdotH = dot(N, H);
    float alpha_squared = alpha * alpha;
    float d_denom = ( NdotH * NdotH ) * ( alpha_squared - 1.0 ) + 1.0;
    float distribution = ( alpha_squared * heaviside( NdotH ) ) / ( PI * d_denom * d_denom );

    float NdotL = clamp( dot(N, L), 0, 1 );

    if ( NdotL > 1e-5 ) {
        float NdotV = dot(N, V);
        float HdotL = dot(H, L);
        float HdotV = dot(H, V);

        float visibility = ( heaviside( HdotL ) / ( abs( NdotL ) + sqrt( alpha_squared + ( 1.0 - alpha_squared ) * ( NdotL * NdotL ) ) ) ) * ( heaviside( HdotV ) / ( abs( NdotV ) + sqrt( alpha_squared + ( 1.0 - alpha_squared ) * ( NdotV * NdotV ) ) ) );

        float specular_brdf = visibility * distribution;

        vec3 diffuse_brdf = (1 / PI) * base_colour.rgb;

        // NOTE(marco): f0 in the formula notation refers to the base colour here
        vec3 conductor_fresnel = specular_brdf * ( base_colour.rgb + ( 1.0 - base_colour.rgb ) * pow( 1.0 - abs( HdotV ), 5 ) );

        // NOTE(marco): f0 in the formula notation refers to the value derived from ior = 1.5
        float f0 = 0.04; // pow( ( 1 - ior ) / ( 1 + ior ), 2 )
        float fr = f0 + ( 1 - f0 ) * pow(1 - abs( HdotV ), 5 );
        vec3 fresnel_mix = mix( diffuse_brdf, vec3( specular_brdf ), fr );

        vec3 material_colour = mix( fresnel_mix, conductor_fresnel, metalness );

        material_colour = emissive + mix( material_colour, material_colour * ao, occlusion_factor);

        frag_color = vec4( encode_srgb( material_colour ), base_colour.a );
    } else {
        frag_color = vec4( base_colour.rgb * 0.1, base_colour.a );
    }
}
)FOO";

        pipelineCreation.m_shaders.SetName( "Cube" ).AddStage( vs_code, ( uint32_t )strlen( vs_code ), VK_SHADER_STAGE_VERTEX_BIT ).AddStage( fs_code, ( uint32_t )strlen( fs_code ), VK_SHADER_STAGE_FRAGMENT_BIT );

        // Descriptor set layout
        DescriptorSetLayoutCreation cubeRllCreation{};
        cubeRllCreation.AddBinding( { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, "LocalConstants" } );
        cubeRllCreation.AddBinding( { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 1, "MaterialConstant" } );
        cubeRllCreation.AddBinding( { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, 1, "diffuseTexture" } );
        cubeRllCreation.AddBinding( { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, 1, "roughnessMetalnessTexture" } );
        cubeRllCreation.AddBinding( { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, 1, "roughnessMetalnessTexture" } );
        cubeRllCreation.AddBinding( { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, 1, "emissiveTexture" } );
        cubeRllCreation.AddBinding( { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6, 1, "occlusionTexture" } );
        // Setting it into pipeline
        cube_dsl = gpu->create_descriptor_set_layout( cubeRllCreation );
        pipelineCreation.AddDescriptorSetLayout( cube_dsl );

        // Constant buffer
        BufferCreation buffer_creation;
        buffer_creation.Reset().Set( VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, ResourceUsageType::Dynamic, sizeof( UniformData ) ).SetName( "cube_cb" );
        cube_cb = gpu->create_buffer( buffer_creation );

        cube_pipeline = gpu->create_pipeline( pipelineCreation );

        glTF::Scene& root_gltf_scene = scene.scenes[ scene.scene ];

        Array(i32) node_parents(*allocator);
        node_parents.reserve( scene.nodes_count );
        node_parents.resize( scene.nodes_count );

        Array(u32) node_stack(*allocator);
        node_stack.reserve( 8 );

        Array(mat4s) node_matrix(*allocator);
        node_matrix.reserve( scene.nodes_count );
        node_matrix.resize( scene.nodes_count );

        for ( u32 node_index = 0; node_index < root_gltf_scene.nodes_count; ++node_index ) {
            u32 root_node = root_gltf_scene.nodes[ node_index ];
            node_parents[ root_node ] = -1;
            node_stack.push_back( root_node );
        }

        while ( node_stack.size() ) {
            u32 node_index = node_stack.back();
            node_stack.pop_back();
            glTF::Node& node = scene.nodes[ node_index ];

            mat4s local_matrix{ };

            if ( node.matrix_count ) {
                // CGLM and glTF have the same matrix layout, just memcopy it
                memcpy( &local_matrix, node.matrix, sizeof( mat4s ) );
            }
            else {
                vec3s node_scale{ 1.0f, 1.0f, 1.0f };
                if ( node.scale_count != 0 ) {
                    CASSERT( node.scale_count == 3 );
                    node_scale = vec3s{ node.scale[0], node.scale[1], node.scale[2] };
                }

                vec3s node_translation{ 0.f, 0.f, 0.f };
                if ( node.translation_count ) {
                    CASSERT( node.translation_count == 3 );
                    node_translation = vec3s{ node.translation[ 0 ], node.translation[ 1 ], node.translation[ 2 ] };
                }

                // Rotation is written as a plain quaternion
                versors node_rotation = glms_quat_identity();
                if ( node.rotation_count ) {
                    CASSERT( node.rotation_count == 4 );
                    node_rotation = glms_quat_init( node.rotation[ 0 ], node.rotation[ 1 ], node.rotation[ 2 ], node.rotation[ 3 ] );
                }

                Transform transform;
                transform.translation = node_translation;
                transform.scale = node_scale;
                transform.rotation = node_rotation;

                local_matrix = transform.calculate_matrix();
            }

            node_matrix[ node_index ] = local_matrix;

            for ( u32 child_index = 0; child_index < node.children_count; ++child_index ) {
                u32 child_node_index = node.children[ child_index ];
                node_parents[ child_node_index ] = node_index;
                node_stack.push_back( child_node_index );
            }

            if ( node.mesh == glTF::INVALID_INT_VALUE ) {
                continue;
            }

            glTF::Mesh& mesh = scene.meshes[ node.mesh ];

            mat4s final_matrix = local_matrix;
            i32 node_parent = node_parents[ node_index ];
            while( node_parent != -1 ) {
                final_matrix = glms_mat4_mul( node_matrix[ node_parent ], final_matrix );
                node_parent = node_parents[ node_parent ];
            }

            // Final SRT composition
            for ( u32 primitive_index = 0; primitive_index < mesh.primitives_count; ++primitive_index ) {
                MeshDraw mesh_draw{ };

                mesh_draw.materialData.model = final_matrix;

                glTF::MeshPrimitive& mesh_primitive = mesh.primitives[ primitive_index ];

                glTF::Accessor& indices_accessor = scene.accessors[ mesh_primitive.indices ];
                CASSERT( indices_accessor.component_type == glTF::Accessor::UNSIGNED_INT || indices_accessor.component_type == glTF::Accessor::UNSIGNED_SHORT );
                mesh_draw.indexType = indices_accessor.component_type == glTF::Accessor::UNSIGNED_INT ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;

                glTF::BufferView& indices_buffer_view = scene.buffer_views[ indices_accessor.buffer_view ];
                BufferResource& indices_buffer_gpu = buffers[ indices_accessor.buffer_view ];
                mesh_draw.indexBuffer = indices_buffer_gpu.m_handle;
                mesh_draw.indexOffset = indices_accessor.byte_offset == glTF::INVALID_INT_VALUE ? 0 : indices_accessor.byte_offset;
                mesh_draw.count = indices_accessor.count;
                CASSERT( ( mesh_draw.count % 3 ) == 0 );

                i32 position_accessor_index = gltfGetAttributeAccessorIndex( mesh_primitive.attributes, mesh_primitive.attribute_count, "POSITION" );
                i32 tangent_accessor_index = gltfGetAttributeAccessorIndex( mesh_primitive.attributes, mesh_primitive.attribute_count, "TANGENT" );
                i32 normal_accessor_index = gltfGetAttributeAccessorIndex( mesh_primitive.attributes, mesh_primitive.attribute_count, "NORMAL" );
                i32 texcoord_accessor_index = gltfGetAttributeAccessorIndex( mesh_primitive.attributes, mesh_primitive.attribute_count, "TEXCOORD_0" );

                vec3s* position_data = nullptr;
                u32* index_data_32 = ( u32* )get_buffer_data( scene.buffer_views, indices_accessor.buffer_view, buffersData );
                u16* index_data_16 = ( u16* )index_data_32;
                u32 vertex_count = 0;

                if ( position_accessor_index != -1 ) {
                    glTF::Accessor& position_accessor = scene.accessors[ position_accessor_index ];
                    glTF::BufferView& position_buffer_view = scene.buffer_views[ position_accessor.buffer_view ];
                    BufferResource& position_buffer_gpu = buffers[ position_accessor.buffer_view ];

                    vertex_count = position_accessor.count;

                    mesh_draw.positionBuffer = position_buffer_gpu.m_handle;
                    mesh_draw.positionOffset = position_accessor.byte_offset == glTF::INVALID_INT_VALUE ? 0 : position_accessor.byte_offset;

                    position_data = ( vec3s* )get_buffer_data( scene.buffer_views, position_accessor.buffer_view, buffersData );
                } else {
                    CASSERT( false );
                    continue;
                }

                if ( normal_accessor_index != -1 ) {
                    glTF::Accessor& normal_accessor = scene.accessors[ normal_accessor_index ];
                    glTF::BufferView& normal_buffer_view = scene.buffer_views[ normal_accessor.buffer_view ];
                    BufferResource& normal_buffer_gpu = buffers[ normal_accessor.buffer_view ];

                    mesh_draw.normalBuffer = normal_buffer_gpu.m_handle;
                    mesh_draw.normalOffset = normal_accessor.byte_offset == glTF::INVALID_INT_VALUE ? 0 : normal_accessor.byte_offset;
                } else {
                    // NOTE(marco): we could compute this at runtime
                    Array(vec3s) normals_array(*allocator);
                    normals_array.reserve( vertex_count );
                    normals_array.resize( vertex_count );
                    memset( normals_array.data(), 0, normals_array.size() * sizeof( vec3s ) );

                    u32 index_count = mesh_draw.count;
                    for ( u32 index = 0; index < index_count; index += 3 ) {
                        u32 i0 = indices_accessor.component_type == glTF::Accessor::UNSIGNED_INT ? index_data_32[ index ] : index_data_16[ index ];
                        u32 i1 = indices_accessor.component_type == glTF::Accessor::UNSIGNED_INT ? index_data_32[ index + 1 ] : index_data_16[ index + 1 ];
                        u32 i2 = indices_accessor.component_type == glTF::Accessor::UNSIGNED_INT ? index_data_32[ index + 2 ] : index_data_16[ index + 2 ];

                        vec3s p0 = position_data[ i0 ];
                        vec3s p1 = position_data[ i1 ];
                        vec3s p2 = position_data[ i2 ];

                        vec3s a = glms_vec3_sub( p1, p0 );
                        vec3s b = glms_vec3_sub( p2, p0 );

                        vec3s normal = glms_cross( a, b );

                        normals_array[ i0 ] = glms_vec3_add( normals_array[ i0 ], normal );
                        normals_array[ i1 ] = glms_vec3_add( normals_array[ i1 ], normal );
                        normals_array[ i2 ] = glms_vec3_add( normals_array[ i2 ], normal );
                    }

                    for ( u32 vertex = 0; vertex < vertex_count; ++vertex ) {
                        normals_array[ vertex ] = glms_normalize( normals_array[ vertex ] );
                    }

                    BufferCreation normals_creation{ };
                    normals_creation.Set( VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, ResourceUsageType::Immutable, normals_array.size() * sizeof( vec3s ) ).SetName( "normals" ).SetData( normals_array.data() );

                    mesh_draw.normalBuffer = gpu->create_buffer( normals_creation );
                    mesh_draw.normalOffset = 0;

                    customMeshBuffers.push_back( mesh_draw.normalBuffer );

                    normals_array.clear();
                }

                if ( tangent_accessor_index != -1 ) {
                    glTF::Accessor& tangent_accessor = scene.accessors[ tangent_accessor_index ];
                    glTF::BufferView& tangent_buffer_view = scene.buffer_views[ tangent_accessor.buffer_view ];
                    BufferResource& tangent_buffer_gpu = buffers[ tangent_accessor.buffer_view ];

                    mesh_draw.tangentBuffer = tangent_buffer_gpu.m_handle;
                    mesh_draw.tangentOffset = tangent_accessor.byte_offset == glTF::INVALID_INT_VALUE ? 0 : tangent_accessor.byte_offset;

                    mesh_draw.materialData.flags |= MaterialFeatures_TangentVertexAttribute;
                }

                if ( texcoord_accessor_index != -1 ) {
                    glTF::Accessor& texcoord_accessor = scene.accessors[ texcoord_accessor_index ];
                    glTF::BufferView& texcoord_buffer_view = scene.buffer_views[ texcoord_accessor.buffer_view ];
                    BufferResource& texcoord_buffer_gpu = buffers[ texcoord_accessor.buffer_view ];

                    mesh_draw.texcoordBuffer = texcoord_buffer_gpu.m_handle;
                    mesh_draw.texcoordOffset = texcoord_accessor.byte_offset == glTF::INVALID_INT_VALUE ? 0 : texcoord_accessor.byte_offset;

                    mesh_draw.materialData.flags |= MaterialFeatures_TexcoordVertexAttribute;
                }

                CASSERT( mesh_primitive.material != glTF::INVALID_INT_VALUE );
                glTF::Material& material = scene.materials[ mesh_primitive.material ];

                // Descriptor Set
                DescriptorSetCreation ds_creation{};
                ds_creation.SetLayout( cube_dsl ).Buffer( cube_cb, 0 );

                buffer_creation.Reset().Set( VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, ResourceUsageType::Dynamic, sizeof( MaterialData ) ).SetName( "material" );
                mesh_draw.materialBuffer = gpu->create_buffer( buffer_creation );
                ds_creation.Buffer( mesh_draw.materialBuffer, 1 );

                if ( material.pbr_metallic_roughness != nullptr ) {
                    if ( material.pbr_metallic_roughness->base_color_factor_count != 0 ) {
                        CASSERT( material.pbr_metallic_roughness->base_color_factor_count == 4 );

                        mesh_draw.materialData.baseColorFactor = {
                            material.pbr_metallic_roughness->base_color_factor[0],
                            material.pbr_metallic_roughness->base_color_factor[1],
                            material.pbr_metallic_roughness->base_color_factor[2],
                            material.pbr_metallic_roughness->base_color_factor[3],
                        };
                    } else {
                        mesh_draw.materialData.baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
                    }

                    if ( material.pbr_metallic_roughness->base_color_texture != nullptr ) {
                        glTF::Texture& diffuse_texture = scene.textures[ material.pbr_metallic_roughness->base_color_texture->index ];
                        TextureResource& diffuse_texture_gpu = images[ diffuse_texture.source ];

                        SamplerHandle sampler_handle = dummySampler;
                        if ( diffuse_texture.sampler != glTF::INVALID_INT_VALUE ) {
                            sampler_handle = samplers[ diffuse_texture.sampler ].m_handle;
                        }

                        ds_creation.TextureSampler( diffuse_texture_gpu.m_handle, sampler_handle, 2 );

                        mesh_draw.materialData.flags |= MaterialFeatures_ColorTexture;
                    } else {
                        ds_creation.TextureSampler( dummyTexture, dummySampler, 2 );
                    }

                    if ( material.pbr_metallic_roughness->metallic_roughness_texture != nullptr ) {
                        glTF::Texture& roughness_texture = scene.textures[ material.pbr_metallic_roughness->metallic_roughness_texture->index ];
                        TextureResource& roughness_texture_gpu = images[ roughness_texture.source ];

                        SamplerHandle sampler_handle = dummySampler;
                        if ( roughness_texture.sampler != glTF::INVALID_INT_VALUE ) {
                            sampler_handle = samplers[ roughness_texture.sampler ].m_handle;
                        }

                        ds_creation.TextureSampler( roughness_texture_gpu.m_handle, sampler_handle, 3 );

                        mesh_draw.materialData.flags |= MaterialFeatures_RoughnessTexture;
                    } else {
                        ds_creation.TextureSampler( dummyTexture, dummySampler, 3 );
                    }

                    if ( material.pbr_metallic_roughness->metallic_factor != glTF::INVALID_FLOAT_VALUE ) {
                        mesh_draw.materialData.metallicFactor = material.pbr_metallic_roughness->metallic_factor;
                    } else {
                        mesh_draw.materialData.metallicFactor = 1.0f;
                    }

                    if ( material.pbr_metallic_roughness->roughness_factor != glTF::INVALID_FLOAT_VALUE ) {
                        mesh_draw.materialData.roughnessFactor = material.pbr_metallic_roughness->roughness_factor;
                    } else {
                        mesh_draw.materialData.roughnessFactor = 1.0f;
                    }
                }

                if ( material.occlusion_texture != nullptr ) {
                    glTF::Texture& occlusion_texture = scene.textures[ material.occlusion_texture->index ];

                    // NOTE(marco): this could be the same as the roughness texture, but for now we treat it as a separate
                    // texture
                    TextureResource& occlusion_texture_gpu = images[ occlusion_texture.source ];

                    SamplerHandle sampler_handle = dummySampler;
                    if ( occlusion_texture.sampler != glTF::INVALID_INT_VALUE ) {
                        sampler_handle = samplers[ occlusion_texture.sampler ].m_handle;
                    }

                    ds_creation.TextureSampler( occlusion_texture_gpu.m_handle, sampler_handle, 4 );

                    mesh_draw.materialData.occlusionFactor = material.occlusion_texture->strength != glTF::INVALID_FLOAT_VALUE ? material.occlusion_texture->strength : 1.0f;
                    mesh_draw.materialData.flags |= MaterialFeatures_OcclusionTexture;
                } else {
                    mesh_draw.materialData.occlusionFactor = 1.0f;
                    ds_creation.TextureSampler( dummyTexture, dummySampler, 4 );
                }

                if ( material.emissive_factor_count != 0 ) {
                    mesh_draw.materialData.emissiveFactor = vec3s{
                        material.emissive_factor[ 0 ],
                        material.emissive_factor[ 1 ],
                        material.emissive_factor[ 2 ],
                    };
                }

                if ( material.emissive_texture != nullptr ) {
                    glTF::Texture& emissive_texture = scene.textures[ material.emissive_texture->index ];

                    // NOTE(marco): this could be the same as the roughness texture, but for now we treat it as a separate
                    // texture
                    TextureResource& emissive_texture_gpu = images[ emissive_texture.source ];

                    SamplerHandle sampler_handle = dummySampler;
                    if ( emissive_texture.sampler != glTF::INVALID_INT_VALUE ) {
                        sampler_handle = samplers[ emissive_texture.sampler ].m_handle;
                    }

                    ds_creation.TextureSampler( emissive_texture_gpu.m_handle, sampler_handle, 5 );

                    mesh_draw.materialData.flags |= MaterialFeatures_EmissiveTexture;
                } else {
                    ds_creation.TextureSampler( dummyTexture, dummySampler, 5 );
                }

                if ( material.normal_texture != nullptr ) {
                    glTF::Texture& normal_texture = scene.textures[ material.normal_texture->index ];
                    TextureResource& normal_texture_gpu = images[ normal_texture.source ];

                    SamplerHandle sampler_handle = dummySampler;
                    if ( normal_texture.sampler != glTF::INVALID_INT_VALUE ) {
                        sampler_handle = samplers[ normal_texture.sampler ].m_handle;
                    }

                    ds_creation.TextureSampler( normal_texture_gpu.m_handle, sampler_handle, 6 );

                    mesh_draw.materialData.flags |= MaterialFeatures_NormalTexture;
                } else {
                    ds_creation.TextureSampler( dummyTexture, dummySampler, 6 );
                }

                mesh_draw.descriptorSet = gpu->create_descriptor_set( ds_creation );

                meshDraws.push_back( mesh_draw );
            }
        }

        node_parents.clear();
        node_stack.clear();
        node_matrix.clear();
    }

    auto rx = 0.0f;
    auto ry = 0.0f;

    for ( u32 buffer_index = 0; buffer_index < scene.buffers_count; ++buffer_index ) {
        void* buffer = buffersData[ buffer_index ];
        allocator->deallocate( buffer );
    }

    buffersData.clear();

    auto beginFrameTick = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );

    vec3s eye = vec3s{ 0.0f, 2.5f, 2.0f };
    vec3s look = vec3s{ 0.0f, 0.0, -1.0f };
    vec3s right = vec3s{ 1.0f, 0.0, 0.0f };

    f32 yaw = 0.0f;
    f32 pitch = 0.0f;

    float model_scale = 1.0f;

    while ( !window->m_requestedExit ) {
        ZoneScoped;

        // New frame
        if ( !window->m_minimized ) {
            gpu->new_frame();
        }
        //input->new_frame();

        window->HandleOsMessages();

        if ( window->m_resized ) {
            //renderer->resize_swapchain( window.width, window.height );
            //on_resize( window.width, window.height );
            gpu->resize( window->m_width, window->m_height );
            window->m_resized = false;
        }
        // This MUST be AFTER os messages!
        imGuiService->NewFrame();

        auto currentTick = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
        f32 deltaTime = static_cast<f32>(std::difftime( beginFrameTick, currentTick ));
        beginFrameTick = currentTick;

        inputHandler->NewFrame();
        inputHandler->Update( deltaTime );

        if ( ImGui::Begin( "Caustix ImGui" ) ) {
            ImGui::InputFloat("Model scale", &model_scale, 0.001f);
        }
        ImGui::End();

        if ( ImGui::Begin( "GPU" ) ) {
            gpuProfiler.ImguiDraw();
        }
        ImGui::End();

        mat4s global_model = { };
        {
            // Update rotating cube gpu data
            MapBufferParameters cb_map = { cube_cb, 0, 0 };
            float* cb_data = ( float* )gpu->map_buffer( cb_map );
            if ( cb_data ) {
                if ( inputHandler->IsMouseDown( MouseButtons::MOUSE_BUTTONS_LEFT ) && !ImGui::GetIO().WantCaptureMouse) {
                    pitch += ( inputHandler->m_mousePosition.m_y - inputHandler->m_previousMousePosition.m_y ) * 0.1f;
                    yaw += ( inputHandler->m_mousePosition.m_x - inputHandler->m_previousMousePosition.m_x ) * 0.3f;

                    pitch = clamp( pitch, -60.0f, 60.0f );

                    if ( yaw > 360.0f ) {
                        yaw -= 360.0f;
                    }

                    mat3s rxm = glms_mat4_pick3( glms_rotate_make( glm_rad( -pitch ), vec3s{ 1.0f, 0.0f, 0.0f } ) );
                    mat3s rym = glms_mat4_pick3( glms_rotate_make( glm_rad( -yaw ), vec3s{ 0.0f, 1.0f, 0.0f } ) );

                    look = glms_mat3_mulv( rxm, vec3s{ 0.0f, 0.0f, -1.0f } );
                    look = glms_mat3_mulv( rym, look );

                    right = glms_cross( look, vec3s{ 0.0f, 1.0f, 0.0f });
                }

                if ( inputHandler->IsKeyDown( Keys::KEY_W ) ) {
                    eye = glms_vec3_add( eye, glms_vec3_scale( look, 5.0f * deltaTime ) );
                } else if ( inputHandler->IsKeyDown( Keys::KEY_S ) ) {
                    eye = glms_vec3_sub( eye, glms_vec3_scale( look, 5.0f * deltaTime ) );
                }

                if ( inputHandler->IsKeyDown( Keys::KEY_D ) ) {
                    eye = glms_vec3_add( eye, glms_vec3_scale( right, 5.0f * deltaTime ) );
                } else if ( inputHandler->IsKeyDown( Keys::KEY_A ) ) {
                    eye = glms_vec3_sub( eye, glms_vec3_scale( right, 5.0f * deltaTime ) );
                }

                mat4s view = glms_lookat( eye, glms_vec3_add( eye, look ), vec3s{ 0.0f, 1.0f, 0.0f } );
                mat4s projection = glms_perspective( glm_rad( 60.0f ), gpu->swapchain_width * 1.0f / gpu->swapchain_height, 0.01f, 1000.0f );

                // Calculate view projection matrix
                mat4s view_projection = glms_mat4_mul( projection, view );

                // Rotate cube:
                rx += 1.0f * deltaTime;
                ry += 2.0f * deltaTime;

                mat4s rxm = glms_rotate_make( rx, vec3s{ 1.0f, 0.0f, 0.0f } );
                mat4s rym = glms_rotate_make( glm_rad( 45.0f ), vec3s{ 0.0f, 1.0f, 0.0f } );

                mat4s sm = glms_scale_make( vec3s{ model_scale, model_scale, model_scale } );
                global_model = glms_mat4_mul( rym, sm );

                UniformData uniform_data{ };
                uniform_data.vp = view_projection;
                uniform_data.m = global_model;
                uniform_data.eye = vec4s{ eye.x, eye.y, eye.z, 1.0f };
                uniform_data.light = vec4s{ 2.0f, 2.0f, 0.0f, 1.0f };

                memcpy( cb_data, &uniform_data, sizeof( UniformData ) );

                gpu->unmap_buffer( cb_map );
            }
        }

        if ( !window->m_minimized ) {
            CommandBuffer* gpu_commands = gpu->get_command_buffer( QueueType::Graphics, true );
            gpu_commands->PushMarker( "Frame" );

            gpu_commands->clear( 0.3f, 0.9f, 0.3f, 1.0f );
            gpu_commands->ClearDepthStencil( 1.0f, 0 );
            gpu_commands->BindPass( gpu->get_swapchain_pass() );
            gpu_commands->BindPipeline( cube_pipeline );
            gpu_commands->SetScissor( nullptr );
            gpu_commands->SetViewport( nullptr );

            for ( u32 mesh_index = 0; mesh_index < meshDraws.size(); ++mesh_index ) {
                MeshDraw mesh_draw = meshDraws[ mesh_index ];
                mesh_draw.materialData.modelInv = glms_mat4_inv( glms_mat4_transpose( glms_mat4_mul( global_model, mesh_draw.materialData.model ) ) );

                MapBufferParameters material_map = { mesh_draw.materialBuffer, 0, 0 };
                MaterialData* material_buffer_data = ( MaterialData* )gpu->map_buffer( material_map );

                memcpy( material_buffer_data, &mesh_draw.materialData, sizeof( MaterialData ) );

                gpu->unmap_buffer( material_map );

                gpu_commands->BindVertexBuffer( mesh_draw.positionBuffer, 0, mesh_draw.positionOffset );
                gpu_commands->BindVertexBuffer( mesh_draw.normalBuffer, 2, mesh_draw.normalOffset );

                if ( mesh_draw.materialData.flags & MaterialFeatures_TangentVertexAttribute ) {
                    gpu_commands->BindVertexBuffer( mesh_draw.tangentBuffer, 1, mesh_draw.tangentOffset );
                } else {
                    gpu_commands->BindVertexBuffer( dummyAttributeBuffer, 1, 0 );
                }

                if ( mesh_draw.materialData.flags & MaterialFeatures_TexcoordVertexAttribute ) {
                    gpu_commands->BindVertexBuffer( mesh_draw.texcoordBuffer, 3, mesh_draw.texcoordOffset );
                } else {
                    gpu_commands->BindVertexBuffer( dummyAttributeBuffer, 3, 0 );
                }

                gpu_commands->BindIndexBuffer( mesh_draw.indexBuffer, mesh_draw.indexOffset, mesh_draw.indexType );
                gpu_commands->BindDescriptorSet( &mesh_draw.descriptorSet, 1, nullptr, 0 );

                gpu_commands->DrawIndexed( TopologyType::Triangle, mesh_draw.count, 1, 0, 0, 0 );
            }

            imGuiService->Render( *gpu_commands );

            gpu_commands->PopMarker();

            gpuProfiler.Update( *gpu );

            // Send commands to GPU
            gpu->queue_command_buffer( gpu_commands );
            gpu->present();

        } else {
            ImGui::Render();
        }

        FrameMark;
    }

    for ( u32 mesh_index = 0; mesh_index < meshDraws.size(); ++mesh_index ) {
        MeshDraw& mesh_draw = meshDraws[ mesh_index ];
        gpu->destroy_descriptor_set( mesh_draw.descriptorSet );
        gpu->destroy_buffer( mesh_draw.materialBuffer );
    }

    for ( u32 mi = 0; mi < customMeshBuffers.size(); ++mi ) {
        gpu->destroy_buffer( customMeshBuffers[ mi ] );
    }
    customMeshBuffers.clear();

    gpu->destroy_buffer( dummyAttributeBuffer );

    gpu->destroy_texture( dummyTexture );
    gpu->destroy_sampler( dummySampler );

    meshDraws.clear();

    gpu->destroy_buffer( cube_cb );
    gpu->destroy_descriptor_set_layout( cube_dsl );
    gpu->destroy_pipeline( cube_pipeline );

    imGuiService->Shutdown();
    renderer->Shutdown();

    samplers.clear();
    images.clear();
    buffers.clear();

    resourceNameBuffer.clear();

    window->UnregisterOsMessagesCallback(InputOsMessagesCallback);
    return 0;
}
