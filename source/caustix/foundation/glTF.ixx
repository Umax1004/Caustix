module;

#include "nlohmann/json.hpp"

#include <string>
#include <filesystem>

export module Foundation.glTF;

import Foundation.Memory.MemoryDefines;
import Foundation.Memory.Allocators.Allocator;
import Foundation.Memory.Allocators.LinearAllocator;
import Foundation.Services.MemoryService;
import Foundation.Services.ServiceManager;
import Foundation.Assert;
import Foundation.Log;
import Foundation.Platform;
import Foundation.File;

export namespace Caustix {

    void InjectDefault3DModel(int& argc, char** argv) {
        static const char* kDefault3DModel = "../../../Models/2.0/Sponza/glTF/Sponza.gltf";
        if (std::filesystem::exists(kDefault3DModel)) {
            argc = 2;
            argv[1] = const_cast<char*>(kDefault3DModel);
        }
        else {
           exit(-1);
        }
    }

    using StringBuffer = std::basic_string<char, std::char_traits<char>, STLAdaptor<char>>;

    namespace glTF {
        constexpr i32 INVALID_INT_VALUE = 2147483647;
        static_assert(INVALID_INT_VALUE == i32_max, "Mismatch between invalid int and i32 max");
        constexpr f32 INVALID_FLOAT_VALUE = 3.402823466e+38F;

        struct Asset {
            StringBuffer copyright;
            StringBuffer generator;
            StringBuffer minVersion;
            StringBuffer version;
        };

        struct CameraOrthographic {
            f32                         xmag;
            f32                         ymag;
            f32                         zfar;
            f32                         znear;
        };

        struct AccessorSparse {
            i32                         count;
            i32                         indices;
            i32                         values;
        };

        struct Camera {
            i32                         orthographic;
            i32                         perspective;
            // perspective
            // orthographic
            StringBuffer                 type;
        };

        struct AnimationChannel {

            enum TargetType {
                Translation, Rotation, Scale, Weights, Count
            };

            i32                         sampler;
            i32                         target_node;
            TargetType                  target_type;
        };

        struct AnimationSampler {
            i32                         input_keyframe_buffer_index;  //"The index of an accessor containing keyframe timestamps."
            i32                         output_keyframe_buffer_index; // "The index of an accessor, containing keyframe output values."

            enum Interpolation {
                Linear, Step, CubicSpline, Count
            };
            // LINEAR The animated values are linearly interpolated between keyframes. When targeting a rotation, spherical linear interpolation (slerp) **SHOULD** be used to interpolate quaternions. The float of output elements **MUST** equal the float of input elements.
            // STEP The animated values remain constant to the output of the first keyframe, until the next keyframe. The float of output elements **MUST** equal the float of input elements.
            // CUBICSPLINE The animation's interpolation is computed using a cubic spline with specified tangents. The float of output elements **MUST** equal three times the float of input elements. For each input element, the output stores three elements, an in-tangent, a spline vertex, and an out-tangent. There **MUST** be at least two keyframes when using this interpolation.
            Interpolation               interpolation;
        };

        struct Skin {
            i32                         inverse_bind_matrices_buffer_index;
            i32                         skeleton_root_node_index;
            u32                         joints_count;
            i32*                        joints;
        };

        struct BufferView {
            enum Target {
                ARRAY_BUFFER = 34962 /* Vertex Data */, ELEMENT_ARRAY_BUFFER = 34963 /* Index Data */
            };

            i32                         buffer;
            i32                         byte_length;
            i32                         byte_offset;
            i32                         byte_stride;
            i32                         target;
            StringBuffer                 name;
        };

        struct Image {
            i32                         buffer_view;
            // image/jpeg
            // image/png
            StringBuffer                 mime_type;
            StringBuffer                 uri;
        };

        struct Node {
            i32                         camera;
            u32                         children_count;
            i32*                        children;
            u32                         matrix_count;
            f32*                        matrix;
            i32                         mesh;
            u32                         rotation_count;
            f32*                        rotation;
            u32                         scale_count;
            f32*                        scale;
            i32                         skin;
            u32                         translation_count;
            f32*                        translation;
            u32                         weights_count;
            f32*                        weights;
            StringBuffer                 name;
        };

        struct TextureInfo {
            i32                         index;
            i32                         texCoord;
        };

        struct MaterialPBRMetallicRoughness {
            u32                         base_color_factor_count;
            f32*                        base_color_factor;
            TextureInfo*                base_color_texture;
            f32                         metallic_factor;
            TextureInfo*                metallic_roughness_texture;
            f32                         roughness_factor;
        };

