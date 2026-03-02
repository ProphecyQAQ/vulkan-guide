#include <Scene/Scene.h>
#include <RHI/GraphicsContext.h>

Scene::Scene()
{
    // Create a simple triangle
    triangleObj = std::make_unique<Object>();
    auto& vertices = triangleObj->getVertices();
    auto& indices = triangleObj->getIndices();

    // Define triangle vertices
    vertices.emplace_back();
    vertices.back().position = glm::vec3(-0.5f, -0.5f, 0.0f);
    vertices.back().color = glm::vec4(1.0f, 0.0f, 0.0f, 1.f); // Red

    vertices.emplace_back();
    vertices.back().position = glm::vec3(0.5f, -0.5f, 0.0f);
    vertices.back().color = glm::vec4(0.0f, 1.0f, 0.0f, 1.f); // Green

    vertices.emplace_back();
    vertices.back().position = glm::vec3(0.0f, 0.5f, 0.0f);
    vertices.back().color = glm::vec4(0.0f, 0.0f, 1.0f, 1.f); // Blue

    // Indices
    indices = {0, 1, 2};

    // Add to scene
    //addObject(triangleObj.get(), glm::mat4(1.0f));
}

Scene::~Scene()
{
}

void Scene::addObject(Object* object, glm::mat4 transform)
{
    objects[object] = transform;
}

void Scene::OnUpdate(float deltaTime)
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

    // Update camera
    camera.update(deltaTime);
}