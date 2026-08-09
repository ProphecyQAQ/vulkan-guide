#pragma once
#include <string>
#include <vector>

#include <RDG/RDGResource.h>
#include <RDG/RDGPass.h>

class RDGGraph
{
public:
    RDGGraph(std::string name) : name(name) {};
    ~RDGGraph() {};

    std::string getName() const { return name; }

    RDGPass& addPass(std::string passName);
private:
    std::string name;

    std::vector<RDGResource> resources;
    std::vector<RDGPass> passes;
};