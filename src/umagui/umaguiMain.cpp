#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include "stdinclude.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

// Data
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool guiDone = true;
bool showRaceWnd = true;
bool closeWhenRaceEnd = false;

HWND hwnd;
RECT cacheRect{ 100, 100, 1250, 600 };

std::map<void*, UmaGUiShowData::UmaRaceMotionData> umaRaceData{};

void SetGuiDone(bool isDone) {
    guiDone = isDone;
}

void SetShowRaceWnd(bool value) {
    showRaceWnd = value;
}

void updateRaceGUIData(std::map<void*, UmaGUiShowData::UmaRaceMotionData>& data) {
    umaRaceData = data;
}

void SetWindowTop()
{
    ::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
}

void CancelWindowTop()
{
    ::SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
}

bool lastTopStat = false;
bool nowTopStat = false;
void changeTopState() {
    if (lastTopStat == nowTopStat) return;
    lastTopStat = nowTopStat;
    if (nowTopStat) {
        SetWindowTop();
    }
    else {
        CancelWindowTop();
    }

}

typedef std::pair<void*, UmaGUiShowData::UmaRaceMotionData> UmaDataPair;

int sortDirection = 0;

namespace UmaCmp{
    template<typename T>
    bool cmpTwo(T a, T b) {
        if (sortDirection == 2) return a > b;
        return a < b;
    }

    bool compareDistance(const UmaDataPair& a, const UmaDataPair& b) {
        return cmpTwo(a.second.distance, b.second.distance);
    }

    bool compareDistanceDesc(const UmaDataPair& a, const UmaDataPair& b) {
        return a.second.distance > b.second.distance;
    }

    bool compareGateNo(const UmaDataPair& a, const UmaDataPair& b) {
        return cmpTwo(a.second.gateNo, b.second.gateNo);
    }

    bool compareName(const UmaDataPair& a, const UmaDataPair& b) {
        return cmpTwo(a.second.charaName, b.second.charaName);
    }

    bool compareRSpeed(const UmaDataPair& a, const UmaDataPair& b) {
        return cmpTwo(a.second.speed, b.second.speed);
    }

    bool compareRate(const UmaDataPair& a, const UmaDataPair& b) {
        return cmpTwo(a.second.rate, b.second.rate);
    }

    bool compareHp(const UmaDataPair& a, const UmaDataPair& b) {
        return cmpTwo(a.second.Hp, b.second.Hp);
    }

    bool compareLastspeed(const UmaDataPair& a, const UmaDataPair& b) {
        return cmpTwo(a.second.lastSpeed, b.second.lastSpeed);
    }

    bool compareSpeed(const UmaDataPair& a, const UmaDataPair& b) {
        return cmpTwo(a.second.Speed, b.second.Speed);
    }

    bool compareStamina(const UmaDataPair& a, const UmaDataPair& b) {
        return cmpTwo(a.second.Stamina, b.second.Stamina);
    }

    bool comparePow(const UmaDataPair& a, const UmaDataPair& b) {
        return cmpTwo(a.second.Pow, b.second.Pow);
    }    
    
    bool compareGuts(const UmaDataPair& a, const UmaDataPair& b) {
        return cmpTwo(a.second.Guts, b.second.Guts);
    }
    
    bool compareWiz(const UmaDataPair& a, const UmaDataPair& b) {
        return cmpTwo(a.second.Wiz, b.second.Wiz);
    }

}

bool getUmaGuiDone() {
    return guiDone;
}

auto cmp_func = UmaCmp::compareGateNo;

