#include <stdinclude.hpp>
#include <sqlite3.h>
#include <format>


namespace UmaDatabase {
    namespace {
        void* AssetBundle_GetAllAssetNames = nullptr;
        void* AssetBundle_LoadFromFile = nullptr;
        void* AssetBundle_Unload = nullptr;
        void* Object_IsNativeObjectAlive = nullptr;

        const auto basePath = std::format("{}/AppData/LocalLow/Cygames/umamusume", getenv("UserProfile"));
        std::unordered_map<std::string, std::string> umaFileAssetHash{};  // dbPath: bundleName
        // std::unordered_map<std::string, std::list<std::wstring>> bundleNames{};  // dbName: [bundlePath1, bundlePath2, ...]
        std::map<std::wstring, std::string> pathBundle{};  // bundlePath: bundleName
        std::unordered_map<std::string, void*> bundleHandleTargetCache{};
    }

    void initPtr() {
        AssetBundle_GetAllAssetNames = reinterpret_cast<void* (*)(void*)>(
            il2cpp_symbols::get_method_pointer(
                "UnityEngine.AssetBundleModule.dll", "UnityEngine",
                "AssetBundle", "GetAllAssetNames", 0
            )
            );

        AssetBundle_LoadFromFile = reinterpret_cast<void* (*)(Il2CppString*)>(
            il2cpp_symbols::get_method_pointer(
                "UnityEngine.AssetBundleModule.dll", "UnityEngine",
                "AssetBundle", "LoadFromFile", 1
            )
            );

        AssetBundle_Unload = reinterpret_cast<void(*)(void*, bool)>(
            il2cpp_symbols::get_method_pointer(
                "UnityEngine.AssetBundleModule.dll", "UnityEngine",
                "AssetBundle", "Unload", -1
            )
            );

        Object_IsNativeObjectAlive = reinterpret_cast<bool(*)(void*)>(
            il2cpp_symbols::get_method_pointer(
                "UnityEngine.CoreModule.dll", "UnityEngine", "Object", "IsNativeObjectAlive", 1
            )
            );
    }

    bool pathIsMatch(std::wstring bundlePath, std::string dbPath) {
        std::wstring dbPathW(dbPath.begin(), dbPath.end());
        return bundlePath.find(dbPathW) != -1;
    }

    std::wstring dbPathToBundlePath(std::string dbPath) {
        for (auto& i : pathBundle) {
            if (pathIsMatch(i.first, dbPath)) {
                return i.first;
            }
        }
        return L"";
    }

    void setBundleHandleTargetCache(std::wstring bundlePath, void* target) {
        if (pathBundle.find(bundlePath) == pathBundle.end()) {
            printf("setBundleHandleTargetCache: %ls not found\n", bundlePath.c_str());
            return;
        }

        auto bundleName = pathBundle.at(bundlePath);
        // printf("set bundle: %s cache at %p", bundleName.c_str(), target);
        bundleHandleTargetCache.emplace(bundleName, target);
    }

    void* getBundleHandleTargetCache(std::wstring bundlePath, bool isDbPath) {
        if (Object_IsNativeObjectAlive == nullptr) {
            initPtr();
        }

        std::string bundleName;
        
        if (isDbPath) {
            const auto newBundlePath = dbPathToBundlePath(std::string(bundlePath.begin(), bundlePath.end()));
            if (newBundlePath.empty()) {
                return nullptr;
            }

            if (pathBundle.find(newBundlePath) == pathBundle.end()) {
                // printf("getBundleHandleTargetCache: %ls not found\n", bundlePath.c_str());
                return nullptr;
            }
            bundleName = pathBundle.at(newBundlePath);
        }
        else {
            if (pathBundle.find(bundlePath) == pathBundle.end()) {
                // printf("getBundleHandleTargetCache: %ls not found\n", bundlePath.c_str());
                return nullptr;
            }
            bundleName = pathBundle.at(bundlePath);
        }
        

        printf("searchBundleName: %s\n", bundleName.c_str());

        if (bundleHandleTargetCache.find(bundleName) == bundleHandleTargetCache.end()) {
            printf("getBundleHandleTargetCache: Bundle %s not found\n", bundleName.c_str());
            return nullptr;
        }
        auto ret = bundleHandleTargetCache.at(bundleName);
        if (!reinterpret_cast<bool(*)(void*)>(Object_IsNativeObjectAlive)(ret)) {
            printf("bundle: %s death\n", bundleName.c_str());
            bundleHandleTargetCache.erase(bundleName);
            return nullptr;
        }
        return ret;
    }

    std::unordered_map<std::string, std::string>* getUmaFileAssetHash() {
        return &umaFileAssetHash;
    }

