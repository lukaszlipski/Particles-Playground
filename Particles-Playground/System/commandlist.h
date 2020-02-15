#pragma once

enum class QueueType;

class CommandList
{
public:
    CommandList(QueueType type);
    ~CommandList();

    CommandList(const CommandList&) = delete;
    CommandList& operator=(const CommandList&) = delete;

    CommandList(CommandList&& rhs);
    CommandList& operator=(CommandList&& rhs);

    ID3D12GraphicsCommandList* operator->();

    void Submit();

    inline ID3D12GraphicsCommandList* Get() { return mCommandList; }

private:
    QueueType mType;
    ID3D12GraphicsCommandList* mCommandList = nullptr;

};