void imguiRaceMainLoop() {
    if (!showRaceWnd) return;

    std::vector<UmaDataPair> sortedData(umaRaceData.begin(), umaRaceData.end());
    std::sort(sortedData.begin(), sortedData.end(), UmaCmp::compareDistanceDesc);
    int rank = 1;
    for (auto& i : sortedData) {
        umaRaceData.at(i.first).setRank(rank);
        i.second.setRank(rank);
        rank++;
    }

    if (ImGui::Begin("Race Info")) {

        static std::vector<const char*> tableTitle{
            "GateNo", "Rank/Distance", "CharaName", "Speed", "Rate", "HP Left" ,"LastSpeed", "Speed", "Stamina", "Pow", "Guts", "Wiz"
        };

        const int num_rows = sortedData.size();
        const int num_columns = tableTitle.size();  // 列数

        ImGui::BeginTable("UmaRaceInfo", num_columns, ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit);

        std::sort(sortedData.begin(), sortedData.end(), cmp_func);

        // 绘制表头
        auto headLine = 0;
        for (const auto& i : tableTitle) {
            if (headLine == 0)
                ImGui::TableSetupColumn(i, ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch);
            else
                ImGui::TableSetupColumn(i, ImGuiTableColumnFlags_WidthStretch);
            headLine++;
        }
        ImGui::TableHeadersRow();

        // 绘制表格内容
        for (const auto& i : sortedData)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            const auto& umaData = i.second;
            ImGui::Text("%d", umaData.gateNo);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d / %.4f", umaData.rank, umaData.distance);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(
                    "MoveDistance: %.4f",
                    umaData.MoveDistance
                );
                ImGui::EndTooltip();
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s%s", umaData.charaName.c_str(), umaData.trainerName.empty() ? "" : std::format(" ({})", umaData.trainerName).c_str());
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(
                    "MinSpeed: %f\nRaceBaseSpeed: %f\nStartDashSpeedThreshold: %f\nBaseSpeed/RawSpeed: %.4f / %d\n\
BaseStamina/RawStamina: %.4f / %d\nBasePow/RawPow: %.4f / %d\nBaseGuts/RawGuts: %.4f / %d\n\
BaseWiz/RawWiz: %.4f / %d\n",
umaData.MinSpeed, umaData.RaceBaseSpeed, umaData.StartDashSpeedThreshold, umaData.BaseSpeed, umaData.RawSpeed,
umaData.BaseStamina, umaData.RawStamina, umaData.BasePow, umaData.RawPow, umaData.BaseGuts, umaData.RawGuts,
umaData.BaseWiz, umaData.RawWiz
);
                ImGui::EndTooltip();
            }

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%f", umaData.speed);

            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%f", umaData.rate);

            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%.1f (%.2f%s) / %.4f", umaData.Hp, umaData.HpPer * 100, "%", umaData.MaxHp);

            ImGui::TableSetColumnIndex(6);
            ImGui::Text("%.4f", umaData.lastSpeed);

            ImGui::TableSetColumnIndex(7);
            ImGui::Text("%.4f", umaData.Speed);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(
                    "MinSpeed: %f\nRaceBaseSpeed: %f\nBaseSpeed: %.4f\nRawSpeed: %d",
                    umaData.MinSpeed, umaData.RaceBaseSpeed, umaData.BaseSpeed, umaData.RawSpeed
                );
                ImGui::EndTooltip();
            }

            ImGui::TableSetColumnIndex(8);
            ImGui::Text("%.4f", umaData.Stamina);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(
                    "BaseStamina:%.4f\nRawStamina: %d",
                    umaData.BaseStamina, umaData.RawStamina
                );
                ImGui::EndTooltip();
            }

            ImGui::TableSetColumnIndex(9);
            ImGui::Text("%.4f", umaData.Pow);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(
                    "BasePow:%.4f\nRawPow: %d",
                    umaData.BasePow, umaData.RawPow
                );
                ImGui::EndTooltip();
            }

            ImGui::TableSetColumnIndex(10);
            ImGui::Text("%.4f", umaData.Guts);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(
                    "BaseGuts:%.4f\nRawGuts: %d",
                    umaData.BaseGuts, umaData.RawGuts
                );
                ImGui::EndTooltip();
            }

            ImGui::TableSetColumnIndex(11);
            ImGui::Text("%.4f", umaData.Wiz);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(
                    "BaseWiz: %.4f\nRawWiz: %d",
                    umaData.BaseWiz, umaData.RawWiz
                );
                ImGui::EndTooltip();
            }
            ImGui::NextColumn();
        }

        // 指定排序规则
        ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs();
        if (sorts_specs) {
            if (sorts_specs->SpecsCount > 0) {
                sortDirection = sorts_specs->Specs->SortDirection;
                switch (sorts_specs->Specs[0].ColumnIndex) {
                case 0: cmp_func = UmaCmp::compareGateNo; break;
                case 1: cmp_func = UmaCmp::compareDistance; break;
                case 2: cmp_func = UmaCmp::compareName; break;
                case 3: cmp_func = UmaCmp::compareRSpeed; break;
                case 4: cmp_func = UmaCmp::compareRate; break;
                case 5: cmp_func = UmaCmp::compareHp; break;
                case 6: cmp_func = UmaCmp::compareLastspeed; break;
                case 7: cmp_func = UmaCmp::compareSpeed; break;
                case 8: cmp_func = UmaCmp::compareStamina; break;
                case 9: cmp_func = UmaCmp::comparePow; break;
                case 10: cmp_func = UmaCmp::compareGuts; break;
                case 11: cmp_func = UmaCmp::compareWiz; break;
                }
            }
        }

        ImGui::EndTable();
    }

    ImGui::Text("Keep Top");
    ImGui::SameLine();
    ImGui::Checkbox("###top", &nowTopStat);    
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::Text("Auto Close Window");
    ImGui::SameLine();
    ImGui::Checkbox("###autoclose", &closeWhenRaceEnd);
    changeTopState();

    ImGui::End();
}

