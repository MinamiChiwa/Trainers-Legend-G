#define _WIN32_WINNT 0x0400
#pragma comment( lib, "user32.lib" )

#include <windows.h>
#include <stdio.h>
#include <iostream>

namespace MHotkey{
    namespace {
        HHOOK hKeyboardHook;
        bool is_uma = false;
        char hotk = 'u';
        std::string extPluginPath = "";
        std::string umaArgs = "";
    }
    
    void fopenExternalPlugin() {
        if (MHotkey::extPluginPath == "") {
            printf("\"externalPlugin\" not found\n");
            return;
        }

        std::string cmdLine = extPluginPath + " " + MHotkey::umaArgs;
        char* commandLine = new char[255];
        strcpy(commandLine, cmdLine.c_str());

        printf("open external plugin: %s\n", commandLine);

        STARTUPINFOA startupInfo{ .cb = sizeof(STARTUPINFOA) };
        PROCESS_INFORMATION pi{};
        if (CreateProcessA(NULL, commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &pi)) {
            CloseHandle(pi.hThread);
            // WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
        }
        else {
            printf("Open external plugin failed.\n");
        }
    }
    

    __declspec(dllexport) LRESULT CALLBACK KeyboardEvent(int nCode, WPARAM wParam, LPARAM lParam)
    {
        DWORD SHIFT_key = 0;
        DWORD CTRL_key = 0;
        DWORD ALT_key = 0;

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

            if (key >= 'A' && key <= 'Z')
            {

                if (GetAsyncKeyState(VK_SHIFT) >= 0) key += 32;

                if (CTRL_key != 0 && key == hotk)
                {
                    fopenExternalPlugin();
                }

                // printf("key = %c\n", key);

                SHIFT_key = 0;
                CTRL_key = 0;
                ALT_key = 0;

            }

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

    int start_hotkey(char sethotk='u')
    {
        HANDLE hThread;
        DWORD dwThread;
        MHotkey::hotk = sethotk;

        hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)my_HotKey, (LPVOID)"", NULL, &dwThread);

        // ShowWindow(FindWindowA("ConsoleWindowClass", NULL), false);
        return 1;

        if (hThread) return WaitForSingleObject(hThread, INFINITE);
        else return 1;

    }
}
