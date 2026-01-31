#include <Scene/Scene.h>

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