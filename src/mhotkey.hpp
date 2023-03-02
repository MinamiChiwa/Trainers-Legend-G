#pragma once

namespace MHotkey {
	void SetKeyCallBack(std::function<void(int, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD)> callbackfun);
	bool get_is_plugin_open();
	void set_ext_server_start(bool status);
	bool get_uma_stat();
	void set_uma_stat(bool s);
	int start_hotkey(char sethotk = 'u');
	void setExtPluginPath(std::string ppath);
	void setUmaCommandLine(std::string args);
	void setTlgPort(int port);
	void fopenExternalPlugin(int tlgPort);
	void closeExternalPlugin();
	void setMKeyBoardRawCallBack(std::function<void(int, int)> cbfunc);
}
