#include <RDG/RDGGraph.h>

RDGPass& RDGGraph::addPass(std::string passName)
{
    passes.emplace_back(passName);
    return passes.back();
}