        struct MeshPrimitive {
            struct Attribute {
                StringBuffer             key;
                i32                     accessor_index;

                Attribute(Allocator& allocator) : key(allocator) {}
            };

            u32                         attribute_count;
            Attribute*                  attributes;
            i32                         indices;
            i32                         material;
            // 0 POINTS
            // 1 LINES
            // 2 LINE_LOOP
            // 3 LINE_STRIP
            // 4 TRIANGLES
            // 5 TRIANGLE_STRIP
            // 6 TRIANGLE_FAN
            i32                         mode;
            // u32 targets_count;
            // object* targets; // TODO(marco): this is a json object
        };

        struct AccessorSparseIndices {
            i32                         buffer_view;
            i32                         byte_offset;
            // 5121 UNSIGNED_BYTE
            // 5123 UNSIGNED_SHORT
            // 5125 UNSIGNED_INT
            i32                         component_type;
        };

        struct Accessor {
            enum ComponentType {
                BYTE = 5120, UNSIGNED_BYTE = 5121, SHORT = 5122, UNSIGNED_SHORT = 5123, UNSIGNED_INT = 5125, FLOAT = 5126
            };

            enum Type {
                Scalar, Vec2, Vec3, Vec4, Mat2, Mat3, Mat4
            };

            i32                         buffer_view;
            i32                         byte_offset;

            i32                         component_type;
            i32                         count;
            u32                         max_count;
            f32*                        max;
            u32                         min_count;
            f32*                        min;
            bool                        normalized;
            i32                         sparse;
            Type                        type;
        };

        struct Texture {
            i32                         sampler;
            i32                         source;
            StringBuffer                 name;
        };

        struct MaterialNormalTextureInfo {
            i32                         index;
            i32                         tex_coord;
            f32                         scale;
        };

        struct Mesh {
            u32                         primitives_count;
            MeshPrimitive*              primitives;
            u32                         weights_count;
            f32*                        weights;
            StringBuffer                 name;
        };

        struct MaterialOcclusionTextureInfo {
            i32                         index;
            i32                         texCoord;
            f32                         strength;
        };

        struct Material {
            f32                         alpha_cutoff;
            // OPAQUE The alpha value is ignored, and the rendered output is fully opaque.
            // MASK The rendered output is either fully opaque or fully transparent depending on the alpha value and the specified `alphaCutoff` value; the exact appearance of the edges **MAY** be subject to implementation-specific techniques such as "`Alpha-to-Coverage`".
            // BLEND The alpha value is used to composite the source and destination areas. The rendered output is combined with the background using the normal painting operation (i.e. the Porter and Duff over operator).
            StringBuffer                 alpha_mode;
            bool                        double_sided;
            u32                         emissive_factor_count;
            f32*                        emissive_factor;
            TextureInfo*                emissive_texture;
            MaterialNormalTextureInfo*  normal_texture;
            MaterialOcclusionTextureInfo* occlusion_texture;
            MaterialPBRMetallicRoughness* pbr_metallic_roughness;
            StringBuffer                 name;
        };

        struct Buffer {
            i32                         byte_length;
            StringBuffer                 uri;
            StringBuffer                 name;
        };

        struct CameraPerspective {
            f32                         aspect_ratio;
            f32                         yfov;
            f32                         zfar;
            f32                         znear;
        };

        struct Animation {
            u32                         channels_count;
            AnimationChannel*           channels;
            u32                         samplers_count;
            AnimationSampler*           samplers;
        };

        struct AccessorSparseValues {
            i32                         bufferView;
            i32                         byteOffset;
        };

        struct Scene {
            u32                         nodes_count;
            i32*                        nodes;
        };

        struct Sampler {
            enum Filter {
                NEAREST = 9728, LINEAR = 9729, NEAREST_MIPMAP_NEAREST = 9984, LINEAR_MIPMAP_NEAREST = 9985, NEAREST_MIPMAP_LINEAR = 9986, LINEAR_MIPMAP_LINEAR = 9987
            };

            enum Wrap {
                CLAMP_TO_EDGE = 33071, MIRRORED_REPEAT = 33648, REPEAT = 10497
            };

            i32                         mag_filter;
            i32                         min_filter;
            i32                         wrap_s;
            i32                         wrap_t;
        };

