#include "Win32Application.h"
#include "DX12AppBase.h"

HWND fne::Win32Application::m_hwnd = nullptr;

int fne::Win32Application::Run(DX12AppBase* pDxApp, HINSTANCE hInstance, int nCmdShow)
{
    // Parse the command line arguments
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    pDxApp->ParseCommandLineArgs(argv, argc);
    LocalFree(argv);

    // Initialize the window class
    WNDCLASSEX windowClass{};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = L"Win32Application";
    RegisterClassEx(&windowClass);

    RECT winRect = {
        0, 0, static_cast<LONG>(pDxApp->GetWidth()), static_cast<LONG>(pDxApp->GetHeight())
    };
    AdjustWindowRect(&winRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the window and store a handle to it.
    m_hwnd = CreateWindow(
        windowClass.lpszClassName,
        pDxApp->GetTitle(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        winRect.right - winRect.left,
        winRect.bottom - winRect.top,
        nullptr,        // We have no parent window.
        nullptr,        // We aren't using menus.
        hInstance,
        pDxApp
    );

    // Initialize the App. OnInit is defined in each child-implementation of DX12App
    pDxApp->OnInit();

    ShowWindow(m_hwnd, nCmdShow);

    // Main loop
    MSG msg{};
    while (msg.message != WM_QUIT) {
        // Process any messages in the queue.
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    // 終了処理
    pDxApp->OnDestroy();

    // Return this part of the WM_QUIT message to Windows.
    return static_cast<char>(msg.wParam);
}

LRESULT fne::Win32Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DX12AppBase* pDX12App = reinterpret_cast<DX12AppBase*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    
    switch (message)
    {
    case WM_CREATE:
	{
		// Save the DX12AppBase* passed in to CreateWindow.
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
	}
        return 0;

    case WM_KEYDOWN:
        if (pDX12App) {
            pDX12App->OnKeyDown(static_cast<UINT8>(wParam));
        }
        return 0;

    case WM_KEYUP:
        if (pDX12App) {
            pDX12App->OnKeyUp(static_cast<UINT8>(wParam));
        }
        return 0;

    case WM_PAINT:
        if (pDX12App) {
            pDX12App->OnUpdate();
            pDX12App->OnRender();
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);

}
