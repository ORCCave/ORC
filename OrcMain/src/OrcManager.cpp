#include "OrcEntity.h"
#include "OrcException.h"
#include "OrcManager.h"
#include "OrcTypes.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include <limits>
#include <memory>
#include <vector>
#include <string>
#include <tuple>

namespace Orc
{
    namespace detail
    {
        class _Entity : public Entity
        {
        public:
            _Entity(const String& name) : Entity(name) {}
        };

        struct Vertex
        {
            DirectX::XMFLOAT3 position;
            DirectX::XMFLOAT3 normal;
            DirectX::XMFLOAT2 texCoord;
        };

        void _appendPosition(std::vector<Vertex>& vertices, const tinygltf::Primitive& primitive, const tinygltf::Model model)
        {
            int accessor_index = primitive.attributes.at("POSITION");
            const tinygltf::Accessor& accessor = model.accessors[accessor_index];
            const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];

            if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
            {
                throw OrcException("Component type is not float");
            }
            if (accessor.type != TINYGLTF_TYPE_VEC3)
            {
                throw OrcException("Accessor type is not vec3");
            }

            size_t stride = buffer_view.byteStride == 0 ? 12 : buffer_view.byteStride;
            size_t strideFloats = stride / sizeof(float);
            const float* positions = reinterpret_cast<const float*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
            for (size_t i = 0; i < accessor.count; ++i)
            {
                Vertex vertex;
                vertex.position = { positions[0], positions[1], positions[2] };
                vertices.push_back(vertex);

                positions += strideFloats;
            }
        }

        void _appendNormal(std::vector<Vertex>& vertices, const tinygltf::Primitive& primitive, const tinygltf::Model model)
        {
            if (primitive.attributes.contains("NORMAL"))
            {
                int accessor_index = primitive.attributes.at("NORMAL");
                const tinygltf::Accessor& accessor = model.accessors[accessor_index];
                const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];

                if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
                {
                    throw OrcException("Component type is not float");
                }
                if (accessor.type != TINYGLTF_TYPE_VEC3)
                {
                    throw OrcException("Accessor type is not vec3");
                }
                if (accessor.count != vertices.size())
                {
                    throw OrcException("Accessor count is wrong");
                }

                size_t stride = buffer_view.byteStride == 0 ? 12 : buffer_view.byteStride;
                size_t strideFloats = stride / sizeof(float);
                const float* normals = reinterpret_cast<const float*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
                for (size_t i = 0; i < accessor.count; ++i)
                {
                    vertices[i].normal = { normals[0], normals[1], normals[2] };
                    normals += strideFloats;
                }
            }
        }

        void _appendTexCoord0(std::vector<Vertex>& vertices, const tinygltf::Primitive& primitive, const tinygltf::Model model)
        {
            if (primitive.attributes.contains("TEXCOORD_0"))
            {
                int accessor_index = primitive.attributes.at("TEXCOORD_0");
                const tinygltf::Accessor& accessor = model.accessors[accessor_index];
                const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];

                if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
                {
                    throw OrcException("Component type is not float");
                }
                if (accessor.type != TINYGLTF_TYPE_VEC2)
                {
                    throw OrcException("Accessor type is not vec2");
                }
                if (accessor.count != vertices.size())
                {
                    throw OrcException("Accessor count is wrong");
                }

                size_t stride = buffer_view.byteStride == 0 ? 8 : buffer_view.byteStride;
                size_t strideFloats = stride / sizeof(float);
                const float* texcoord0 = reinterpret_cast<const float*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
                for (size_t i = 0; i < accessor.count; ++i)
                {
                    vertices[i].texCoord = { texcoord0[0], texcoord0[1] };
                    texcoord0 += strideFloats;
                }
            }
        }

        void _appendIndices(std::vector<uint16>& indices, const tinygltf::Primitive& primitive, const tinygltf::Model model, size_t vertexCount)
        {
            const auto& indexAccessor = primitive.indices;

            if (indexAccessor == -1)
            {
                for (size_t i = 0; i < vertexCount; ++i)
                {
                    indices.push_back(i);
                }
                return;
            }

            const tinygltf::Accessor& indexAccessorData = model.accessors[indexAccessor];
            const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessorData.bufferView];
            const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];

            if (indexAccessorData.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE && 
                indexAccessorData.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT &&
                indexAccessorData.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
            {
                throw OrcException("Component type is not unsigned");
            }
            if (indexAccessorData.type != TINYGLTF_TYPE_SCALAR)
            {
                throw OrcException("Accessor type is not scalar");
            }
            if (indexBufferView.byteStride != 0)
            {
                throw OrcException("Stride is not 0");
            }

            const size_t count = indexAccessorData.count;
            const uint8* dataPtr = reinterpret_cast<const uint8*>(&indexBuffer.data[indexBufferView.byteOffset + indexAccessorData.byteOffset]);
            switch (indexAccessorData.componentType)
            {
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                {
                    auto src = reinterpret_cast<const uint8*>(dataPtr);
                    for (size_t i = 0; i < count; ++i)
                    {
                        indices.push_back(static_cast<uint16>(src[i]));
                    }
                    break;
                }
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                {
                    auto src = reinterpret_cast<const uint16*>(dataPtr);
                    for (size_t i = 0; i < count; ++i)
                    {
                        indices.push_back(src[i]);
                    }
                    break;
                }
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                {
                    auto src = reinterpret_cast<const uint32*>(dataPtr);
                    for (size_t i = 0; i < count; ++i)
                    {
                        uint32 v = src[i];
                        if (v > std::numeric_limits<uint16>::max())
                        {
                            throw OrcException("Index value exceeds uint16 range");
                        }
                        indices.push_back(static_cast<uint16>(v));
                    }
                    break;
                }
            }
        }

        std::shared_ptr<Entity> createGLTFEntity(const String& entityName, const String& filePath)
        {
            tinygltf::TinyGLTF loader;
            tinygltf::Model model;
            std::string err;
            std::string warn;
            bool ret = false;
            if (filePath.ends_with(".glb"))
            {
                ret = loader.LoadBinaryFromFile(&model, &err, &warn, filePath);
            }
            else
            {
                ret = loader.LoadASCIIFromFile(&model, &err, &warn, filePath);
            }
            if (!ret)
            {
                throw OrcException(err);
            }

            if (model.meshes.empty())
            {
                throw OrcException("Mesh is empty");
            }
            
            std::vector<Vertex> vertices;
            std::vector<uint16> indices;

            for (const auto& mesh : model.meshes)
            {
                for (const auto& primitive : mesh.primitives)
                {
                    _appendPosition(vertices, primitive, model);
                    _appendNormal(vertices, primitive, model);
                    _appendTexCoord0(vertices, primitive, model);
                    _appendIndices(indices, primitive, model, vertices.size());
                }
            }

            return std::make_shared<_Entity>(entityName);
        }
    }

    Entity* SceneManager::createEntity(const String& entityName, const String& filePath)
    {
        if (filePath.ends_with(".glb") || filePath.ends_with(".gltf"))
        {
            auto entity = mEntities.emplace_back(detail::createGLTFEntity(entityName, filePath));
            return entity.get();
        }

        return nullptr;
    }

    void SceneManager::destroyEntity(Entity* ent)
    {
        for (auto it = mEntities.begin(); it != mEntities.end(); ++it)
        {
            if (ent == it->get())
            {
                mEntities.erase(it);
                break;
            }
        }
    }
}