        struct glTF {
            u32                         accessors_count;
            Accessor*                   accessors;
            u32                         animations_count;
            Animation*                  animations;
            Asset*                      asset;
            u32                         buffer_views_count;
            BufferView*                 buffer_views;
            u32                         buffers_count;
            Buffer*                     buffers;
            u32                         cameras_count;
            Camera*                     cameras;
            u32                         extensions_required_count;
            StringBuffer*                extensions_required;
            u32                         extensions_used_count;
            StringBuffer*                extensions_used;
            u32                         images_count;
            Image*                      images;
            u32                         materials_count;
            Material*                   materials;
            u32                         meshes_count;
            Mesh*                       meshes;
            u32                         nodes_count;
            Node*                       nodes;
            u32                         samplers_count;
            Sampler*                    samplers;
            i32                         scene;
            u32                         scenes_count;
            Scene*                      scenes;
            u32                         skins_count;
            Skin*                       skins;
            u32                         textures_count;
            Texture*                    textures;

            Allocator*                  allocator;

            glTF(Allocator* allocator_) : allocator(allocator_) {}
        };

        i32                             GetDataOffset(i32 accessorOffset, i32 bufferViewOffset);
    }

    glTF::glTF  gltfLoadFile( cstring filePath, Allocator* allocator );

    i32         gltfGetAttributeAccessorIndex( glTF::MeshPrimitive::Attribute* attributes, u32 attributeCount, cstring attributeName );
}

namespace Caustix {
    using json = nlohmann::json;

    static void* AllocateAndZero(Allocator* allocator, sizet size) {
        void* result = allocator->allocate( size, 64 );
        memset( result, 0, size );

        return result;
    }

    static void TryLoadString(json& jsonData, cstring key, StringBuffer& stringBuffer, Allocator* allocator) {
        auto it = jsonData.find( key );
        if ( it == jsonData.end() )
            return;

        std::string value = jsonData.value( key, "" );

        StringBuffer tempBuffer(*allocator);

        memcpy(&stringBuffer, &tempBuffer, sizeof (StringBuffer));

        stringBuffer.append( value );
    }

    static void TryLoadInt(json& json_data, cstring key, i32& value) {
        auto it = json_data.find(key);
        if (it == json_data.end())
        {
            value = glTF::INVALID_INT_VALUE;
            return;
        }

        value = json_data.value(key, 0);
    }

    static void TryLoadFloat( json& jsonData, cstring key, f32& value ) {
        auto it = jsonData.find( key );
        if ( it == jsonData.end() )
        {
            value = glTF::INVALID_FLOAT_VALUE;
            return;
        }

        value = jsonData.value( key, 0.0f );
    }

    static void TryLoadBool( json& jsonData, cstring key, bool& value ) {
        auto it = jsonData.find( key );
        if ( it == jsonData.end() )
        {
            value = false;
            return;
        }

        value = jsonData.value( key, false );
    }

    static void TryLoadType( json& jsonData, cstring key, glTF::Accessor::Type& type ) {
        std::string value = jsonData.value( key, "" );
        if ( value == "SCALAR" ) {
            type = glTF::Accessor::Type::Scalar;
        }
        else if ( value == "VEC2" ) {
            type = glTF::Accessor::Type::Vec2;
        }
        else if ( value == "VEC3" ) {
            type = glTF::Accessor::Type::Vec3;
        }
        else if ( value == "VEC4" ) {
            type = glTF::Accessor::Type::Vec4;
        }
        else if ( value == "MAT2" ) {
            type = glTF::Accessor::Type::Mat2;
        }
        else if ( value == "MAT3" ) {
            type = glTF::Accessor::Type::Mat3;
        }
        else if ( value == "MAT4" ) {
            type = glTF::Accessor::Type::Mat4;
        }
        else {
            CASSERT( false );
        }
    }

    static void TryLoadIntArray( json& jsonData, cstring key, u32& count, i32** array, Allocator* allocator ) {
        auto it = jsonData.find( key );
        if ( it == jsonData.end() ) {
            count = 0;
            *array = nullptr;
            return;
        }

        json jsonArray = jsonData.at( key );

        count = jsonArray.size();

        i32* values = ( i32* )AllocateAndZero( allocator, sizeof( i32 ) * count );

        for ( sizet i = 0; i < count; ++i ) {
            values[ i ] = jsonArray.at( i );
        }

        *array = values;
    }

    static void TryLoadFloatArray( json& jsonData, cstring key, u32& count, float** array, Allocator* allocator ) {
        auto it = jsonData.find( key );
        if ( it == jsonData.end() ) {
            count = 0;
            *array = nullptr;
            return;
        }

        json json_array = jsonData.at( key );

        count = json_array.size();

        float* values = ( float* )AllocateAndZero( allocator, sizeof( float ) * count );

        for ( sizet i = 0; i < count; ++i ) {
            values[ i ] = json_array.at( i );
        }

        *array = values;
    }

