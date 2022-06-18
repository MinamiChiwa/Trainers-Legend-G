#pragma once

namespace MHotkey {
	bool get_uma_stat();
	void set_uma_stat(bool s);
	int start_hotkey(char sethotk = 'u');
	void setExtPluginPath(std::string ppath);
	void setUmaCommandLine(std::string args);
	void fopenExternalPlugin();
}
