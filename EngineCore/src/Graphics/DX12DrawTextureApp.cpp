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
	PopulateCommandList();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	if (FAILED(m_swapChain->Present(1, 0))) {
		LOG_ERROR("Graphics/render", "Failed to present swapchain");
	}
	WaitForPreviousFrame();
}

void fne::DX12DrawTextureApp::OnDestroy()
{
	WaitForPreviousFrame();
    CloseHandle(m_fenceEvent);
}

void fne::DX12DrawTextureApp::LoadPipeline()
{
	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	{
		// デバッグレイヤー作成
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
			LOG_DEBUG("graphics/debug", "DebugLayer is enabled");
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

		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (FAILED(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)))) {
			LOG_ERROR("graphics/init", "Failed to create a descriptor heap for SRV");
		}
	}

	// フレーム描画のためのGPUリソースを確保
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		
		// RTVをフレームバッファ数分作成
		for (UINT n = 0; n < FrameCount; n++) {
			if (FAILED(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])))) {
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
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData{};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
			LOG_WARN("graphics/init", "this device does not support feature level 1.1.");
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		CD3DX12_DESCRIPTOR_RANGE1 ranges[1] = {};
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		CD3DX12_ROOT_PARAMETER1 rootParameters[1] = {};
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

		D3D12_STATIC_SAMPLER_DESC sampler{};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> errorBlob;
		if (FAILED(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBlob))) {
			LOG_ERROR("graphics/init", "Failed D3D12SerializeRootSignature()");
		}
		if (FAILED(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)))) {
			LOG_ERROR("graphics/init", "Failed CreateRootSignature()");
		}
	}
	
	// パイプラインステートを作成して、シェーダをコンパイル及びロードする
	{
		ComPtr<ID3DBlob> vertexShaderBlob;
		ComPtr<ID3DBlob> pixelShaderBlob;
		ComPtr<ID3DBlob> errorBlob;

#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		LOG_DEBUG_W(L"Graphics/shader", fmt::format(L"Compile ShaderFile({})", GetAssetFullPath(shaderFileName)));
		
		HRESULT hr = D3DCompileFromFile(GetAssetFullPath(shaderFileName).c_str(),
						nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
						"VSMain", "vs_5_0", compileFlags, 0, &vertexShaderBlob, &errorBlob);
		if (FAILED(hr)) {
			std::string errStr;
			errStr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errStr.begin());
			LOG_ERROR("Graphics/shader", fmt::format("ShaderCompile Error: {}", errStr));
		}
		if (FAILED(hr = D3DCompileFromFile(GetAssetFullPath(shaderFileName).c_str(),
			nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShaderBlob, nullptr))) {
			std::string errStr;
			errStr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errStr.begin());
			LOG_ERROR("Graphics/shader", fmt::format("ShaderCompile Error: {}", errStr));
		}

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		if (FAILED(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)))) {
			LOG_ERROR("Graphics/init", "Failed CreateGraphicsPipelineState()");
		}

		if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(),
			m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)))) {
			LOG_ERROR("Graphics/init", "Failed CreateCommandList()");
		}
		
		// 頂点データを作成する
		{
			Vertex triangleVertices[] = {
				{ {0.0f, 0.25f * m_aspectRatio, 0.0f}, {0.5f, 0.0f} },
				{ {0.25f, -0.25f * m_aspectRatio, 0.0f}, {1.0f, 1.0f} },
				{ {-0.25f, -0.25f * m_aspectRatio, 0.0f}, {0.0f, 1.0f} },
			};
			const UINT vertexBufferSize = sizeof(triangleVertices);
			
			// NOTE: 本来静的リソースはHEAP_TYPE_DEFAULTのほうがアクセスが高速なため
			// ここでは簡単のためUPLOADヒープを使用する
			const auto heapPropertyUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			const auto vertBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
			if (FAILED(m_device->CreateCommittedResource(
				&heapPropertyUpload,
				D3D12_HEAP_FLAG_NONE,
				&vertBufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&m_vertexBuffer)))) {
				LOG_ERROR("Graphics/init", "Failed Create VertexData Resources.");
			}

			// 頂点バッファをGPUにコピー
			UINT8* pVertexDataBegin;
			CD3DX12_RANGE readRange(0, 0);
			if (FAILED(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)))) {
				LOG_ERROR("Graphics/init", "Failed to copy veretexBuffer to GPU.");
			}
			memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));

			// 頂点バッファビューを初期化
			m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
			m_vertexBufferView.StrideInBytes = sizeof(Vertex);
			m_vertexBufferView.SizeInBytes = vertexBufferSize;
		}

		ComPtr<ID3D12Resource> textureUploadHeap;
		{
			// テクスチャ作成
			D3D12_RESOURCE_DESC textureDesc{};
			textureDesc.MipLevels = 1;
			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			textureDesc.Width = TextureWidth;
			textureDesc.Height = TextureHeight;
			textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			textureDesc.DepthOrArraySize = 1;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			
			auto heapPropertyDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			auto heapPropertyUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			if (FAILED(m_device->CreateCommittedResource(
				&heapPropertyDefault, D3D12_HEAP_FLAG_NONE,
				&textureDesc, D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr, IID_PPV_ARGS(&m_texture)))) {
				LOG_ERROR("graphcs/init", "Failed to CreateCommittedResource() for texture");
			}

			// GPUにアップロードするためのバッファを作成する
			const auto resourceBuffer = CD3DX12_RESOURCE_DESC::Buffer(GetRequiredIntermediateSize(m_texture.Get(), 0, 1));
			if (FAILED(m_device->CreateCommittedResource(
				&heapPropertyUpload, D3D12_HEAP_FLAG_NONE,
				&resourceBuffer, D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&textureUploadHeap)))) {
				LOG_ERROR("graphics/init", "Failed to CreateCommittedResource() for textureUploadHeap");
			}

			// 中間アップロードヒープにデータをコピーして
			// アップロードヒープからTexture2Dへのデータコピーを予約する
			std::vector<UINT8> texture = GenerateTextureData();
			D3D12_SUBRESOURCE_DATA textureData{};
			textureData.pData = &texture[0];
			textureData.RowPitch = TextureWidth * TexturePixelSize;
			textureData.SlicePitch = textureData.RowPitch * TextureHeight;

			UpdateSubresources(m_commandList.Get(), m_texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);
			auto textureResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			m_commandList->ResourceBarrier(1, &textureResourceBarrier);
			
			// テクスチャ用のSRVを作成する
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = textureDesc.Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			m_device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());
		}

		if (FAILED(m_commandList->Close())) {
			LOG_ERROR("Graphics/init", "Failed commandList::close()");
		}
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		
		// 同期用オブジェクトを作成して、アセットをGPUに送りきるまで待つ
		{
			if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)))) {
				LOG_ERROR("Graphics/init", "Failed CreateFence()");
			}
			m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

			WaitForPreviousFrame();
		}
	}
}

