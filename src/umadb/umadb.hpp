#pragma once
#include <unordered_map>

namespace UmaDatabase {
	std::unordered_map<std::string, std::string>* getUmaFileAssetHash();
	void executeQueryRes();
	std::string bundleNameToPath(std::string bundleName);
	std::pair<std::wstring, std::string> origPathToNewPath(std::wstring origPath);

	void setBundleHandleTargetCache(std::wstring bundlePath, void* target);
	void* getBundleHandleTargetCache(std::wstring bundlePath, bool isDbPath=false);
}