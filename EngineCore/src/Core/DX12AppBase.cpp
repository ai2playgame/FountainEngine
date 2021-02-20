#include "pch.h"
#include "DX12AppBase.h"
#include "DX12CoreHelper.h"
#include "Win32Application.h"

using namespace Microsoft::WRL;

fne::DX12AppBase::DX12AppBase(UINT width, UINT height, std::wstring name)
    : m_width(width)
    , m_height(height)
    , m_title(name)
    , m_useWarpDevice(false)
{
    WCHAR assetsPath[512];
    GetAssetsPath(assetsPath, _countof(assetsPath));
    m_assetsPath = assetsPath;

    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

fne::DX12AppBase::~DX12AppBase()
{
}

// コマンドライン引数をパースするヘルパー関数
_Use_decl_annotations_
void fne::DX12AppBase::ParseCommandLineArgs(WCHAR* argv[], int argc)
{
    for (int i = 1; i < argc; ++i) {
        // -wrap（/wrap）オプションが有効な時、warpDevice有効
        if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 ||
            _wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
        {
            m_useWarpDevice = true;
            m_title = m_title + L"(WARP)";
        }
    }
}

std::wstring fne::DX12AppBase::GetAssetFullPath(LPCWSTR assetName)
{
    return m_assetsPath + assetName;
}

// Direct3D 12をサポートするハードウェアアダプタを獲得するヘルパ関数
// もしアダプタが見つからなければ、**ppAdapterにはnullptrが設定される
_Use_decl_annotations_
void fne::DX12AppBase::GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter)
{
    *ppAdapter = nullptr;
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIFactory6> factory6;
    
    if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6)))) {
        for (UINT adapterIndex = 0;
            factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                (requestHighPerformanceAdapter == true) ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS(&adapter))
            != DXGI_ERROR_NOT_FOUND;
            ++adapterIndex) {

            DXGI_ADAPTER_DESC1 adapterDesc1;
            adapter->GetDesc1(&adapterDesc1);

			// 通常のレンダードライバーアダプターとしてい選択してはならない
			// もしあなたがソフトウェアアダプターを使うのなら、"/warp"引数を渡す
            if (adapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            // Direct3D 12をサポートするアダプターかどうかチェック
            // ただし、ここでは実際にデバイスを作らない
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))) {
                break;
            }
        }
    }
    else {
        for (UINT adapterIndex = 0; pFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex) {
            DXGI_ADAPTER_DESC1 adapterDesc1;
            adapter->GetDesc1(&adapterDesc1);

			// ソフトウェアアダプタを使うときは"/warp"引数を渡して起動
            if (adapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            // Direct3D 12をサポートするアダプタかどうかチェック
            // ただし、ここでは実際にデバイスを作らない
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))) {
                break;
            }
        }
    }

    *ppAdapter = adapter.Detach();
}

// ウィンドウのタイトルテキストを設定する
void fne::DX12AppBase::SetCustomWindowText(LPCWSTR text)
{
    std::wstring windowText = m_title + L": " + text;
    SetWindowText(Win32Application::GetHwnd(), windowText.c_str());
}