void fne::DX12DrawTextureApp::PopulateCommandList()
{
	if (FAILED((m_commandAllocator->Reset()))) {
		LOG_ERROR("graphics/command", "Failed to reset commnadAllocator");
	}
	if (FAILED((m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get())))) {
		LOG_ERROR("graphics/command", "Failed to reset commandList");
	}

	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargets[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &resourceBarrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(3, 1, 0, 0);

	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &resourceBarrier);

	if (FAILED(m_commandList->Close())) {
		LOG_ERROR("graphics/command", "Failed to close command list.");
	}
}

void fne::DX12DrawTextureApp::WaitForPreviousFrame()
{
	const UINT64 fence = m_fenceValue;
	if (FAILED(m_commandQueue->Signal(m_fence.Get(), fence))) {
		LOG_ERROR("graphics/fence", "Failed to commandQueue->Signal()");
	}
	m_fenceValue++;

	if (m_fence->GetCompletedValue() < fence) {
		if (FAILED(m_fence->SetEventOnCompletion(fence, m_fenceEvent))) {
			LOG_ERROR("graphics/fence", "fence error");
		}
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

std::vector<UINT8> fne::DX12DrawTextureApp::GenerateTextureData()
{
	const UINT rowPitch = TextureWidth * TexturePixelSize;
    const UINT cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
    const UINT cellHeight = TextureWidth >> 3;    // The height of a cell in the checkerboard texture.
    const UINT textureSize = rowPitch * TextureHeight;

    std::vector<UINT8> data(textureSize);
    UINT8* pData = &data[0];

    for (UINT n = 0; n < textureSize; n += TexturePixelSize)
    {
        UINT x = n % rowPitch;
        UINT y = n / rowPitch;
        UINT i = x / cellPitch;
        UINT j = y / cellHeight;

        if (i % 2 == j % 2)
        {
            pData[n] = 0x00;        // R
            pData[n + 1] = 0x00;    // G
            pData[n + 2] = 0x00;    // B
            pData[n + 3] = 0xff;    // A
        }
        else
        {
            pData[n] = 0xff;        // R
            pData[n + 1] = 0xff;    // G
            pData[n + 2] = 0xff;    // B
            pData[n + 3] = 0xff;    // A
        }
    }

    return data;
}
