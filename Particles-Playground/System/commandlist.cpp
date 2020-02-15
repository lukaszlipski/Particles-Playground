#include "commandlist.h"
#include "graphic.h"

CommandList::CommandList(QueueType type)
    : mType(type)
{
    ID3D12CommandAllocator* allocator = Graphic::Get().GetCurrentCommandAllocator(type);
    const D3D12_COMMAND_LIST_TYPE commandListType = Graphic::GetCommandListType(type);
    Graphic::Get().GetDevice()->CreateCommandList(0, commandListType, allocator, nullptr, IID_PPV_ARGS(&mCommandList));
}

CommandList::~CommandList()
{
    if (!mCommandList) { return; }

    mCommandList->Release();
    mCommandList = nullptr;
}

CommandList::CommandList(CommandList&& rhs)
{
    *this = std::move(rhs);
}

ID3D12GraphicsCommandList* CommandList::operator->()
{
    return mCommandList;
}

void CommandList::Submit()
{
    if (!mCommandList) { return; }

    mCommandList->Close();
    Graphic::Get().GetQueue(mType)->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&mCommandList));
}

CommandList& CommandList::operator=(CommandList&& rhs)
{
    mCommandList = rhs.mCommandList;
    rhs.mCommandList = nullptr;

    return *this;
}
