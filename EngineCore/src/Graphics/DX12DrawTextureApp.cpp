#include "pch.h"
#include "DX12DrawTextureApp.h"
#include "DX12CoreHelper.h"
#include "Win32Application.h"
#include "Core/Utility/Logger/Logger.h"

namespace {
LPCWSTR shaderFileName = L"shaders.hlsl";
}

fne::DX12DrawTextureApp::DX12DrawTextureApp(uint32_t width, uint32_t height, std::wstring appName)
	: DX12AppBase(width, height, appName)
	, m_frameIndex(0)
	, m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height))
	, m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height))
	, m_rtvDescriptorSize(0)
{
}

fne::DX12DrawTextureApp::~DX12DrawTextureApp()
{
}

void fne::DX12DrawTextureApp::OnInit()
{
	LoadPipeline();
	LoadAssets();
}

void fne::DX12DrawTextureApp::OnUpdate()
{
}

void fne::DX12DrawTextureApp::OnRender()
{
}

void fne::DX12DrawTextureApp::OnDestroy()
{
}

void fne::DX12DrawTextureApp::LoadPipeline()
{
	HRESULT hr;
	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	{
		// デバッグレイヤー作成
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
			LOG_DEBUG("graphics/init", "D3D12GetDebugInterface is SUCCEEDED");
		}
		else {
			LOG_WARN("graphics/init", "DebugLayerの作成に失敗しました");
		}
	}
#endif
	// デバイス作成
	ComPtr<IDXGIFactory4> factory;
	if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)))) {
		LOG_ERROR("graphics/init", "Failed CreateDXGIFactory()");
	}
	ComPtr<IDXGIAdapter1> hardwareAdapter;
	GetHardwareAdapter(factory.Get(), &hardwareAdapter);
	if (FAILED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)))) {
		LOG_ERROR("graphics/init", "Failed D3D12CreateDevice()");
	}

	LOG_DEBUG("graphics/init", "Succeed in creating the Dx12Device");
	
	// デバイスからコマンドキュー作成
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	if (FAILED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)))) {
		LOG_ERROR("graphics/init", "Failed CreateCommandQueue()");
	}
	
	// デバイスからスワップチェーンを作成
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	ComPtr<IDXGISwapChain1> swapChain;
	if (FAILED(factory->CreateSwapChainForHwnd(m_commandQueue.Get(),
		Win32Application::GetHwnd(),
		&swapChainDesc, nullptr, nullptr, &swapChain))) {
		LOG_ERROR("graphics/init", "Failed CerateSwapChainForHwnd()");
	}
	if (FAILED(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER))) {
		LOG_ERROR("graphics/init", "Failed to call MakeWindowAssociation()");
	}
	if (FAILED(swapChain.As(&m_swapChain))) {
		LOG_ERROR("graphics/init", "Failed to set swapChain");
	}
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	
	// レンダーターゲット用のディスクリプタヒープを作成
	{
		// RTV用ディスクリプタヒープ作成
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		if (FAILED(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)))) {
			LOG_ERROR("graphics/init", "Failed to create a descriptor heap for RTV");
		}
		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// フレーム描画のためのGPUリソースを確保
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		
		// RTVをフレームバッファ数分作成
		for (UINT n = 0; n < FrameCount; n++) {
			HRESULT hr;
			hr = m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
			if (FAILED(hr)) {
				LOG_ERROR("graphics/init", "Failed to get back buffer's RTV");
			}
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

	// コマンドアロケータ作成
	if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)))) {
		LOG_ERROR("graphics/init", "Failed CreateCommandAllocator()");
	}
}

void fne::DX12DrawTextureApp::LoadAssets()
{
	// ルートシグネチャを作成
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
		rootSigDesc.Init(0, nullptr, 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> errorBlob;
		if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			&signature, &errorBlob))) {
			LOG_ERROR("graphics/init", "Failed D3D12SerializeRootSignature()");
		}
		if (FAILED(m_device->CreateRootSignature(0, signature->GetBufferPointer(),
			signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)))) {
			LOG_ERROR("graphics/init", "Failed CreateRootSignature()");
		}
	}
	
	// パイプラインステートを作成して、シェーダをコンパイル及びロードする
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		LOG_DEBUG_W(L"Graphics/shader", fmt::format(L"Compile ShaderFile({})", GetAssetFullPath(shaderFileName)));
		
		if (FAILED(D3DCompileFromFile(GetAssetFullPath(shaderFileName).c_str(),
			nullptr, nullptr, "VSMain", "vs_5_0",
			compileFlags, 0, &vertexShader, nullptr))) {
			LOG_ERROR("Graphics/shader", "Failed to compile the vertex shader file");
		}
		if (FAILED(D3DCompileFromFile(GetAssetFullPath(shaderFileName).c_str(),
			nullptr, nullptr, "PSMain", "ps_5_0",
			compileFlags, 0, &pixelShader, nullptr))) {
			LOG_ERROR("Graphics/shader", "Failed to compole the pixel shader file");
		}

		// TODO: 02/21未明ここまで
	}
}

void fne::DX12DrawTextureApp::PopulateCommandList()
{
}

void fne::DX12DrawTextureApp::WaitForPreviousFrame()
{
}