    static void LoadScene(json& jsonData, glTF::Scene& scene, Allocator* allocator) {
        TryLoadIntArray(jsonData, "nodes", scene.nodes_count, &scene.nodes, allocator);
    }

    static void LoadScenes(json& jsonData, glTF::glTF& gltfData, Allocator* allocator) {
        json scenes = jsonData[ "scenes" ];

        sizet scene_count = scenes.size();
        gltfData.scenes = ( glTF::Scene* )AllocateAndZero( allocator, sizeof( glTF::Scene ) * scene_count );
        gltfData.scenes_count = scene_count;

        for ( sizet i = 0; i < scene_count; ++i ) {
            LoadScene( scenes[ i ], gltfData.scenes[ i ], allocator );
        }
    }

    static void LoadBuffer( json& jsonData, glTF::Buffer& buffer, Allocator* allocator ) {
        TryLoadString( jsonData, "uri", buffer.uri, allocator );
        TryLoadInt( jsonData, "byteLength", buffer.byte_length );
        TryLoadString( jsonData, "name", buffer.name, allocator );
    }

    static void LoadBuffers( json& jsonData, glTF::glTF& gltfData, Allocator* allocator ) {
        json buffers = jsonData[ "buffers" ];

        sizet buffer_count = buffers.size();
        gltfData.buffers = ( glTF::Buffer* )AllocateAndZero( allocator, sizeof( glTF::Buffer ) * buffer_count );
        gltfData.buffers_count = buffer_count;

        for ( sizet i = 0; i < buffer_count; ++i ) {
            LoadBuffer( buffers[ i ], gltfData.buffers[ i ], allocator );
        }
    }

    static void LoadBufferView( json& jsonData, glTF::BufferView& bufferView, Allocator* allocator ) {
        TryLoadInt( jsonData, "buffer", bufferView.buffer );
        TryLoadInt( jsonData, "byteLength", bufferView.byte_length );
        TryLoadInt( jsonData, "byteOffset", bufferView.byte_offset );
        TryLoadInt( jsonData, "byteStride", bufferView.byte_stride );
        TryLoadInt( jsonData, "target", bufferView.target );
        TryLoadString( jsonData, "name", bufferView.name, allocator );
    }

    static void LoadBufferViews( json& jsonData, glTF::glTF& gltfData, Allocator* allocator ) {
        json buffers = jsonData[ "bufferViews" ];

        sizet buffer_count = buffers.size();
        gltfData.buffer_views = ( glTF::BufferView* )AllocateAndZero( allocator, sizeof( glTF::BufferView ) * buffer_count );
        gltfData.buffer_views_count = buffer_count;

        for ( sizet i = 0; i < buffer_count; ++i ) {
            LoadBufferView( buffers[ i ], gltfData.buffer_views[ i ], allocator );
        }
    }

    static void LoadNode( json& jsonData, glTF::Node& node, Allocator* allocator ) {
        TryLoadInt( jsonData, "camera", node.camera );
        TryLoadInt( jsonData, "mesh", node.mesh );
        TryLoadInt( jsonData, "skin", node.skin );
        TryLoadIntArray( jsonData, "children", node.children_count, &node.children, allocator );
        TryLoadFloatArray( jsonData, "matrix", node.matrix_count, &node.matrix, allocator );
        TryLoadFloatArray( jsonData, "rotation", node.rotation_count, &node.rotation, allocator );
        TryLoadFloatArray( jsonData, "scale", node.scale_count, &node.scale, allocator );
        TryLoadFloatArray( jsonData, "translation", node.translation_count, &node.translation, allocator );
        TryLoadFloatArray( jsonData, "weights", node.weights_count, &node.weights, allocator );
        TryLoadString( jsonData, "name", node.name, allocator );
    }

    static void LoadNodes( json& jsonData, glTF::glTF& gltfData, Allocator* allocator ) {
        json array = jsonData[ "nodes" ];

        sizet array_count = array.size();
        gltfData.nodes = ( glTF::Node* )AllocateAndZero( allocator, sizeof( glTF::Node ) * array_count );
        gltfData.nodes_count = array_count;

        for ( sizet i = 0; i < array_count; ++i ) {
            LoadNode( array[ i ], gltfData.nodes[ i ], allocator );
        }
    }

