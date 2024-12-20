#define _WIN32_WINNT 0x0400
#pragma comment( lib, "user32.lib" )

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <format>
#include <functional>
#include <filesystem>
#include <TlHelp32.h>
#include "cpprest/details/basic_types.h"
#include "cpprest/details/http_helpers.h"

extern void (*testFunction)();
extern std::filesystem::path DLL_DIR;

namespace MHotkey{
    namespace {
        HHOOK hKeyboardHook;
        bool is_uma = false;
        char hotk = 'u';
        std::string extPluginPath = "";
        std::string umaArgs = "";
        bool openPluginSuccess = false;
        DWORD pluginPID = -1;
        int tlgport = 43215;
        bool ext_server_start = false;

        std::function<void(int, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD)> mKeyBoardCallBack = nullptr;
        std::function<void(int, int)> mKeyBoardRawCallBack = nullptr;
        bool hotKeyThreadStarted = false;
    }

    bool get_is_plugin_open() {
        return openPluginSuccess && ext_server_start;
    }

    void set_ext_server_start(const bool status)
    {
        ext_server_start = status;
    }

    void SetKeyCallBack(std::function<void(int, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD)> callbackfun) {
        mKeyBoardCallBack = callbackfun;
    }
    
    void fopenExternalPlugin_Legacy(int tlgPort) {
        std::thread([tlgPort](){
            if (openPluginSuccess) {
                printf("External plugin is already open.\n");
                return;
            }
            openPluginSuccess = true;

            std::string file_check_name = std::format("{}.autoupdate", extPluginPath);

            if (MHotkey::extPluginPath == "") {
                printf("\"externalPlugin\" not found\n");
                return;
            }
            if (std::filesystem::exists(file_check_name)) {
                try {
                    if (std::filesystem::exists(extPluginPath)) {
                        std::filesystem::remove(extPluginPath);
                    }
                    std::filesystem::rename(file_check_name, extPluginPath);
                }
                catch (std::exception& e) {
                    printf("update external plugin failed: %s\n", e.what());
                }
            }

            std::string cmdLine = std::format("{} {} --tlgport={} --resource_path=\"{}\"", extPluginPath, MHotkey::umaArgs, tlgPort, DLL_DIR.string());
            char* commandLine = new char[255];
            strcpy(commandLine, cmdLine.c_str());

            STARTUPINFOA startupInfo{ .cb = sizeof(STARTUPINFOA) };
            PROCESS_INFORMATION pi{};
            if (CreateProcessA(NULL, commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &pi)) {
                printf("open external plugin: %s (%lu)\n", commandLine, pi.dwProcessId);
                pluginPID = pi.dwProcessId;
                DWORD dwRetun = 0;
                WaitForSingleObject(pi.hProcess, INFINITE);
                GetExitCodeProcess(pi.hProcess, &dwRetun);
                printf("plugin exit: %d\n", dwRetun);
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
            }
            else {
                printf("Open external plugin failed.\n");
            }
            openPluginSuccess = false;
            ext_server_start = false;
        }).detach();
    }
       
