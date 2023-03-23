#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include "stdinclude.hpp"
#include "umagui/guiLanguage.hpp"

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
RECT cacheRect{ 100, 100, 1250, 1000 };

std::map<void*, UmaGUiShowData::UmaRaceMotionData> umaRaceData{};
std::vector<UmaGUiShowData::SkillEventData> umaUsedSkillList{};
using namespace GuiTrans;

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
        if ((a.second.finallyRank != -1) || b.second.finallyRank != -1) {
            return cmpTwo(b.second.finallyRank, a.second.finallyRank);
        }
        return cmpTwo(a.second.distance, b.second.distance);
    }

    bool compareDistanceDesc(const UmaDataPair& a, const UmaDataPair& b) {
        if ((a.second.finallyRank != -1) || b.second.finallyRank != -1) {
            return a.second.finallyRank < b.second.finallyRank;
        }
        return a.second.distance > b.second.distance;
    }

    bool compareGateNo(const UmaDataPair& a, const UmaDataPair& b) {
        return cmpTwo(a.second.gateNo, b.second.gateNo);
    }

    bool compareName(const UmaDataPair& a, const UmaDataPair& b) {
        return cmpTwo(a.second.charaName, b.second.charaName);
    }

    bool compareRSpeed(const UmaDataPair& a, const UmaDataPair& b) {
        return cmpTwo(a.second.MoveDistance, b.second.MoveDistance);
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

    bool compareDistFront(const UmaDataPair& a, const UmaDataPair& b) {
        return cmpTwo(a.second.distanceFront, b.second.distanceFront);
    }

    bool compareLastSpurtStartDistance(const UmaDataPair& a, const UmaDataPair& b) {
        return cmpTwo(a.second.LastSpurtStartDistance, b.second.LastSpurtStartDistance);
    }

}

bool getUmaGuiDone() {
    return guiDone;
}

ImVec4 rgbaToImVec4 (float r, float g, float b, float a) {
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
};


auto cmp_func = UmaCmp::compareGateNo;

