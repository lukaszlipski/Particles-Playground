#include "Graphics/RenderGraph/miscnodes.h"

void PrepareSceneBufferNode::Execute(const RGExecuteContext& context)
{
    Camera* camera = context.GetSceneData().mCamera;
    CommandList& commandList = context.GetCommandList();

    SceneContants data;
    data.Projection = camera->GetProjection();
    data.View = camera->GetView();

    GPUBuffer* sceneBuffer = context.GetGPUBuffer(RESOURCEID("SceneBuffer"));

    uint8_t* dstData = sceneBuffer->Map();

    memcpy(dstData, &data, sizeof(SceneContants));

    sceneBuffer->Unmap(commandList);
}
