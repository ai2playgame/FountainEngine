//*********************************************************
//
// Main関数
//
//*********************************************************

#include "pch.h"
#include "Graphics/DX12TriangleApp.h"
#include "Graphics/Win32Application.h"
#include <fmt/core.h>
#include "Core/Utility/Logger/Logger.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    using namespace fne;

    Logger::SetLogLevel(LogLevel::Debug);
    LOG_FATAL("main", "logその1");
    LOG_ERROR("main/category", "logその2");
    LOG_WARN("main/category2", "logその3");
    LOG_INFO("main", "logその4");
    LOG_DEBUG("main", "logその5");

    DX12TriangleApp sample(1280, 720, L"D3D12 Hello Triangle");
    return Win32Application::Run(&sample, hInstance, nCmdShow);
}

