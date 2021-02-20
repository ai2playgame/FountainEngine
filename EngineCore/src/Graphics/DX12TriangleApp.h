//*********************************************************
//
// DirectX12を使って三角形ポリゴンを表示する
// アプリケーションクラス
//
//*********************************************************

#pragma once
#include "DX12AppBase.h"

using Microsoft::WRL::ComPtr;

namespace fne {

using namespace DirectX;

class DX12TriangleApp : public DX12AppBase {
public:
	DX12TriangleApp(UINT width, UINT height, std::wstring name);

	virtual void OnInit() override;
	virtual void OnUpdate() override;
	virtual void OnRender() override;
	virtual void OnDestroy() override;

private:

	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();
	void WaitForPreviousFrame();

	static const UINT FrameCount = 2;
	
	struct Vertex {
		XMFLOAT3 pos;
		XMFLOAT4 color;
	};

	// Rendering Pipeline Objects;
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	UINT m_rtvDescriptorSize;

	// Application resources
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	// Synchronization object
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	UINT m_fenceValue;

};

}