    static void LoadMeshPrimitive( json& jsonData, glTF::MeshPrimitive& meshPrimitive, Allocator* allocator ) {
        TryLoadInt( jsonData, "indices", meshPrimitive.indices );
        TryLoadInt( jsonData, "material", meshPrimitive.material );
        TryLoadInt( jsonData, "mode", meshPrimitive.mode );

        json attributes = jsonData[ "attributes" ];

        meshPrimitive.attributes = ( glTF::MeshPrimitive::Attribute* )AllocateAndZero( allocator, sizeof( glTF::MeshPrimitive::Attribute ) * attributes.size() );
        meshPrimitive.attribute_count = attributes.size();

        u32 index = 0;
        for ( auto json_attribute : attributes.items() ) {
            std::string key = json_attribute.key();
            glTF::MeshPrimitive::Attribute& attribute = meshPrimitive.attributes[ index ];

            StringBuffer tempBuffer(*allocator);

            memcpy(&attribute.key, &tempBuffer, sizeof (StringBuffer));

            attribute.key.append( key.c_str() );

            attribute.accessor_index = json_attribute.value();

            ++index;
        }
    }

    static void LoadMeshPrimitives( json& jsonData, glTF::Mesh& mesh, Allocator* allocator ) {
        json array = jsonData[ "primitives" ];

        sizet array_count = array.size();
        mesh.primitives = ( glTF::MeshPrimitive* )AllocateAndZero( allocator, sizeof( glTF::MeshPrimitive ) * array_count );
        mesh.primitives_count = array_count;

        for ( sizet i = 0; i < array_count; ++i ) {
            LoadMeshPrimitive( array[ i ], mesh.primitives[ i ], allocator );
        }
    }

    static void LoadMesh( json& jsonData, glTF::Mesh& mesh, Allocator* allocator ) {
        LoadMeshPrimitives( jsonData, mesh, allocator );
        TryLoadFloatArray( jsonData, "weights", mesh.weights_count, &mesh.weights, allocator );
        TryLoadString( jsonData, "name", mesh.name, allocator );
    }

    static void LoadMeshes( json& jsonData, glTF::glTF& gltfData, Allocator* allocator ) {
        json array = jsonData[ "meshes" ];

        sizet array_count = array.size();
        gltfData.meshes = ( glTF::Mesh* )AllocateAndZero( allocator, sizeof( glTF::Mesh ) * array_count );
        gltfData.meshes_count = array_count;

        for ( sizet i = 0; i < array_count; ++i ) {
            LoadMesh( array[ i ], gltfData.meshes[ i ], allocator );
        }
    }

    static void LoadAccessor( json& jsonData, glTF::Accessor& accessor, Allocator* allocator ) {
        TryLoadInt( jsonData, "bufferView", accessor.buffer_view );
        TryLoadInt( jsonData, "byteOffset", accessor.byte_offset );
        TryLoadInt( jsonData, "componentType", accessor.component_type );
        TryLoadInt( jsonData, "count", accessor.count );
        TryLoadInt( jsonData, "sparse", accessor.sparse );
        TryLoadFloatArray( jsonData, "max", accessor.max_count, &accessor.max, allocator );
        TryLoadFloatArray( jsonData, "min", accessor.min_count, &accessor.min, allocator );
        TryLoadBool( jsonData, "normalized", accessor.normalized );
        TryLoadType( jsonData, "type", accessor.type );
    }

    static void LoadAccessors( json& jsonData, glTF::glTF& gltfData, Allocator* allocator ) {
        json array = jsonData[ "accessors" ];

        sizet array_count = array.size();
        gltfData.accessors = ( glTF::Accessor* )AllocateAndZero( allocator, sizeof( glTF::Accessor ) * array_count );
        gltfData.accessors_count = array_count;

        for ( sizet i = 0; i < array_count; ++i ) {
            LoadAccessor( array[ i ], gltfData.accessors[ i ], allocator );
        }
    }

    static void TryLoadTextureInfo( json& jsonData, cstring key, glTF::TextureInfo** texture_info, Allocator* allocator ) {
        auto it = jsonData.find( key );
        if ( it == jsonData.end() ) {
            *texture_info = nullptr;
            return;
        }

        glTF::TextureInfo* ti = ( glTF::TextureInfo* ) allocator->allocate( sizeof( glTF::TextureInfo ), 64 );

        TryLoadInt( *it, "index", ti->index );
        TryLoadInt( *it, "texCoord", ti->texCoord );

        *texture_info = ti;
    }

    static void TryLoadMaterialNormalTextureInfo( json& jsonData, cstring key, glTF::MaterialNormalTextureInfo** texture_info, Allocator* allocator ) {
        auto it = jsonData.find( key );
        if ( it == jsonData.end() ) {
            *texture_info = nullptr;
            return;
        }

        glTF::MaterialNormalTextureInfo* ti = ( glTF::MaterialNormalTextureInfo* ) allocator->allocate( sizeof( glTF::MaterialNormalTextureInfo ), 64 );

        TryLoadInt( *it, "index", ti->index );
        TryLoadInt( *it, "texCoord", ti->tex_coord );
        TryLoadFloat( *it, "scale", ti->scale );

        *texture_info = ti;
    }