    DWORD GetProcessIdByName(const std::wstring& processName)
    {
        DWORD pid = 0;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

        PROCESSENTRY32W pe = { 0 };
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(hSnapshot, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, processName.c_str()) == 0) {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnapshot, &pe));
        }
        CloseHandle(hSnapshot);
        return pid;
    }


    void fopenExternalPlugin(int tlgPort) {
        std::thread([tlgPort]() {
            if (openPluginSuccess) {
                printf("External plugin is already open.\n");
                return;
            }
            openPluginSuccess = true;

            std::string file_check_name = std::format("{}.autoupdate", extPluginPath);

            if (MHotkey::extPluginPath == "") {
                printf("\"externalPlugin\" not found\n");
                return;
            }
            if (std::filesystem::exists(file_check_name)) {
                try {
                    if (std::filesystem::exists(extPluginPath)) {
                        std::filesystem::remove(extPluginPath);
                    }
                    std::filesystem::rename(file_check_name, extPluginPath);
                }
                catch (std::exception& e) {
                    printf("update external plugin failed: %s\n", e.what());
                }
            }

            std::string cmdLine = std::format("{} {} --tlgport={} --resource_path=\"{}\"", extPluginPath, MHotkey::umaArgs, tlgPort, DLL_DIR.string());

            const std::string PIPE_NAME = R"(\\.\pipe\TlgExtPluginPipe)";

            std::cout << "Connecting to starter pipe..." << std::endl;

            HANDLE hPipe = CreateFile(
                PIPE_NAME.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0,                                // 不共享
                nullptr,                          // 默认安全属性
                OPEN_EXISTING,
                0,
                nullptr
            );

            if (hPipe == INVALID_HANDLE_VALUE) {
                std::cerr << "Failed to connect to named pipe. Error: " << GetLastError() << std::endl;
                return;
            }

            std::cout << "Connected to starter pipe." << std::endl;

            DWORD bytesWritten;
            WriteFile(hPipe, cmdLine.c_str(), cmdLine.size(), &bytesWritten, nullptr);

            while (true) {
                char buffer[1024];
                DWORD bytesRead;
                if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr)) {
                    buffer[bytesRead] = '\0';
                    // std::cout << "Received from server: " << buffer << std::endl;

                    try {
                        int getPid = std::stoi(buffer);
                        if (getPid > 0) {
                            pluginPID = getPid;
                            printf("open external plugin: %s (%lu)\n", cmdLine.c_str(), pluginPID);
                        }
                        else {
                            printf("Open external plugin failed.\n");
                            openPluginSuccess = false;
                        }
                    }
                    catch (std::exception& e) {
                        std::cerr << "Get pid failed" << std::endl;
                        openPluginSuccess = false;
                    }

                    const std::string requestExitCmd = "exit";
                    WriteFile(hPipe, requestExitCmd.c_str(), requestExitCmd.size(), &bytesWritten, nullptr);
                    break;
                }
                else {
                    std::cerr << "Read pipe failed. Error: " << GetLastError() << std::endl;
                    openPluginSuccess = false;
                    break;
                }
            }

            CloseHandle(hPipe);

            // openPluginSuccess = false;
            // ext_server_start = false;
            }).detach();

        // return fopenExternalPlugin_Legacy(tlgPort);
    }

    void closeExternalPlugin() {
        if (std::filesystem::exists(DLL_DIR / "dontcloseext.lock")) {
            std::filesystem::remove(DLL_DIR / "dontcloseext.lock");
            return;
        }
        if (openPluginSuccess && pluginPID != -1) {
            std::string cmdLine = std::format("taskkill /T /PID {}", pluginPID);
            char* commandLine = new char[255];
            strcpy(commandLine, cmdLine.c_str());
            STARTUPINFOA startupInfo{ .cb = sizeof(STARTUPINFOA) };
            PROCESS_INFORMATION pi{};
            CreateProcessA(NULL, commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &pi);
            printf("kill: %s\n", commandLine);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
    }

    bool setWindowPosOffset(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) {
        RECT* windowR = new RECT;
        RECT* clientR = new RECT;
        GetWindowRect(hWnd, windowR);
        GetClientRect(hWnd, clientR);

        return SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx + windowR->right - windowR->left - clientR->right,
            cy + windowR->bottom - windowR->top - clientR->bottom, uFlags);
    }

    void setMKeyBoardRawCallBack(std::function<void(int, int)> cbfunc) {
        mKeyBoardRawCallBack = cbfunc;
    }

    void Win32SetWindowPos(bool is_v) {
        printf("Win32SetWindowPos\n");
        auto window = FindWindow(NULL, "umamusume");

        RECT* windowR = new RECT;
        RECT* clientR = new RECT;
        GetWindowRect(window, windowR);
        GetClientRect(window, clientR);

        printf("windowR: left: %ld, right: %ld, top: %ld, bottom: %ld\n", windowR->left, windowR->right, windowR->top, windowR->bottom);
        printf("clientR: left: %ld, right: %ld, top: %ld, bottom: %ld\n", clientR->left, clientR->right, clientR->top, clientR->bottom);

        if (is_v) 
            setWindowPosOffset(window, HWND_NOTOPMOST, 875, 10, 444, 960, SWP_DEFERERASE);
        else
            setWindowPosOffset(window, HWND_NOTOPMOST, 10, 5, 1920, 888, SWP_DEFERERASE);
    }
    
    __declspec(dllexport) LRESULT CALLBACK KeyboardEvent(int nCode, WPARAM wParam, LPARAM lParam)
    {
        DWORD SHIFT_key = 0;
        DWORD CTRL_key = 0;
        DWORD ALT_key = 0;
        DWORD SPACE_key = 0;
        DWORD UP_key = 0;
        DWORD DOWN_key = 0;
        DWORD LEFT_key = 0;
        DWORD RIGHT_key = 0;


        if (nCode == HC_ACTION)
        {
            if ((wParam == WM_SYSKEYUP) || wParam == WM_KEYUP) {
                KBDLLHOOKSTRUCT hooked_key = *((KBDLLHOOKSTRUCT*)lParam);
                int key = hooked_key.vkCode;
                if (mKeyBoardRawCallBack != nullptr) mKeyBoardRawCallBack(wParam, key);
            }
            else if ((wParam == WM_SYSKEYDOWN) || (wParam == WM_KEYDOWN)) {
                KBDLLHOOKSTRUCT hooked_key = *((KBDLLHOOKSTRUCT*)lParam);
                DWORD dwMsg = 1;
                dwMsg += hooked_key.scanCode << 16;
                dwMsg += hooked_key.flags << 24;
                char lpszKeyName[1024] = { 0 };
                lpszKeyName[0] = '[';

                int i = GetKeyNameText(dwMsg, (lpszKeyName + 1), 0xFF) + 1;
                lpszKeyName[i] = ']';

                int key = hooked_key.vkCode;

                if (mKeyBoardRawCallBack != nullptr) mKeyBoardRawCallBack(wParam, key);
                SHIFT_key = GetAsyncKeyState(VK_SHIFT);
                CTRL_key = GetAsyncKeyState(VK_CONTROL);
                ALT_key = GetAsyncKeyState(VK_MENU);
                SPACE_key = GetAsyncKeyState(VK_SPACE);

                UP_key = GetAsyncKeyState(VK_UP);
                DOWN_key = GetAsyncKeyState(VK_DOWN);
                LEFT_key = GetAsyncKeyState(VK_LEFT);
                RIGHT_key = GetAsyncKeyState(VK_RIGHT);

                if (mKeyBoardCallBack != nullptr) {
                    mKeyBoardCallBack(key, SHIFT_key, CTRL_key, ALT_key, SPACE_key, UP_key, DOWN_key, LEFT_key, RIGHT_key);
                }

                if (key >= 'A' && key <= 'Z')
                {

                    if (GetAsyncKeyState(VK_SHIFT) >= 0) key += 32;

                    if (CTRL_key != 0 && key == hotk)
                    {
                        fopenExternalPlugin(tlgport);
                    }

                    if (CTRL_key != 0 && key == 'T') {
                        if (testFunction != nullptr) {
                            testFunction();
                        }
                    }

                    SHIFT_key = 0;
                    CTRL_key = 0;
                    ALT_key = 0;
                    SPACE_key = 0;
                    DWORD UP_key = 0;
                    DWORD DOWN_key = 0;
                    DWORD LEFT_key = 0;
                    DWORD RIGHT_key = 0;
                }
            }

        }
        return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
    }

    void MessageLoop()
    {
        MSG message;
        while (GetMessage(&message, NULL, 0, 0))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }

    DWORD WINAPI my_HotKey(LPVOID lpParm)
    {
        HINSTANCE hInstance = GetModuleHandle(NULL);
        if (!hInstance) hInstance = LoadLibrary((LPCSTR)lpParm);
        if (!hInstance) return 1;

        hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardEvent, hInstance, NULL);
        MessageLoop();
        UnhookWindowsHookEx(hKeyboardHook);
        return 0;
    }

    bool get_uma_stat() {
        return MHotkey::is_uma;
    }
    void set_uma_stat(bool s) {
        MHotkey::is_uma = s;
    }

    void setExtPluginPath(std::string ppath) {
        MHotkey::extPluginPath = ppath;
    }
    void setUmaCommandLine(std::string args) {
        MHotkey::umaArgs = args;
    }
    void setTlgPort(int port) {
        tlgport = port;
    }

    int start_hotkey(char sethotk='u')
    {
        MHotkey::hotk = sethotk;
        if (hotKeyThreadStarted) return 1;

        HANDLE hThread;
        DWORD dwThread;

        hotKeyThreadStarted = true;
        hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)my_HotKey, (LPVOID)"", NULL, &dwThread);

        // ShowWindow(FindWindowA("ConsoleWindowClass", NULL), false);
        return 1;

        if (hThread) return WaitForSingleObject(hThread, INFINITE);
        else return 1;

    }
}
