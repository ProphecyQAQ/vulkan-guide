#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

#include <Scene/Object.h>

Object Object::loadGLTF(std::filesystem::path path)
{
    Object obj;

    fastgltf::Parser parser {};
    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(path);

    fastgltf::Asset gltfAsset;
    fastgltf::GltfType type = fastgltf::determineGltfFileType(&data);
    switch (type) {
        case fastgltf::GltfType::glTF: 
        {
            auto [error, asset] = parser.loadGLTF(&data, path.parent_path(), gltfOptions);
            if (error != fastgltf::Error::None) {
                LOG_ERROR("Failed to load glTF: {}", fastgltf::to_underlying(error));
                return obj;
            }
            gltfAsset = std::move(asset);
            break;
        }
        case fastgltf::GltfType::GLB: 
        {
            auto [error, asset] = parser.loadBinaryGLTF(&data, path.parent_path(), gltfOptions);
            if (error != fastgltf::Error::None) {
                LOG_ERROR("Failed to load GLB: {}", fastgltf::to_underlying(error));
                return obj;
            }
            gltfAsset = std::move(asset);
            break;
        }
        case fastgltf::GltfType::Invalid:
        default:
        {
            LOG_ERROR("Invalid glTF/GLB file");
            return obj;
        }
    }

    // load mesh
    std::vector<uint32_t>& indices = obj.getIndices();
    std::vector<Vertex>& vertices = obj.getVertices();
    for (fastgltf::Mesh& mesh : gltfAsset.meshes) 
    {
        // primitive
        for (fastgltf::Primitive& primitive : mesh.primitives)
        {
            if (primitive.type != fastgltf::PrimitiveType::Triangles)
            {
                LOG_ERROR("Only triangle primitives are supported");
                continue;
            }
            
            // vertex count
            uint32_t initialIndex = static_cast<uint32_t>(vertices.size());

            // indices accessor
            fastgltf::Accessor& indexAccessor = gltfAsset.accessors[primitive.indicesAccessor.value()];
            uint32_t indicesCount = static_cast<uint32_t>(indexAccessor.count);

            indices.reserve(indices.size() + indicesCount);
            fastgltf::iterateAccessor<uint32_t>(gltfAsset, indexAccessor, [&](uint32_t index)
            {
                // remap primitive local index to global index
                indices.push_back(initialIndex + index);
            });

            // load vertex
            fastgltf::Accessor& vertexAccessor = gltfAsset.accessors[primitive.findAttribute("POSITION")->second];
            fastgltf::iterateAccessorWithIndex<glm::vec3>(gltfAsset, vertexAccessor, [&](glm::vec3 position, uint32_t index){
                Vertex& vertex = vertices.emplace_back();
                vertex.position = position;
            });

            auto normal = primitive.findAttribute("NORMAL");
            if (normal != primitive.attributes.end())
            {
                fastgltf::Accessor& normalAccessor = gltfAsset.accessors[normal->second];
                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltfAsset, normalAccessor, [&](glm::vec3 normal, uint32_t index){
                    vertices.back().normal = normal;
                });
            }

            auto uv = primitive.findAttribute("TEXCOORD_0");
            if (uv != primitive.attributes.end())
            {
                fastgltf::Accessor& uvAccessor = gltfAsset.accessors[uv->second];
                fastgltf::iterateAccessorWithIndex<glm::vec2>(gltfAsset, uvAccessor, [&](glm::vec2 uv, uint32_t index){
                    vertices.back().uv_u = uv.x;
                    vertices.back().uv_v = uv.y;
                });
            }

            auto color = primitive.findAttribute("COLOR_0");
            if (color != primitive.attributes.end())
            {
                fastgltf::Accessor& colorAccessor = gltfAsset.accessors[color->second];
                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltfAsset, colorAccessor, [&](glm::vec4 color, uint32_t index){
                    vertices.back().color = color;
                });
            }

            // bounding box
            BoundingBox& boundingBox = obj.getBoundingBox();
            boundingBox.minPos = vertices[0].position;
            boundingBox.maxPos = vertices[0].position;
            for (const Vertex& vertex : vertices)
            {
                boundingBox.minPos = glm::min(boundingBox.minPos, vertex.position);
                boundingBox.maxPos = glm::max(boundingBox.maxPos, vertex.position);
            }
        }
    }
    return obj; 
}