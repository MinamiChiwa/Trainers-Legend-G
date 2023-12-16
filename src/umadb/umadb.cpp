#include <stdinclude.hpp>
#include <sqlite3.h>
#include <format>
#include <boost/algorithm/string/join.hpp>


namespace UmaDatabase {
    namespace {
        void* AssetBundle_GetAllAssetNames = nullptr;
        void* AssetBundle_LoadFromFile = nullptr;
        void* AssetBundle_Unload = nullptr;
        void* Object_IsNativeObjectAlive = nullptr;

        const auto basePath = g_enable_custom_PersistentDataPath ? g_custom_PersistentDataPath : std::format("{}/AppData/LocalLow/Cygames/umamusume", getenv("UserProfile"));
        std::unordered_map<std::string, std::string> umaFileAssetHash{};  // dbPath: bundleName
        // std::unordered_map<std::string, std::list<std::wstring>> bundleNames{};  // dbName: [bundlePath1, bundlePath2, ...]
        std::map<std::wstring, std::string> pathBundle{};  // bundlePath: bundleName
        std::unordered_map<std::wstring, void*> bundleHandleTargetCache{};
        std::unordered_map<int, int> umaDressHeadId{};
        std::unordered_map<int, int> umaDressHasMini{};
    }
    std::unordered_map<int, std::unordered_map<std::string, int>> umaSkillInfo{};

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
        bundleHandleTargetCache.emplace(bundlePath, target);
    }

    void* getBundleHandleTargetCache(std::wstring bundlePath, bool isDbPath) {
        if (Object_IsNativeObjectAlive == nullptr) {
            initPtr();
        }

        if (bundleHandleTargetCache.find(bundlePath) == bundleHandleTargetCache.end()) {
            return nullptr;
        }
        auto ret = bundleHandleTargetCache.at(bundlePath);
        if (!reinterpret_cast<bool(*)(void*)>(Object_IsNativeObjectAlive)(ret)) {
            bundleHandleTargetCache.erase(bundlePath);
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
                // printf("Row: %d, Col: %d\n", nRow, nCol);
                if ((nRow == 0) && (nCol == 0)) printf("Resource Not Found: %s\n", queryPath);
                sqlite3_free_table(pResult);
            }
        }
        sqlite3_close(db);
    }

    void executeQueryDress()
    {
        sqlite3* db;
        char* zErrMsg = 0;
        const char* data = "Getting Master Data...";

        const auto dbPath = std::format("{}/master/master.mdb", basePath);
        int rc = sqlite3_open(dbPath.c_str(), &db);
        if (rc) {
            printf("Can't open database: %s\n", sqlite3_errmsg(db));
            return;
        }
        umaFileAssetHash.clear();
        // bundleNames.clear();
        pathBundle.clear();


        auto sql = "SELECT id, head_sub_id, have_mini FROM dress_data";

        char** pResult;
        char* errmsg;
        int nRow;
        int nCol;

        rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, &zErrMsg);
        if (rc != SQLITE_OK) {
            printf("SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return;
        }
        umaDressHeadId.clear();

        int index = 0;
        for (int n = 0; n < nRow; n++)
        {
            int umaDressId = -114514;
            int umaHeadId = 0;
            int haveMini = 0;
            bool hasErr = false;
            for (int j = 0; j < nCol; j++)
            {
                try {
                    auto value = pResult[index];
                    switch (j)
                    {
                    case 0:
                        umaDressId = std::stoi(value); break;
                    case 1:
                        umaHeadId = std::stoi(value); break;
                    case 2:
                        haveMini = std::stoi(value); break;
                    default:
                        break;
                    }
                }
                catch (std::exception& e) {
                    // printf("err: %s\n", e.what());
                    hasErr = true;
                }
                index++;
            }
            if (hasErr) continue;
            umaDressHeadId.emplace(umaDressId, umaHeadId);
            umaDressHasMini.emplace(umaDressId, haveMini);
        }
        sqlite3_free_table(pResult);
        sqlite3_close(db);
    }

    void executeQuerySkillData()
    {
        sqlite3* db;
        char* zErrMsg = 0;
        const char* data = "Getting MasterSkillData...";

        const auto dbPath = std::format("{}/master/master.mdb", basePath);
        int rc = sqlite3_open(dbPath.c_str(), &db);
        if (rc) {
            printf("Can't open database: %s\n", sqlite3_errmsg(db));
            return;
        }

        static std::vector<std::string> tableHeads = { "id", 
                                                       "ability_type_1_1", "ability_type_1_2", "ability_type_1_3", "ability_type_2_1", "ability_type_2_2", "ability_type_2_3",
                                                       "float_ability_value_1_1", "float_ability_value_1_2", "float_ability_value_1_3", 
                                                       "float_ability_value_2_1", "float_ability_value_2_2", "float_ability_value_2_3",
                                                       "float_cooldown_time_1", "float_cooldown_time_2",
                                                       "target_type_1_1", "target_type_1_2", "target_type_1_3",
                                                       "target_type_2_1", "target_type_2_2", "target_type_2_3" };
        
        const auto joinedRows = boost::algorithm::join(tableHeads, ",");
        auto sql = std::format("SELECT {} FROM skill_data", joinedRows);

        char** pResult;
        char* errmsg;
        int nRow;
        int nCol;

        rc = sqlite3_get_table(db, sql.c_str(), &pResult, &nRow, &nCol, &zErrMsg);
        if (rc != SQLITE_OK) {
            printf("SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return;
        }
        umaSkillInfo.clear();

        int index = 0;
        for (int n = 0; n < nRow; n++)
        {
            int currId = 0;
            bool hasErr = false;
            for (int j = 0; j < nCol; j++)
            {
                try {
                    auto value = pResult[index];
                    switch (j) {
                    case 0: {  // id
                        currId = std::stoi(value);
                        std::unordered_map<std::string, int> dic{};
                        umaSkillInfo.emplace(currId, std::move(dic));
                    }; break;
                    default: {
                        const auto& headName = tableHeads[j];
                        umaSkillInfo[currId].emplace(headName, std::stoi(value));
                    }
                    }
                }
                catch (std::exception& e) {
                    printf("executeQuerySkillData err: %s\n", e.what());
                    hasErr = true;
                }
                index++;
            }
            if (hasErr) continue;
        }
        sqlite3_free_table(pResult);
        sqlite3_close(db);
    }


    int get_head_id_from_dress_id(int dressId) {
        if (umaDressHeadId.empty()) {
            executeQueryDress();
        }

        if (umaDressHeadId.contains(dressId)) {
            return umaDressHeadId.at(dressId);
        }
        printf("uma dressId not found: %d\n", dressId);
        return 0;
    }

    bool get_dress_have_mini(int dressId) {
        if (umaDressHasMini.empty()) {
            executeQueryDress();
        }
        if (umaDressHasMini.contains(dressId)) {
            return umaDressHasMini.at(dressId);
        }
        return false;
    }


}
