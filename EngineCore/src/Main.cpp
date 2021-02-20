//*********************************************************
//
// Main関数
//
//*********************************************************

#include "pch.h"
#include "Graphics/DX12TriangleApp.h"
#include "Graphics/Win32Application.h"
#include <fmt/core.h>

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	auto text = fmt::format("The answer is {}.\n", 42);
    OutputDebugStringA(text.c_str());

    using namespace fne;
    DX12TriangleApp sample(1280, 720, L"D3D12 Hello Triangle");
    return Win32Application::Run(&sample, hInstance, nCmdShow);
}