    static void TryLoadMaterialOcclusionTextureInfo( json& jsonData, cstring key, glTF::MaterialOcclusionTextureInfo** texture_info, Allocator* allocator ) {
        auto it = jsonData.find( key );
        if ( it == jsonData.end() ) {
            *texture_info = nullptr;
            return;
        }

        glTF::MaterialOcclusionTextureInfo* ti = ( glTF::MaterialOcclusionTextureInfo* ) allocator->allocate( sizeof( glTF::MaterialOcclusionTextureInfo ), 64 );

        TryLoadInt( *it, "index", ti->index );
        TryLoadInt( *it, "texCoord", ti->texCoord );
        TryLoadFloat( *it, "strength", ti->strength );

        *texture_info = ti;
    }

    static void TryLoadMaterialPBRMetallicRoughness( json& jsonData, cstring key, glTF::MaterialPBRMetallicRoughness** texture_info, Allocator* allocator ) {
        auto it = jsonData.find( key );
        if ( it == jsonData.end() )
        {
            *texture_info = nullptr;
            return;
        }

        glTF::MaterialPBRMetallicRoughness* ti = ( glTF::MaterialPBRMetallicRoughness* ) allocator->allocate( sizeof( glTF::MaterialPBRMetallicRoughness ), 64 );

        TryLoadFloatArray( *it, "baseColorFactor", ti->base_color_factor_count, &ti->base_color_factor, allocator );
        TryLoadTextureInfo( *it, "baseColorTexture", &ti->base_color_texture, allocator );
        TryLoadFloat( *it, "metallicFactor", ti->metallic_factor );
        TryLoadTextureInfo( *it, "metallicRoughnessTexture", &ti->metallic_roughness_texture, allocator );
        TryLoadFloat( *it, "roughnessFactor", ti->roughness_factor );

        *texture_info = ti;
    }

    static void load_material( json& json_data, glTF::Material& material, Allocator* allocator ) {
        TryLoadFloatArray( json_data, "emissiveFactor", material.emissive_factor_count, &material.emissive_factor, allocator );
        TryLoadFloat( json_data, "alphaCutoff", material.alpha_cutoff );
        TryLoadString( json_data, "alphaMode", material.alpha_mode, allocator );
        TryLoadBool( json_data, "doubleSided", material.double_sided );

        TryLoadTextureInfo( json_data, "emissiveTexture", &material.emissive_texture, allocator );
        TryLoadMaterialNormalTextureInfo( json_data, "normalTexture", &material.normal_texture, allocator );
        TryLoadMaterialOcclusionTextureInfo( json_data, "occlusionTexture", &material.occlusion_texture, allocator );
        TryLoadMaterialPBRMetallicRoughness( json_data, "pbrMetallicRoughness", &material.pbr_metallic_roughness, allocator );

        TryLoadString( json_data, "name", material.name, allocator );
    }

    static void LoadMaterials( json& jsonData, glTF::glTF& gltfData, Allocator* allocator ) {
        json array = jsonData[ "materials" ];

        sizet array_count = array.size();
        gltfData.materials = ( glTF::Material* )AllocateAndZero( allocator, sizeof( glTF::Material ) * array_count );
        gltfData.materials_count = array_count;

        for ( sizet i = 0; i < array_count; ++i ) {
            load_material( array[ i ], gltfData.materials[ i ], allocator );
        }
    }

    static void LoadTexture( json& jsonData, glTF::Texture& texture, Allocator* allocator ) {
        TryLoadInt( jsonData, "sampler", texture.sampler );
        TryLoadInt( jsonData, "source", texture.source );
        TryLoadString( jsonData, "name", texture.name, allocator );
    }

    static void LoadTextures( json& jsonData, glTF::glTF& gltfData, Allocator* allocator ) {
        json array = jsonData[ "textures" ];

        sizet array_count = array.size();
        gltfData.textures = ( glTF::Texture* )AllocateAndZero( allocator, sizeof( glTF::Texture ) * array_count );
        gltfData.textures_count = array_count;

        for ( sizet i = 0; i < array_count; ++i ) {
            LoadTexture( array[ i ], gltfData.textures[ i ], allocator );
        }
    }

    static void LoadImage( json& jsonData, glTF::Image& image, Allocator* allocator ) {
        TryLoadInt( jsonData, "bufferView", image.buffer_view );
        TryLoadString( jsonData, "mimeType", image.mime_type, allocator );
        TryLoadString( jsonData, "uri", image.uri, allocator );
    }

