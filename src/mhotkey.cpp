#define _WIN32_WINNT 0x0400
#pragma comment( lib, "user32.lib" )

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <format>
#include <functional>

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

    bool check_file_exist(const std::string& name) {
        struct stat buffer;
        return (stat(name.c_str(), &buffer) == 0);
    }
    
    void fopenExternalPlugin(int tlgPort) {
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

            if (check_file_exist(file_check_name)) {
                if (check_file_exist(extPluginPath)) {
                    remove(extPluginPath.c_str());
                }
                if (rename(file_check_name.c_str(), extPluginPath.c_str()) != 0) {
                    printf("update external plugin failed\n");
                }
            }

            std::string cmdLine = std::format("{} {} --tlgport={}", extPluginPath, MHotkey::umaArgs, tlgPort);
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

    void closeExternalPlugin() {
        if (check_file_exist("dontcloseext.lock")) {
            remove("dontcloseext.lock");
            return;
        }
        if (openPluginSuccess && pluginPID != -1) {
            std::string cmdLine = std::format("taskkill /F /T /PID {}", pluginPID);
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

        if ((nCode == HC_ACTION) && ((wParam == WM_SYSKEYDOWN) || (wParam == WM_KEYDOWN)))
        {

            KBDLLHOOKSTRUCT hooked_key = *((KBDLLHOOKSTRUCT*)lParam);
            DWORD dwMsg = 1;
            dwMsg += hooked_key.scanCode << 16;
            dwMsg += hooked_key.flags << 24;
            char lpszKeyName[1024] = { 0 };
            lpszKeyName[0] = '[';

            int i = GetKeyNameText(dwMsg, (lpszKeyName + 1), 0xFF) + 1;
            lpszKeyName[i] = ']';

            int key = hooked_key.vkCode;

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

                // printf("key = %c\n", key);

                SHIFT_key = 0;
                CTRL_key = 0;
                ALT_key = 0;
                SPACE_key = 0;
                DWORD UP_key = 0;
                DWORD DOWN_key = 0;
                DWORD LEFT_key = 0;
                DWORD RIGHT_key = 0;
            }
            /* ¥∞ø⁄…Ë÷√≤‚ ‘¥˙¬Î
            if (key == 'i') {
                Win32SetWindowPos(true);
            }
            if (key == 'o') {
                Win32SetWindowPos(false);
            }
            */

            // printf("lpszKeyName = %s\n", lpszKeyName);
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
