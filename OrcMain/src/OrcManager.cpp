#include "OrcEntity.h"
#include "OrcException.h"
#include "OrcManager.h"
#include "OrcTypes.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include <memory>
#include <vector>
#include <string>

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
            for (const auto& mesh : model.meshes)
            {
                for (const auto& primitive : mesh.primitives)
                {

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