    static void LoadImages( json& jsonData, glTF::glTF& gltfData, Allocator* allocator ) {
        json array = jsonData[ "images" ];

        sizet array_count = array.size();
        gltfData.images = ( glTF::Image* )AllocateAndZero( allocator, sizeof( glTF::Image ) * array_count );
        gltfData.images_count = array_count;

        for ( sizet i = 0; i < array_count; ++i ) {
            LoadImage( array[ i ], gltfData.images[ i ], allocator );
        }
    }

    static void LoadSampler( json& jsonData, glTF::Sampler& sampler, Allocator* allocator ) {
        TryLoadInt( jsonData, "magFilter", sampler.mag_filter );
        TryLoadInt( jsonData, "minFilter", sampler.min_filter );
        TryLoadInt( jsonData, "wrapS", sampler.wrap_s );
        TryLoadInt( jsonData, "wrapT", sampler.wrap_t );
    }

    static void LoadSamplers( json& jsonData, glTF::glTF& gltfData, Allocator* allocator ) {
        json array = jsonData[ "samplers" ];

        sizet array_count = array.size();
        gltfData.samplers = ( glTF::Sampler* )AllocateAndZero( allocator, sizeof( glTF::Sampler ) * array_count );
        gltfData.samplers_count = array_count;

        for ( sizet i = 0; i < array_count; ++i ) {
            LoadSampler( array[ i ], gltfData.samplers[ i ], allocator );
        }
    }

    static void LoadSkin( json& jsonData, glTF::Skin& skin, Allocator* allocator ) {
        TryLoadInt( jsonData, "skeleton", skin.skeleton_root_node_index );
        TryLoadInt( jsonData, "inverseBindMatrices", skin.inverse_bind_matrices_buffer_index );
        TryLoadIntArray( jsonData, "joints", skin.joints_count, &skin.joints, allocator );
    }

    static void LoadSkins( json& jsonData, glTF::glTF& gltfData, Allocator* allocator ) {
        json array = jsonData[ "skins" ];

        sizet array_count = array.size();
        gltfData.skins = ( glTF::Skin* )AllocateAndZero( allocator, sizeof( glTF::Skin ) * array_count );
        gltfData.skins_count = array_count;

        for ( sizet i = 0; i < array_count; ++i ) {
            LoadSkin( array[ i ], gltfData.skins[ i ], allocator );
        }
    }

    static void LoadAnimation( json& jsonData, glTF::Animation& animation, Allocator* allocator ) {

        json json_array = jsonData.at( "samplers" );
        if ( json_array.is_array() ) {
            sizet count = json_array.size();

            glTF::AnimationSampler* values = ( glTF::AnimationSampler* )AllocateAndZero( allocator, sizeof( glTF::AnimationSampler ) * count );

            for ( sizet i = 0; i < count; ++i ) {
                json element = json_array.at( i );
                glTF::AnimationSampler& sampler = values[ i ];

                TryLoadInt( element, "input", sampler.input_keyframe_buffer_index );
                TryLoadInt( element, "output", sampler.output_keyframe_buffer_index );

                std::string value = element.value( "interpolation", "");
                if ( value == "LINEAR" ) {
                    sampler.interpolation = glTF::AnimationSampler::Linear;
                }
                else if ( value == "STEP" ) {
                    sampler.interpolation = glTF::AnimationSampler::Step;
                }
                else if ( value == "CUBICSPLINE" ) {
                    sampler.interpolation = glTF::AnimationSampler::CubicSpline;
                }
                else {
                    sampler.interpolation = glTF::AnimationSampler::Linear;
                }
            }

            animation.samplers = values;
            animation.samplers_count = count;
        }

        json_array = jsonData.at( "channels" );
        if ( json_array.is_array() ) {
            sizet count = json_array.size();

            glTF::AnimationChannel* values = ( glTF::AnimationChannel* )AllocateAndZero( allocator, sizeof( glTF::AnimationChannel ) * count );

            for ( sizet i = 0; i < count; ++i ) {
                json element = json_array.at( i );
                glTF::AnimationChannel& channel = values[ i ];

                TryLoadInt( element, "sampler", channel.sampler );
                json target = element.at( "target" );
                TryLoadInt( target, "node", channel.target_node );

                std::string target_path = target.value( "path", "");
                if ( target_path == "scale" ) {
                    channel.target_type = glTF::AnimationChannel::Scale;
                }
                else if ( target_path == "rotation" ) {
                    channel.target_type = glTF::AnimationChannel::Rotation;
                }
                else if ( target_path == "translation" ) {
                    channel.target_type = glTF::AnimationChannel::Translation;
                }
                else if ( target_path == "weights" ) {
                    channel.target_type = glTF::AnimationChannel::Weights;
                }
                else {
                    error( "Error parsing target path %s\n", target_path.c_str() );
                    CASSERT( false );
                    channel.target_type = glTF::AnimationChannel::Count;
                }
            }

            animation.channels = values;
            animation.channels_count = count;
        }
    }

