#include <queue>

#include <RDG/RDGGraph.h>
#include <RDG/RDGResource.h>
#include <RDG/RDGTransientResourcePool.h>

#include <Vulkan/VulkanContext.h>
#include <Vulkan/VulkanHelper.h>

RDGPass& RDGGraph::addPass(std::string passName)
{
    passes.emplace_back(passName);
    return passes.back();
}

uint32_t RDGGraph::createTexture(const std::string& name, const RDGTextureDesc& desc)
{
    RDGResource resource;
    resource.handle = static_cast<uint32_t>(resources.size());
    resource.name = name;
    resource.type = RDG_RESOURCE_TYPE_TEXTURE;
    resource.desc = desc;

    resources.push_back(resource);
    return resource.handle;
}

uint32_t RDGGraph::createBuffer(const std::string& name, const RDGBufferDesc& desc)
{
    RDGResource resource;
    resource.handle = static_cast<uint32_t>(resources.size());
    resource.name = name;
    resource.type = RDG_RESOURCE_TYPE_BUFFER;
    resource.desc = desc;

    resources.push_back(resource);
    return resource.handle;
}

uint32_t RDGGraph::importTexture(const std::string& name, VulkanImage* image)
{
    RDGResource resource;
    resource.handle = static_cast<uint32_t>(resources.size());
    resource.name = name;
    resource.type = RDG_RESOURCE_TYPE_TEXTURE;
    resource.isExternal = true;
    resource.image = image;

    resources.push_back(resource);
    return resource.handle;
}

const std::string& RDGGraph::getPassNameById(uint32_t idx) const
{
    const static std::string empty;

    for (const RDGPass& pass : passes)
    {
        if (pass.getIndex() == idx)
        {
            return pass.GetName();
        }
    }
    assert(0);

    return empty;
}

void RDGGraph::orderBefore(std::string& before, std::string& after)
{
    explicitOrder[before].insert(after);
}

void RDGGraph::compile()
{
    if (isCompiled)
    {
        LOG_WARN("RDG {0} already compiled.", name);
        return;
    }
    isCompiled = true;

    // pass dependency
    std::vector<std::pair<uint32_t, uint32_t>> edges;
    // resource access (key: resource handle, value: pass id + wirte)
    std::map<uint32_t, std::vector<std::pair<uint32_t, bool>>> resourceAccesses;
    
    // collect resource access info from passes
    for (uint32_t passIdx = 0; passIdx < passes.size(); passIdx ++)
    {   
        RDGPass& pass = passes[passIdx];
        pass.setIndex(passIdx);
        for (const RDGPassAccess& access : pass.GetResourceAccesses())
        {
            RDGResource& resource = resources[access.resourceHandle];

            resourceAccesses[access.resourceHandle].push_back({passIdx, access.isWrite});

            resource.firstUsePass = std::min(resource.firstUsePass, passIdx);
            resource.lastUsePass  = std::max(resource.lastUsePass, passIdx); 
        }
    }

    // Construct dependency edge
    for (auto [resourceHandle, accessPasses] : resourceAccesses)
    {
        for (uint32_t ii = 0; ii < accessPasses.size(); ii ++)
        {
            for (uint32_t jj = ii + 1; jj < accessPasses.size(); jj ++)
            {
                std::pair<uint32_t, bool> prev = accessPasses[ii];
                std::pair<uint32_t, bool> curr = accessPasses[jj];

                bool hasDenpendency = false;

                // check expilcit order
                const std::string& prevPassName = getPassNameById(prev.first);
                const std::string& currPassName = getPassNameById(curr.first);
                if (explicitOrder[prevPassName].contains(currPassName))
                {
                    hasDenpendency = true;
                }
          
                // No denpendency only both pass read resource
                if (prev.second || curr.second)
                {
                    hasDenpendency = true;
                }

                if (hasDenpendency)
                {
                    edges.push_back({prev.first, curr.first});
                }
            }
        }
    }

    // topoliocal sort
    std::vector<int> inDegree(passes.size(), 0);
    for (auto& [from, to]: edges)
    {
        passes[from].addSuccessor(to);
        inDegree[to] ++;
    }
    std::queue<uint32_t> q;
    for (uint32_t idx = 0; idx < inDegree.size(); idx ++)
    {
        if (inDegree[idx] == 0)
        {
            q.push(idx);
        }
    }
    
    while (!q.empty())
    {
        uint32_t passId = q.front();
        q.pop();
        executeOrder.push_back(passId);

        for (uint32_t successorId : passes[passId].GetSuccessors())
        {
            inDegree[successorId] --;
            if (inDegree[successorId] == 0)
            {
                q.push(successorId);
            }
        }
    }

    assert(executeOrder.size() == passes.size());

    // update resource use time
    for (uint32_t passId : executeOrder)
    {
        for (const RDGPassAccess& passAccess : passes[passId].GetResourceAccesses())
        {
            resources[passAccess.resourceHandle].firstUsePass = std::min(resources[passAccess.resourceHandle].firstUsePass, passId);
            resources[passAccess.resourceHandle].lastUsePass = std::max(resources[passAccess.resourceHandle].lastUsePass, passId);
        }
    }

    // allocate resource
    for (RDGResource& res : resources)
    {
        if (res.isExternal) 
        {
            continue;
        }

        if (res.type == RDG_RESOURCE_TYPE_TEXTURE)
        {
            res.image = RDGTransientResourcePool::get().acquireImage(std::get<RDGTextureDesc>(res.desc));
        }
        else if (res.type == RDG_RESOURCE_TYPE_BUFFER)
        {
            res.buffer = RDGTransientResourcePool::get().acquireBuffer(std::get<RDGBufferDesc>(res.desc));
        }
        else 
        {
            LOG_ERROR("Error resouce type {} in RDG {}", res.name, name);
        }
    }
}

void RDGGraph::execute(VkCommandBuffer cmd)
{
    if (!isCompiled)
    {
        LOG_ERROR("Running uncompiled RDG {}", name);
        return;
    }

    for (uint32_t passIdx : executeOrder)
    {
        RDGPass& pass = passes[passIdx];

        for (const RDGPassAccess& resAccess : pass.GetResourceAccesses())
        {
            RDGResource& res = resources[resAccess.resourceHandle];
            if (res.type != RDG_RESOURCE_TYPE_TEXTURE)
            {
                continue;
            }
            if (res.image == nullptr)
            {
                LOG_ERROR("RDG {0} resource {1} has not initialized", name, res.name);
            }

            if (res.currentState.layout != resAccess.layout)
            {
                VulkanHeaplerLibrary::transitionImage(cmd, 
                    res.image->getImage(), 
                    res.currentState.layout,
                    resAccess.layout);
                res.currentState.layout = resAccess.layout;
            }
        }

        RDGPassContext ctx(VulkanContext::get()->getDevice(), resources);        
        pass.execute(cmd, ctx);
    }
}

void RDGGraph::clear()
{
    RDGTransientResourcePool::get().releaseAll();
}