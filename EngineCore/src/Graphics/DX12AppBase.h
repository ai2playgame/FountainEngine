//*********************************************************
//
// DX12の基本機能をラップしたクラス
//
//*********************************************************

#pragma once
#include "pch.h"

namespace fne {

class DX12AppBase {
public:
	DX12AppBase(UINT width, UINT height, std::wstring name);
	virtual ~DX12AppBase();

	virtual void OnInit() = 0;
	virtual void OnUpdate() = 0;
	virtual void OnRender() = 0;
	virtual void OnDestroy() = 0;

	virtual void OnKeyDown(UINT8) {}
	virtual void OnKeyUp(UINT8) {}

	// アクセッサ
	UINT GetWidth() const { return m_width; }
	UINT GetHeight() const { return m_height; }
	const WCHAR* GetTitle() const { return m_title.c_str(); }

	void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);

protected:
	std::wstring GetAssetFullPath(LPCWSTR assetName);

	void GetHardwareAdapter(
		_In_ IDXGIFactory1* pFactory,
		_Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
		bool requestHighPerformanceAdapter = false
		);

	void SetCustomWindowText(LPCWSTR text);

	// ビューポートのサイズ
	UINT m_width, m_height;
	float m_aspectRatio;

	// アダプタ情報
	bool m_useWarpDevice;

private:

	std::wstring m_assetsPath;
	std::wstring m_title;

};

}
