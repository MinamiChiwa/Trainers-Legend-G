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
    }
    
    /*
    void showmessagebox() {
        MSGBOXPARAMSW msgBox;
        msgBox.cbSize = sizeof(MSGBOXPARAMS);
        msgBox.dwStyle = MB_USERICON | MB_OK | MB_SYSTEMMODAL;
        msgBox.hInstance = NULL;
        msgBox.hwndOwner = NULL;
        msgBox.lpszCaption = L"测试标题";
        msgBox.lpszIcon = MAKEINTRESOURCE(JI_BITMAP0);
        msgBox.lpszText = L"测试内容";

        MessageBoxIndirect(&msgBox);
    }
    */
    

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
                    MHotkey::is_uma = !MHotkey::is_uma;
                    auto pt = MHotkey::is_uma ? L"已切换为: 无视换行符优先(竖屏推荐)" : L"已切换为: 使用原换行配置优先(横屏推荐)";
                    // showmessagebox();
                    MessageBoxW(NULL, 
                        pt,
                        L"文本显示配置 Modified by 'sunset",
                        MB_OK|MB_SYSTEMMODAL);
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