    std::string bundleNameToPath(std::string bundleName) {
        return std::format("{}/dat/{}/{}", basePath, bundleName.substr(0, 2), bundleName);
        /*
        const auto origPath = std::format("{}/dat/{}/{}", basePath, bundleName.substr(0, 2), bundleName);
        const auto newPath = std::format("./RedirectAsset/{}", bundleName);
        if (!std::filesystem::is_directory("./RedirectAsset")) {
            std::filesystem::create_directory("./RedirectAsset");
        }
        std::filesystem::copy_file(origPath, newPath, std::filesystem::copy_options::overwrite_existing);
        return newPath;
        */
    }

    /*
    bundle内路径, bundle整体本地路径
    */
    std::pair<std::wstring, std::string> origPathToNewPath(std::wstring origPath) {
        std::string newDbPath;
        std::string newBundleName;
        for (auto& i : g_replaceBuiltInAssets) {
            if (pathIsMatch(origPath, i.first)) {
                newDbPath = i.second;
                if (umaFileAssetHash.find(newDbPath) == umaFileAssetHash.end()) {
                    printf("dbPath: %s not found.", newDbPath.c_str());
                    continue;
                }
                newBundleName = umaFileAssetHash.at(newDbPath);
                break;
            }
        }
        if (newDbPath.empty()) {
            return std::make_pair(L"", "");
        }
        auto newBundlePath = dbPathToBundlePath(newDbPath);
        if (newBundlePath.empty()) {
            return std::make_pair(L"", "");
        }
        return std::make_pair(newBundlePath, newBundleName);
    }

    void initBundleAllNames(std::string bundleName) {
        if ((AssetBundle_LoadFromFile == nullptr) || (AssetBundle_GetAllAssetNames == nullptr)) {
            // printf("ptr is null\n");
            initPtr();
        }
        const auto bundleFile = reinterpret_cast<void* (*)(Il2CppString*)>(AssetBundle_LoadFromFile)(
            il2cpp_string_new(bundleName.c_str())
        );
        if (!bundleFile) {
            printf("init bundle failed: %s\n", bundleName.c_str());
            return;
        }
        const auto allAssetPaths = reinterpret_cast<void* (*)(void*)>(AssetBundle_GetAllAssetNames)(bundleFile);
        il2cpp_symbols::iterate_IEnumerable<Il2CppString*>(allAssetPaths, [bundleName](Il2CppString* path)
            {
                pathBundle.emplace(path->start_char, bundleName);
                /*
                if (bundleNames.find(bundleName) == bundleNames.end()) {
                    bundleNames.emplace(bundleName, std::list<std::wstring>{path->start_char});
                }
                else {
                    auto& lst = bundleNames.at(bundleName);
                    lst.emplace_back(path->start_char);
                }
                */
            }
        );

        auto assetBundleHandle = il2cpp_gchandle_new(bundleFile, false);
        reinterpret_cast<void(*)(void*, bool)>(AssetBundle_Unload)(il2cpp_gchandle_get_target(assetBundleHandle), true); 
    }

    void executeQueryRes()
    {
        sqlite3* db;
        char* zErrMsg = 0;
        const char* data = "Getting Resource Hash...";
        
        const auto dbPath = std::format("{}/meta", basePath);
        int rc = sqlite3_open(dbPath.c_str(), &db);
        if (rc) {
            printf("Can't open database: %s\n", sqlite3_errmsg(db));
            return;
        }
        umaFileAssetHash.clear();
        // bundleNames.clear();
        pathBundle.clear();

        for (auto& i : g_replaceBuiltInAssets) {
            for (int _n = 0; _n <= 1; _n++) {
                const char* queryPath = (_n ? i.second.c_str() : i.first.c_str());

                auto sql = sqlite3_mprintf("SELECT h FROM a WHERE n=%Q", queryPath);

                char** pResult;
                char* errmsg;
                int nRow;
                int nCol;

                // rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
                rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, &zErrMsg);
                sqlite3_free(sql);
                if (rc != SQLITE_OK) {
                    printf("SQL error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                    continue;
                }

                for (int n = 0; n < nRow; n++)
                {
                    for (int j = 0; j < nCol; j++)
                    {
                        // auto key = pResult[j];
                        auto value = pResult[nCol];
                        initBundleAllNames(bundleNameToPath(value));
                        printf("FindAsset: %s, At: %s\n", queryPath, value);
                        umaFileAssetHash.emplace(queryPath, value);
                    }
                }
                printf("Row: %d, Col: %d\n", nRow, nCol);
                sqlite3_free_table(pResult);
            }
        }
        sqlite3_close(db);
    }
}