void imguiRaceMainLoop(ImGuiIO& io) {
    if (!showRaceWnd) return;

    static bool showKmH = false;
    std::vector<UmaDataPair> sortedData(umaRaceData.begin(), umaRaceData.end());
    std::sort(sortedData.begin(), sortedData.end(), UmaCmp::compareDistanceDesc);
    int rank = 1;
    float firstDst = 0.0f;
    float lastDst = 0.0f;
    for (auto& i : sortedData) {
        umaRaceData.at(i.first).setRank(rank);
        i.second.setRank(rank);
        if (rank == 1) {
            umaRaceData.at(i.first).setFrontAndFirstDist(0, 0);
            i.second.setFrontAndFirstDist(0, 0);
            firstDst = i.second.distance;
        }
        else {
            umaRaceData.at(i.first).setFrontAndFirstDist(lastDst - i.second.distance, firstDst - i.second.distance);
            i.second.setFrontAndFirstDist(lastDst - i.second.distance, firstDst - i.second.distance);
        }
        lastDst = i.second.distance;
        rank++;
    }

    if (ImGui::Begin("Race Info")) {

        std::vector<const char*> tableTitle{
            GetTrans("GateNo/CharaName"), GetTrans("Rank/Distance"), GetTrans("DistanceFrom Front/First"),
            GetTrans("InstantSpeed"), GetTrans("Rate"), GetTrans("HP Left"), GetTrans("IsLastSpurt/Start Distance"),
            GetTrans("LastSpeed"), GetTrans("Speed"), GetTrans("Stamina"), GetTrans("Pow"), GetTrans("Guts"), GetTrans("Wiz")
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

        static std::map<int, std::pair<int, ImVec4>> highlightGates{};
        static std::vector<ImVec4> colors{ 
            rgbaToImVec4(0xff, 0x6f, 0x00, 255),  // 橙
            rgbaToImVec4(0x14, 0xd1, 0x00, 255),  // 绿
            rgbaToImVec4(0x11, 0x44, 0xaa, 255),
            rgbaToImVec4(0x5d, 0xce, 0xc6, 255),
            rgbaToImVec4(0x9f, 0x3e, 0xd5, 255),
            rgbaToImVec4(0x71, 0x09, 0xaa, 255),
            rgbaToImVec4(0xcd, 0x00, 0x74, 255),
            rgbaToImVec4(0xff, 0x07, 0x00, 255),
            rgbaToImVec4(0x84, 0xb2, 0x2d, 255),
            rgbaToImVec4(0xff, 0x5c, 0x00, 255),
            rgbaToImVec4(0xff, 0x9c, 0x00, 255),
            rgbaToImVec4(0xed, 0x00, 0x2f, 255),
            rgbaToImVec4(0x00, 0xa7, 0x70, 255),
            rgbaToImVec4(0x00, 0xc1, 0x2b, 255),
            rgbaToImVec4(0xDE, 0x00, 0x52, 255),
            rgbaToImVec4(0xFF, 0x0D, 0x00, 255),
            rgbaToImVec4(0x07, 0x76, 0xA0, 255),
            rgbaToImVec4(0x00, 0xA6, 0x7C, 255),
            rgbaToImVec4(0x00, 0xBF, 0x32, 255),
        };
        static int lastColorIndex = colors.size() - 1;
        static auto selectNextColor = []() {
            if (highlightGates.size() >= colors.size()) return -1;

            lastColorIndex++;
            if (lastColorIndex >= colors.size()) {
                lastColorIndex = 0;
            }
            while (highlightGates.contains(lastColorIndex)) {
                if (lastColorIndex >= colors.size()) {
                    lastColorIndex = -1;
                }
                lastColorIndex++;
            }
            return lastColorIndex;
        };

        // 绘制表格内容
        for (const auto& i : sortedData) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const auto& umaData = i.second;

            auto fColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

            if (const auto iter = highlightGates.find(umaData.gateNo); iter != highlightGates.end()) {
                fColor = iter->second.second;
            }
            ImGui::PushStyleColor(ImGuiCol_Text, fColor);

            ImGui::Text("%d. %s%s", umaData.gateNo, umaData.charaName.c_str(), umaData.trainerName.empty() ? "" : std::format(" ({})", umaData.trainerName).c_str());
            
            if (ImGui::IsItemClicked()) {
                if (highlightGates.contains(umaData.gateNo)) {
                    highlightGates.erase(umaData.gateNo);
                }
                else {
                    auto nextColorIndex = selectNextColor();
                    if (nextColorIndex != -1) {
                        highlightGates.emplace(umaData.gateNo, std::make_pair(nextColorIndex, colors[nextColorIndex]));
                    }
                }
            }
            
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text(
                    std::format("{}: %f\n{}: %f\n{}: %f\n{}/{}: %.4f / %d\n{}/{}: %.4f / %d\n{}/{}: %.4f / %d\n{}/{}: %.4f / %d\n\
{}/{}: %.4f / %d\n", GetTrans("MinSpeed"), GetTrans("RaceBaseSpeed"), GetTrans("StartDashSpeedThreshold"),
GetTrans("BaseSpeed"), GetTrans("RawSpeed"), GetTrans("BaseStamina"), GetTrans("RawStamina"), 
GetTrans("BasePow"), GetTrans("RawPow"), GetTrans("BaseGuts"), GetTrans("RawGuts"),
GetTrans("BaseWiz"), GetTrans("RawWiz")).c_str(),
umaData.MinSpeed, umaData.RaceBaseSpeed, umaData.StartDashSpeedThreshold, umaData.BaseSpeed, umaData.RawSpeed,
umaData.BaseStamina, umaData.RawStamina, umaData.BasePow, umaData.RawPow, umaData.BaseGuts, umaData.RawGuts,
umaData.BaseWiz, umaData.RawWiz
);
                ImGui::EndTooltip();
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d / %.4f", umaData.rank, umaData.distance);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(
                    std::format("{}: %.4f", GetTrans("MoveDistance")).c_str(),
                    umaData.MoveDistance
                );
                ImGui::EndTooltip();
            }

            ImGui::TableSetColumnIndex(2);
            if (umaData.rank == 1) {
                ImGui::PushStyleColor(ImGuiCol_Text, fColor);
            }
            else {
                if (umaData.distanceFront < 0.5)
                    ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xfe, 0x36, 0x01, 255));
                else if (umaData.distanceFront < 1.0)
                    ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xFF, 0xc5, 0x45, 255));
                else
                    ImGui::PushStyleColor(ImGuiCol_Text, fColor);
            }
            ImGui::Text("%.2f / %.2f", umaData.distanceFront, umaData.distanceFirst);

            ImGui::PopStyleColor();

            ImGui::TableSetColumnIndex(3);
            const auto speedMpers = umaData.MoveDistance / umaData.deltatime;

            if (speedMpers <= 16.67)
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0x00, 0xb9, 0x45, 255));
            else if (speedMpers <= 19.44)
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0x64, 0xb4, 0xcf, 255));
            else if (speedMpers <= 22.22)
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xb7, 0x6f, 0xea, 255));
            else if (speedMpers <= 23.61)
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xc9, 0x37, 0xd3, 255));
            else if (speedMpers <= 25)
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xf3, 0x6b, 0xb3, 255));
            else if (speedMpers <= 26.38)
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xf3, 0x3f, 0x6e, 255));
            else
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xff, 0x31, 0x00, 255));

            if (showKmH) {
                ImGui::Text("%.2f km/h", speedMpers * 3.6);
            }
            else {
                ImGui::Text("%.2f m/s", speedMpers);
            }
            ImGui::PopStyleColor();
            
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(
                    std::format("{}: %.4f\n{}: %.4f", GetTrans("RunMotionSpeed"), GetTrans("MoveDistance")).c_str(),
                    umaData.speed, umaData.MoveDistance
                );
                ImGui::EndTooltip();
            }

            ImGui::TableSetColumnIndex(4);
            if (umaData.rate < 1.0)
                ImGui::PushStyleColor(ImGuiCol_Text, fColor);
            else if (umaData.rate <= 1.2)
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0x64, 0xb4, 0xcf, 255));
            else if (umaData.rate <= 1.35)
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0x92, 0x40, 0xd5, 255));
            else if (umaData.rate <= 1.4)
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xc9, 0x37, 0xd3, 255));
            else if (umaData.rate <= 1.5)
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xe7, 0x00, 0x3e, 255));
            else
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xff, 0x31, 0x00, 255));

            ImGui::Text("%f", umaData.rate);

            ImGui::PopStyleColor();

            ImGui::TableSetColumnIndex(5);
            if (umaData.HpPer <= 0.001)
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xea, 0x00, 0x37, 255));
            else if (umaData.HpPer <= 0.06)
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xff, 0x35, 0x00, 255));
            else if (umaData.HpPer <= 0.12)
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xff, 0xe8, 0x00, 255));
            else
                ImGui::PushStyleColor(ImGuiCol_Text, fColor);

            ImGui::Text("%.1f (%.2f%s) / %.4f", umaData.Hp, umaData.HpPer * 100, "%", umaData.MaxHp);
            ImGui::PopStyleColor();

            ImGui::TableSetColumnIndex(6);
            if (umaData.isLastSpurt)
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xfe, 0x95, 0x9a, 255));
            else
                ImGui::PushStyleColor(ImGuiCol_Text, fColor);

            ImGui::Text("%s", umaData.isLastSpurt ? "Yes" : "No");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("/ %.4f", umaData.LastSpurtStartDistance);

            ImGui::TableSetColumnIndex(7);
            ImGui::Text("%.4f", umaData.lastSpeed);

            ImGui::TableSetColumnIndex(8);
            ImGui::Text("%.4f", umaData.Speed);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(
                    std::format("{}: %f\n{}: %f\n{}: %.4f\n{}: %d",
                        GetTrans("MinSpeed"), GetTrans("RaceBaseSpeed"), GetTrans("BaseSpeed"), GetTrans("RawSpeed")).c_str(),
                    umaData.MinSpeed, umaData.RaceBaseSpeed, umaData.BaseSpeed, umaData.RawSpeed
                );
                ImGui::EndTooltip();
            }

            ImGui::TableSetColumnIndex(9);
            ImGui::Text("%.4f", umaData.Stamina);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(
                    std::format("{}:%.4f\n{}: %d", GetTrans("BaseStamina"), GetTrans("RawStamina")).c_str(),
                    umaData.BaseStamina, umaData.RawStamina
                );
                ImGui::EndTooltip();
            }

            ImGui::TableSetColumnIndex(10);
            ImGui::Text("%.4f", umaData.Pow);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(
                    std::format("{}:%.4f\n{}: %d", GetTrans("BasePow"), GetTrans("RawPow")).c_str(),
                    umaData.BasePow, umaData.RawPow
                );
                ImGui::EndTooltip();
            }

            ImGui::TableSetColumnIndex(11);
            ImGui::Text("%.4f", umaData.Guts);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(
                    std::format("{}:%.4f\n{}: %d", GetTrans("BaseGuts"), GetTrans("RawGuts")).c_str(),
                    umaData.BaseGuts, umaData.RawGuts
                );
                ImGui::EndTooltip();
            }

            ImGui::TableSetColumnIndex(12);
            ImGui::Text("%.4f", umaData.Wiz);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(
                    std::format("{}: %.4f\n{}: %d", GetTrans("BaseWiz"), GetTrans("RawWiz")).c_str(),
                    umaData.BaseWiz, umaData.RawWiz
                );
                ImGui::EndTooltip();
            }
            // ImGui::NextColumn();
            ImGui::PopStyleColor();
        }

        // 指定排序规则
        ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs();
        if (sorts_specs) {
            if (sorts_specs->SpecsCount > 0) {
                sortDirection = sorts_specs->Specs->SortDirection;
                switch (sorts_specs->Specs[0].ColumnIndex) {
                case 0: cmp_func = UmaCmp::compareGateNo; break;
                case 1: cmp_func = UmaCmp::compareDistance; break;
                case 2: cmp_func = UmaCmp::compareDistFront; break;
                case 3: cmp_func = UmaCmp::compareRSpeed; break;
                case 4: cmp_func = UmaCmp::compareRate; break;
                case 5: cmp_func = UmaCmp::compareHp; break;
                case 6: cmp_func = UmaCmp::compareLastSpurtStartDistance; break;
                case 7: cmp_func = UmaCmp::compareLastspeed; break;
                case 8: cmp_func = UmaCmp::compareSpeed; break;
                case 9: cmp_func = UmaCmp::compareStamina; break;
                case 10: cmp_func = UmaCmp::comparePow; break;
                case 11: cmp_func = UmaCmp::compareGuts; break;
                case 12: cmp_func = UmaCmp::compareWiz; break;
                }
            }
        }

        ImGui::EndTable();
    }

    ImGui::Text(GetTrans("Keep Top"));
    ImGui::SameLine();
    ImGui::Checkbox("###top", &nowTopStat);    
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::Text(GetTrans("Auto Close Window"));
    ImGui::SameLine();
    ImGui::Checkbox("###autoclose", &closeWhenRaceEnd);
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::Text(GetTrans("Show km/h"));
    ImGui::SameLine();
    ImGui::Checkbox("###showkmh", &showKmH);    
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::Text(GetTrans("Ignore Negative Speed"));
    ImGui::SameLine();
    ImGui::Checkbox("###ignoreNegativeSpeed", &ignoreNegativeSpeed);

    static const char* items[] = { "English", "简体中文", "繁體中文" };
    static int current_item = checkDefaultLang();

    ImGui::Text("Language");
    ImGui::SameLine();
    ImGui::Combo("##", &current_item, items, IM_ARRAYSIZE(items));
    switch (current_item) {
    case 0:
        GuiLanguage = GUILangType::ENGLISH; break;    
    case 1:
        GuiLanguage = GUILangType::SCHINESE; break;    
    case 2:
        GuiLanguage = GUILangType::TCHINESE; break;
    }

    changeTopState();
    ImGui::End();
}

