//*********************************************************
//
// Win32アプリケーションを起動するためのラッパークラス
//
//*********************************************************

#pragma once

#include "pch.h"

namespace fne {

// Forward Declaration
class DX12AppBase;

class Win32Application {
public:
	static int Run(DX12AppBase* pDxApp, HINSTANCE hInstance, int nCmdShow);
	static HWND GetHwnd() { return m_hwnd; }

protected:
	// ウィンドウプロシージャ関数
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	static HWND m_hwnd;

};

}