    static void LoadAnimations( json& jsonData, glTF::glTF& gltfData, Allocator* allocator ) {
        json array = jsonData[ "animations" ];

        sizet array_count = array.size();
        gltfData.animations = ( glTF::Animation* )AllocateAndZero( allocator, sizeof( glTF::Animation ) * array_count );
        gltfData.animations_count = array_count;

        for ( sizet i = 0; i < array_count; ++i ) {
            LoadAnimation( array[ i ], gltfData.animations[ i ], allocator );
        }
    }

    static void LoadAsset( json& jsonData, glTF::glTF& gltfData, Allocator* allocator) {
        json jsonAsset = jsonData[ "asset" ];

        gltfData.asset = ( glTF::Asset* )AllocateAndZero( allocator, sizeof( glTF::Asset ) );

        TryLoadString(jsonAsset, "copyright", gltfData.asset->copyright, allocator);
        TryLoadString(jsonAsset, "generator", gltfData.asset->generator, allocator);
        TryLoadString(jsonAsset, "minVersion", gltfData.asset->minVersion, allocator);
        TryLoadString(jsonAsset, "version", gltfData.asset->version, allocator);
    }

    glTF::glTF gltfLoadFile(cstring filePath, Allocator* allocator_) {
        glTF::glTF result(allocator_);

//        if (std::filesystem::exists(filePath) || true) {
//            info("Error: file {} does not exists.\n", filePath);
//            return result;
//        }

        Allocator* heapAllocator = &ServiceManager::GetInstance()->Get<MemoryService>()->m_systemAllocator;

        FileReadResult readResult = FileReadText(filePath, heapAllocator);

        json gltfData = json::parse(readResult.data);

        Allocator* allocator = result.allocator;

        for (auto properties : gltfData.items()) {
            if (properties.key() == "asset") {
                LoadAsset(gltfData, result, allocator);
            } else if (properties.key() == "scene") {
                TryLoadInt(gltfData, "scene", result.scene);
            } else if ( properties.key() == "scenes" ) {
                LoadScenes( gltfData, result, allocator );
            } else if ( properties.key() == "buffers" ) {
                LoadBuffers( gltfData, result, allocator );
            } else if ( properties.key() == "bufferViews" ) {
                LoadBufferViews( gltfData, result, allocator );
            } else if ( properties.key() == "nodes" ) {
                LoadNodes( gltfData, result, allocator );
            } else if ( properties.key() == "meshes" ) {
                LoadMeshes( gltfData, result, allocator );
            } else if ( properties.key() == "accessors" ) {
                LoadAccessors( gltfData, result, allocator );
            } else if ( properties.key() == "materials" ) {
                LoadMaterials( gltfData, result, allocator );
            } else if ( properties.key() == "textures" ) {
                LoadTextures( gltfData, result, allocator );
            } else if ( properties.key() == "images" ) {
                LoadImages( gltfData, result, allocator );
            } else if ( properties.key() == "samplers" ) {
                LoadSamplers( gltfData, result, allocator );
            } else if ( properties.key() == "skins" ) {
                LoadSkins( gltfData, result, allocator );
            } else if ( properties.key() == "animations" ) {
                LoadAnimations( gltfData, result, allocator );
            }
        }

        heapAllocator->deallocate(readResult.data);

        return result;
    }

    i32 gltfGetAttributeAccessorIndex( glTF::MeshPrimitive::Attribute* attributes, u32 attributeCount, cstring attributeName ) {
        for ( u32 index = 0; index < attributeCount; ++index) {
            glTF::MeshPrimitive::Attribute& attribute = attributes[ index ];
            if ( strcmp( attribute.key.c_str(), attributeName ) == 0 ) {
                return attribute.accessor_index;
            }
        }

        return -1;
    }

    i32 glTF::GetDataOffset(i32 accessorOffset, i32 bufferViewOffset) {
        i32 byte_offset = bufferViewOffset == INVALID_INT_VALUE ? 0 : bufferViewOffset;
        byte_offset += accessorOffset == INVALID_INT_VALUE ? 0 : accessorOffset;
        return byte_offset;
    }
}