// Main code
void guimain()
{
    guiDone = false;
    WNDCLASSW wc;
    RECT WindowRectangle;
    HWND GameHwnd;
    int WindowWide;
    int WindowHeight;

    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    if (!raceInfoTabAttachToGame) {
        WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"Trainers Legend G", NULL };
        ::RegisterClassExW(&wc);
        hwnd = ::CreateWindowW(wc.lpszClassName, L"TLG GUI", WS_OVERLAPPEDWINDOW, cacheRect.left, cacheRect.top,
            cacheRect.right - cacheRect.left, cacheRect.bottom - cacheRect.top, NULL, NULL, wc.hInstance, NULL);
        if (nowTopStat) SetWindowTop();
    }
    else {
        // 注册窗体类
        // 附加到指定窗体上
        wc.cbClsExtra = NULL;
        wc.cbWndExtra = NULL;
        wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
        wc.hCursor = LoadCursor(0, IDC_ARROW);
        wc.hIcon = LoadIcon(0, IDI_APPLICATION);
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpfnWndProc = (WNDPROC)WndProc;
        wc.lpszClassName = L" ";
        wc.lpszMenuName = L" ";
        wc.style = CS_VREDRAW | CS_HREDRAW;

        RegisterClassW(&wc);

        // 得到窗口句柄
        WindowRectangle;
        GameHwnd = FindWindowW(L"UnityWndClass", L"umamusume");
        GetWindowRect(GameHwnd, &WindowRectangle);
        WindowWide = WindowRectangle.right - WindowRectangle.left;
        WindowHeight = WindowRectangle.bottom - WindowRectangle.top;

        // 创建窗体
       //  HWND hwnd = ::CreateWindowW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, L" ", L" ", WS_POPUP, 1, 1, WindowWide, WindowHeight, 0, 0, wc.hInstance, 0);
        hwnd = ::CreateWindowW(wc.lpszClassName, L"", WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED | WS_POPUP, 1, 1, WindowWide, WindowHeight, NULL, NULL, wc.hInstance, NULL);
        LONG lWinStyleEx = GetWindowLong(hwnd, GWL_EXSTYLE);
        lWinStyleEx = lWinStyleEx | WS_EX_LAYERED;

        SetWindowLong(hwnd, GWL_EXSTYLE, lWinStyleEx);
        SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
        // 去掉标题栏及边框
        LONG_PTR Style = GetWindowLongPtr(hwnd, GWL_STYLE);
        Style = Style & ~WS_CAPTION & ~WS_SYSMENU & ~WS_SIZEBOX;
        SetWindowLongPtr(hwnd, GWL_STYLE, Style);
        // 至顶层窗口 最大化
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, WindowWide, WindowHeight, SWP_SHOWWINDOW);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        // 设置窗体可穿透鼠标
        // SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TRANSPARENT | WS_EX_LAYERED);

    }

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        printf("init D3D failed\n");
        CleanupDeviceD3D();
        if (raceInfoTabAttachToGame) ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return;
    }

    if (raceInfoTabAttachToGame) {
        // Show the window
        SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
        SetLayeredWindowAttributes(hwnd, 0, 0, LWA_COLORKEY);
    }

    ::ShowWindow(hwnd, SW_SHOW);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImFontGlyphRangesBuilder builder;

    ImFontConfig config;
    builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());
    builder.AddRanges(io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    builder.AddRanges(io.Fonts->GetGlyphRangesChineseFull());
    ImVector<ImWchar> glyphRanges;
    builder.BuildRanges(&glyphRanges);
    config.GlyphRanges = glyphRanges.Data;

    if (std::filesystem::exists("c:\\Windows\\Fonts\\msyh.ttc")) {
        io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\msyh.ttc", 18.0f, &config);
    }
    else
        io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f, &config);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 0.00f);

    // Main loop
    while (!guiDone)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                guiDone = true;
        }
        if (guiDone)
            break;


        if (raceInfoTabAttachToGame) {
            // 每次都将窗体置顶并跟随游戏窗体移动
            GetWindowRect(GameHwnd, &WindowRectangle);
            WindowWide = (WindowRectangle.right) - (WindowRectangle.left);
            WindowHeight = (WindowRectangle.bottom) - (WindowRectangle.top);
            DWORD dwStyle = GetWindowLong(GameHwnd, GWL_STYLE);
            if (dwStyle & WS_BORDER)
            {
                WindowRectangle.top += 23;
                WindowHeight -= 23;
            }

            // 跟随窗口移动
            MoveWindow(hwnd, WindowRectangle.left + 9, WindowRectangle.top + 9, WindowWide - 18, WindowHeight - 18, true);

        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();
        imguiRaceMainLoop();

        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
        // g_pSwapChain->Present(0, 0); // Present without vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_WARP, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SIZING: {
        RECT* rect = reinterpret_cast<RECT*>(lParam);
        cacheRect.left = rect->left;
        cacheRect.right = rect->right;
        cacheRect.top = rect->top;
        cacheRect.bottom = rect->bottom;
    }; break;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}