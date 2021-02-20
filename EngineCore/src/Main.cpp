//*********************************************************
//
// Main関数
//
//*********************************************************

#include "pch.h"
#include "Graphics/DX12TriangleApp.h"
#include "Graphics/Win32Application.h"
#include "Core/Utility/Logger/Logger.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    using namespace fne;

    Logger::SetLogLevel(LogLevel::Debug);
    LOG_DEBUG("main", "ロギングテスト");

    DX12TriangleApp sample(1280, 720, L"D3D12 Hello Triangle");
    return Win32Application::Run(&sample, hInstance, nCmdShow);
}

