//*********************************************************
//
// Main関数
//
//*********************************************************

#include "pch.h"
#include "Core/DX12TriangleApp.h"
#include "Core/Win32Application.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    using namespace fne;
    DX12TriangleApp sample(1280, 720, L"D3D12 Hello Triangle");
    return Win32Application::Run(&sample, hInstance, nCmdShow);
}

