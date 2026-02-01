#include <Scene/Scene.h>
#include <RHI/GraphicsContext.h>

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::addObject(Object* object, glm::mat4 transform)
{
    objects[object] = transform;
}

void Scene::OnUpdate()
{
    RenderSystem* renderSystem = RenderSystem::get();
    for (auto& [obj, transform] : objects)
    {
        // update object if needed
        RenderData renderData;
        renderData.transform = transform;
        renderData.vertices = &obj->getVertices();
        renderData.indices = &obj->getIndices();
        renderSystem->submit(renderData);
    }
}