std::unordered_map<int, std::string> abilityTypes{
    { 0x0, "None" },
    { 0x1, "Speed" },
    { 0x2, "Stamina" },
    { 0x3, "Power" },
    { 0x4, "Guts" },
    { 0x5, "Wiz" },
    { 0x6, "RunningStyleExOonige" },
    { 0x7, "HpDecRate" },
    { 0x8, "VisibleDistance" },
    { 0x9, "HpRate" },
    { 0xa, "StartDash" },
    { 0xb, "ForceOvertakeIn" },
    { 0xc, "ForceOvertakeOut" },
    { 0xd, "TemptationEndTime" },
    { 0xe, "StartDelayFix" },
    { 0xf, "NOUSE_13" },
    { 0x10, "NOUSE_14" },
    { 0x11, "NOUSE" },
    { 0x12, "NOUSE_3" },
    { 0x13, "NOUSE_21" },
    { 0x14, "NOUSE_8" },
    { 0x15, "CurrentSpeed" },
    { 0x16, "CurrentSpeedWithNaturalDeceleration" },
    { 0x17, "NOUSE_2" },
    { 0x18, "NOUSE_4" },
    { 0x19, "NOUSE_7" },
    { 0x1a, "NOUSE_5" },
    { 0x1b, "TargetSpeed" },
    { 0x1c, "LaneMoveSpeed" },
    { 0x1d, "TemptationPer" },
    { 0x1e, "PushPer" },
    { 0x1f, "Accel" },
    { 0x20, "AllStatus" },
    { 0x21, "NOUSE_10" },
    { 0x22, "NOUSE_20" },
    { 0x23, "TargetLane" },
    { 0x24, "ActivateRandomNormalAndRareSkill" },
    { 0x25, "ActivateRandomRareSkill" },
    { 0x26, "NOUSE_17" },
    { 0x27, "NOUSE_18" },
    { 0x1f5, "ChallengeMatchBonus_Old" },
    { 0x1f6, "ChallengeMatchBonusStatus" },
    { 0x1f7, "ChallengeMatchBonusMotivation" }
};

