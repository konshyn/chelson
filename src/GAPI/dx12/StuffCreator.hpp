#include <d3d12.h>
#include <d3dcompiler.h>

namespace dx12
{
    ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);

    ComPtr<ID3D12CommandQueue> CreateCommandQueue();

    ComPtr<ID3D12RootSignature> CreateRootSignature();
    ComPtr<ID3D12Resource> CreateBuffer(const uint8_t* data, size_t sizeInBytes);

    ComPtr<ID3D12PipelineState> CreatePipelineStateObject(ComPtr<ID3D12RootSignature>& rootSignature, ComPtr<ID3DBlob> vertexShader,ComPtr<ID3DBlob> pixelShader);

    ComPtr<ID3DBlob> LoadShaderFromFile(LPCWSTR fileName);

    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type);

    ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type);
}