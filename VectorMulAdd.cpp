#include <d3dx12.h>
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <vector>
#include <iostream>
#include <d3dcompiler.h>
#include <dxcore.h> // Include for experimental features

#include "include/util.h"

using namespace Microsoft::WRL;

// Helper function to check HRESULT
void CheckHRFunc(HRESULT hr, const char* file, int line) {
    if (FAILED(hr)) {
        std::cerr << "HRESULT failed with code: " << std::hex << hr 
                  << " at " << file << ":" << std::dec << line << std::endl;
        exit(EXIT_FAILURE);
    }
}
#define CheckHR(hr) CheckHRFunc(hr, __FILE__, __LINE__)

// Constants
const UINT THREAD_GROUP_SIZE = 4; // Number of threads per group

int main() {
    // Enable the debug layer (optional, for debugging)
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
    }
#endif


    // Create DXGI Factory
    ComPtr<IDXGIFactory6> factory;
    CheckHR(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

    // Enumerate adapters and check for DirectX 12 support
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr))) {
            break;
        }
    }
    if (!adapter) {
        std::cerr << "No DirectX 12 compatible GPU found." << std::endl;
        return EXIT_FAILURE;
    }

    // Create the DirectX 12 device
    ComPtr<ID3D12Device> device;
    CheckHR(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));

    // Create a command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ComPtr<ID3D12CommandQueue> commandQueue;
    CheckHR(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

    // Create a command allocator and command list
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    CheckHR(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&commandAllocator)));

    ComPtr<ID3D12GraphicsCommandList> commandList;
    CheckHR(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));


    // Define buffer sizes
    auto AlignTo = [](uint32_t size, uint32_t alignment) {
        return (size + alignment - 1) & ~(alignment - 1);
    };
    auto InitilizeBuffer = [](DataType dt, std::vector<uint8_t>& buffer, uint32_t M, uint32_t K, uint32_t stride_align_bytes, float initValue = 1.0f) {
        for (uint32_t m = 0; m < M; ++m) {
            for (uint32_t k = 0; k < K; ++k) {
                SetDataFloat(buffer.data(), dt, m * stride_align_bytes, k, initValue);
            }
        }
    };

    DataType dt = DATA_TYPE_FLOAT32;
    constexpr uint32_t STRIDE_ALIGH_BYTES = 32;
    uint32_t M = 8;
    uint32_t K = 8;

    uint32_t inputVectorBufferSize = SizeofType(dt) * K;
    uint32_t matrixBufferSize = AlignTo(SizeofType(dt) * K, STRIDE_ALIGH_BYTES) * M;
    uint32_t biasBufferSize = SizeofType(dt) * M;
    uint32_t outputVectorBufferSize = SizeofType(dt) * M;

    std::vector<uint8_t> inputVectorData(inputVectorBufferSize, 0);
    std::vector<uint8_t> matrixData(matrixBufferSize, 0);
    std::vector<uint8_t> biasData(biasBufferSize, 0);
    std::vector<uint8_t> outputData(outputVectorBufferSize, 0);

    InitilizeBuffer(dt, inputVectorData, 1, K, STRIDE_ALIGH_BYTES, 4.0f);
    InitilizeBuffer(dt, matrixData, M, K, STRIDE_ALIGH_BYTES, 2.0f);
    InitilizeBuffer(dt, biasData, 1, M, STRIDE_ALIGH_BYTES, 3.0f);

    auto CreateBuffer = [](ComPtr<ID3D12Device>& device, uint32_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, ComPtr<ID3D12Resource>& buffer) {
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size, flags);
        CheckHR(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, initState, nullptr, IID_PPV_ARGS(&buffer)));
    };

    auto CreateUploadBuffer = [](ComPtr<ID3D12Device>& device, uint32_t size, ComPtr<ID3D12Resource>& buffer) {
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
        CheckHR(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer)));
    };

    auto CreateReadBackBuffer = [](ComPtr<ID3D12Device>& device, uint32_t size, ComPtr<ID3D12Resource>& buffer) {
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_READBACK);
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
        CheckHR(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&buffer)));
    };

    // Create input_vector_buffer
    ComPtr<ID3D12Resource> inputVectorBuffer, matrixBuffer, biasBuffer, outputVectorBuffer;
    ComPtr<ID3D12Resource> inputVectorUploadBuffer, matrixUploadBuffer, biasUploadBuffer, outputReadbackBuffer;

    CreateBuffer(device, inputVectorBufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, inputVectorBuffer);
    CreateBuffer(device, matrixBufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, matrixBuffer);
    CreateBuffer(device, biasBufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, biasBuffer);
    CreateBuffer(device, outputVectorBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, outputVectorBuffer);

    CreateUploadBuffer(device, inputVectorBufferSize, inputVectorUploadBuffer);
    CreateUploadBuffer(device, matrixBufferSize, matrixUploadBuffer);
    CreateUploadBuffer(device, biasBufferSize, biasUploadBuffer);

    CreateReadBackBuffer(device, outputVectorBufferSize, outputReadbackBuffer);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 4; // Number of descriptors
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    CheckHR(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap)));

    // Populate the descriptor heap with SRVs
    D3D12_CPU_DESCRIPTOR_HANDLE handle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

    // SRV for inputVectorBuffer
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.StructureByteStride = 0;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW; // Use raw buffer flag for raw buffers
    srvDesc.Format = DXGI_FORMAT_R32_TYPELESS; // Use a typeless format for raw buffers

    srvDesc.Buffer.NumElements = inputVectorBufferSize / SizeofType(dt);
    device->CreateShaderResourceView(inputVectorBuffer.Get(), &srvDesc, handle);
    handle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    srvDesc.Buffer.NumElements = matrixBufferSize / SizeofType(dt);
    device->CreateShaderResourceView(matrixBuffer.Get(), &srvDesc, handle);
    handle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    srvDesc.Buffer.NumElements = biasBufferSize / SizeofType(dt);
    device->CreateShaderResourceView(biasBuffer.Get(), &srvDesc, handle);
    handle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Add UAV for output_vector_buffer
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.CounterOffsetInBytes = 0;
    uavDesc.Buffer.NumElements = outputVectorBufferSize / SizeofType(dt); // Assuming float elements
    // uavDesc.Buffer.StructureByteStride = SizeofType(dt);
    // uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.Buffer.StructureByteStride = 0;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW; // Use raw buffer flag for raw buffers
    uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    device->CreateUnorderedAccessView(outputVectorBuffer.Get(), nullptr, &uavDesc, handle);

    auto UploadData = [](ComPtr<ID3D12Resource>& uploadBuffer, const void* data, uint32_t size) {
        void* mappedData;
        CheckHR(uploadBuffer->Map(0, nullptr, &mappedData));
        memcpy(mappedData, data, size);
        uploadBuffer->Unmap(0, nullptr);
    };

    UploadData(inputVectorUploadBuffer, inputVectorData.data(), inputVectorBufferSize);
    UploadData(matrixUploadBuffer, matrixData.data(), matrixBufferSize);
    UploadData(biasUploadBuffer, biasData.data(), biasBufferSize);

    auto TransitionResource = [](ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12Resource>& resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState) {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), beforeState, afterState);
        commandList->ResourceBarrier(1, &barrier);
    };

    TransitionResource(commandList, inputVectorBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionResource(commandList, matrixBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionResource(commandList, biasBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionResource(commandList, outputVectorBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    commandList->CopyBufferRegion(inputVectorBuffer.Get(), 0, inputVectorUploadBuffer.Get(), 0, inputVectorBufferSize);
    commandList->CopyBufferRegion(matrixBuffer.Get(), 0, matrixUploadBuffer.Get(), 0, matrixBufferSize);
    commandList->CopyBufferRegion(biasBuffer.Get(), 0, biasUploadBuffer.Get(), 0, biasBufferSize);

    TransitionResource(commandList, inputVectorBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    TransitionResource(commandList, matrixBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    TransitionResource(commandList, biasBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    // Load and create the compute shader
    ComPtr<ID3DBlob> computeShaderBlob;
    ComPtr<ID3D12PipelineState> pipelineState;
    ComPtr<ID3D12RootSignature> rootSignature;

    CheckHR(D3DReadFileToBlob(L"VectorMulAdd.cso", &computeShaderBlob));
    std::cout << "Compute shader loaded successfully!" << std::endl;
    
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 3; // Number of SRVs: input_vector_buffer, matrix_buffer, bias_buffer
    srvRange.BaseShaderRegister = 0;
    srvRange.RegisterSpace = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    
    D3D12_ROOT_PARAMETER rootParameters[2] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].DescriptorTable.pDescriptorRanges = &srvRange;
    
    D3D12_DESCRIPTOR_RANGE uavRange = {};
    uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange.NumDescriptors = 1;
    uavRange.BaseShaderRegister = 0;
    uavRange.RegisterSpace = 0;
    uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[1].DescriptorTable.pDescriptorRanges = &uavRange;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = _countof(rootParameters);
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> serializedRootSignature;
    ComPtr<ID3DBlob> errorBlob;
    CheckHR(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSignature, &errorBlob));
    CheckHR(device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

    // Create pipeline state
    D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineStateDesc = {};
    pipelineStateDesc.pRootSignature = rootSignature.Get();
    pipelineStateDesc.CS = { computeShaderBlob->GetBufferPointer(), computeShaderBlob->GetBufferSize() };
    CheckHR(device->CreateComputePipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pipelineState)));

    // Set up the compute shader
    commandList->SetPipelineState(pipelineState.Get());
    commandList->SetComputeRootSignature(rootSignature.Get());

    // Bind the descriptor heap to the command list
    ID3D12DescriptorHeap* heaps[] = { descriptorHeap.Get() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);
    commandList->SetComputeRootDescriptorTable(0, descriptorHeap->GetGPUDescriptorHandleForHeapStart());
    commandList->SetComputeRootDescriptorTable(1, CD3DX12_GPU_DESCRIPTOR_HANDLE(
        descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 3, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));

    // Dispatch compute shader
    commandList->Dispatch(M / THREAD_GROUP_SIZE, 1, 1);

    // Transition output buffer to copy source
    TransitionResource(commandList, outputVectorBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->CopyBufferRegion(outputReadbackBuffer.Get(), 0, outputVectorBuffer.Get(), 0, outputVectorBufferSize);

    // Close and execute command list
    CheckHR(commandList->Close());
    ID3D12CommandList* commandLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    // Wait for GPU to finish
    ComPtr<ID3D12Fence> fence;
    HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    uint32_t fenceValue = 1;
    CheckHR(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    CheckHR(commandQueue->Signal(fence.Get(), fenceValue));
    if (fence->GetCompletedValue() < fenceValue) {
        CheckHR(fence->SetEventOnCompletion(fenceValue, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);
    }
    CloseHandle(fenceEvent);

    // Read back data
    void* mappedData;
    CheckHR(outputReadbackBuffer->Map(0, nullptr, &mappedData));
    memcpy(outputData.data(), mappedData, outputVectorBufferSize);
    outputReadbackBuffer->Unmap(0, nullptr);

    // Verify results
    {
        std::vector<uint8_t> goldenData(outputVectorBufferSize, 0);
        MatMulAdd(dt, goldenData.data(), matrixData.data(), inputVectorData.data(), biasData.data(), M, K, STRIDE_ALIGH_BYTES);

        int err = 0;
        for (size_t i = 0; i < M; ++i) {
            float v = GetDataFloat(outputData.data(), dt, 0, i);
            float golden = GetDataFloat(goldenData.data(), dt, 0, i);

            if (v != golden) {
                std::cout << "Output[" << i << "] = " << v << "; ";
                std::cout << "Golden[" << i << "] = " << golden << std::endl;
                err ++;
            }
        }
        if (err != 0) return EXIT_FAILURE;
    }

    std::cout << "Compute shader executed successfully and results are correct!" << std::endl;
    return 0;
}