std::string transSkillAbilityType(int abilityType) {
    if (const auto iter = abilityTypes.find(abilityType); iter != abilityTypes.end()) {
        return GetTrans(iter->second.c_str());
    }
    return std::format("0x{:x}", abilityType);
}

void imGuiRaceSkillInfoMainLoop() {
    if (ImGui::Begin("Race Skills")) {
        std::vector<const char*> tableTitle{
            GetTrans("Gate/Name"), GetTrans("Skill"), GetTrans("AbilityType"), 
            GetTrans("Value"), GetTrans("Targets"), GetTrans("CoolDownTime")
        };

        const int num_rows = umaUsedSkillList.size();
        const int num_columns = tableTitle.size();  // 列数
        const std::vector thisLoopList(umaUsedSkillList.rbegin(), umaUsedSkillList.rend());

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 18));
        ImGui::BeginTable("UmaRaceSkills", num_columns, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit);

        for (const auto& i : tableTitle) {
            ImGui::TableSetupColumn(i, ImGuiTableColumnFlags_WidthFixed);
        }
        ImGui::TableHeadersRow();


        for (auto& it : thisLoopList) {
            static auto showSkillInfo = [](UmaGUiShowData::SkillEventData it) {
                ImGui::BeginTooltip();
                ImGui::Text(std::format("{} (Rarity: {} Lv.{})", it.skillName, it.rarity, it.skillLevel).c_str());

                std::vector<const char*> skillTableTitle{
                    GetTrans("AbilityType"), GetTrans("Value"), GetTrans("Targets")
                };

                const int numRows = it.skillAbilities.size();
                const int numColumns = skillTableTitle.size();  // 列数

                ImGui::BeginTable("UmaRaceSkillInfo", numColumns);
                for (const auto& iTitle : skillTableTitle) {
                    ImGui::TableSetupColumn(iTitle, ImGuiTableColumnFlags_WidthFixed);
                }
                ImGui::TableHeadersRow();

                for (const auto& ability : it.skillAbilities) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", transSkillAbilityType(ability.abilityType).c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.6f", ability.effValue);

                    ImGui::TableSetColumnIndex(2);
                    
                    std::stringstream ss;
                    int fCount = 0;
                    for (const auto& target : ability.targets) {
                        ss << std::format("{}. {}{} ", target.gateNo, target.charaName, 
                            target.trainerName.empty() ? "" : std::format(" ({})", target.trainerName));
                        fCount++;
                        if (fCount >= 3) {
                            fCount = 0;
                            ss << "\n";
                        }
                    }
                    ImGui::Text("%s", ss.str().c_str());
                }

                ImGui::EndTable();

                ImGui::EndTooltip();
            };

            ImGui::TableNextRow();

            if (it.rarity == 1)  // 白
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xff, 0xff, 0xff, 255));
            else if (it.rarity == 2)  // 金
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xff, 0xee, 0x40, 255));
            else if (it.rarity == 5 || it.rarity == 4)  // 固有
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xf0, 0x6c, 0x98, 255));
            else if (it.rarity == 6)  // 金技能升级
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xfd, 0x95, 0x9c, 255));
            else
                ImGui::PushStyleColor(ImGuiCol_Text, rgbaToImVec4(0xff, 0xff, 0xff, 255));

            ImGui::TableSetColumnIndex(0);
            ImGui::Text(std::format("{}. {}{}", it.currGateNo, it.horseName, it.tName).c_str());
            if (ImGui::IsItemHovered()) showSkillInfo(it);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s (Rarity: %d Lv.%d)", it.skillName.c_str(), it.rarity, it.skillLevel);
            if (ImGui::IsItemHovered()) showSkillInfo(it);

            ImGui::TableSetColumnIndex(2);
            std::stringstream ss;
            for (const auto& iab : it.skillAbilities) {
                ss << transSkillAbilityType(iab.abilityType) << "\n";
            }
            ImGui::Text(ss.str().c_str());
            ss.str("");
            if (ImGui::IsItemHovered()) showSkillInfo(it);

            ImGui::TableSetColumnIndex(3);
            for (const auto& iab : it.skillAbilities) {
                ss << iab.effValue << "\n";
            }
            ImGui::Text(ss.str().c_str());
            ss.str("");
            if (ImGui::IsItemHovered()) showSkillInfo(it);

            ImGui::TableSetColumnIndex(4);
            for (const auto& iab : it.skillAbilities) {
                for (const auto& target : iab.targets) {
                    ss << std::format("{}. {}{} ", target.gateNo, target.charaName,
                        target.trainerName.empty() ? "" : std::format(" ({})", target.trainerName));
                }
                ss << "\n";
            }
            ImGui::Text(ss.str().c_str());
            ss.str("");
            if (ImGui::IsItemHovered()) showSkillInfo(it);

            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%f", it.cooldownTime);
            if (ImGui::IsItemHovered()) showSkillInfo(it);

            ImGui::PopStyleColor();
        }

        ImGui::EndTable();
        ImGui::PopStyleVar();
    }
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
    builder.AddRanges(io.Fonts->GetGlyphRangesKorean());
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    builder.AddText("○◎△×☆");
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
        imguiRaceMainLoop(io);
        imGuiRaceSkillInfoMainLoop();

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