#include "imgui/imgui.h"
#include "stdinclude.hpp"

extern bool showLiveWnd;


#define INPUT_AND_SLIDER_FLOAT(label, data, min, max) \
    ImGui::SetNextItemWidth(inputFloatWidth);\
    ImGui::InputFloat("##"##label, data);\
    ImGui::SameLine();\
    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth - 1.0f);\
    ImGui::SliderFloat(label, data, min, max)
    

namespace LiveGUILoops {
    static float inputFloatWidth = 70.0f;

    void postEffectUpdateInfo_DOF_guiLoop() {
        if (!showLiveWnd) return;

        if (ImGui::Begin("Live DOF")) {
            ImGui::Checkbox("Use Game DOF Settings", &UmaGUiShowData::dofColtrollerFollowGame);
            ImGui::Checkbox("Enable DOF", &UmaGUiShowData::postEffectUpdateInfo_DOF.IsEnableDOF);
            
            INPUT_AND_SLIDER_FLOAT("Focal Size", &UmaGUiShowData::postEffectUpdateInfo_DOF.forcalSize, -60.00f, 60.00f);
            INPUT_AND_SLIDER_FLOAT("Blur Spread", &UmaGUiShowData::postEffectUpdateInfo_DOF.blurSpread, -60.00f, 60.00f);

            const char* dofBlurTypeItems[] = { "Horizon", "Mixed", "Disc", "BallBlur" };
            ImGui::Combo("DOF Blur Type", (int*)&UmaGUiShowData::postEffectUpdateInfo_DOF.dofBlurType, dofBlurTypeItems, 4);

            const char* dofQualityItems[] = { "Only Background", "Background and Foreground" };
            int dofQualityIndex = UmaGUiShowData::postEffectUpdateInfo_DOF.dofQuality == UmaGUiShowData::DofQuality::OnlyBackground ? 0 : 1;
            ImGui::Combo("DOF Quality", &dofQualityIndex, dofQualityItems, 2);
            UmaGUiShowData::postEffectUpdateInfo_DOF.dofQuality = (UmaGUiShowData::DofQuality)(dofQualityIndex == 0 ? UmaGUiShowData::DofQuality::OnlyBackground : UmaGUiShowData::DofQuality::BackgroundAndForeground);

            INPUT_AND_SLIDER_FLOAT("DOF Foreground Size", &UmaGUiShowData::postEffectUpdateInfo_DOF.dofForegroundSize, -60.00f, 60.00f);
            INPUT_AND_SLIDER_FLOAT("DOF Focal Point", &UmaGUiShowData::postEffectUpdateInfo_DOF.dofFocalPoint, -60.00f, 60.00f);
            INPUT_AND_SLIDER_FLOAT("DOF Smoothness", &UmaGUiShowData::postEffectUpdateInfo_DOF.dofSoomthness, -60.00f, 60.00f);
            ImGui::Checkbox("Use Focal Point", &UmaGUiShowData::postEffectUpdateInfo_DOF.isUseFocalPoint);
            INPUT_AND_SLIDER_FLOAT("Ball Blur Curve Factor", &UmaGUiShowData::postEffectUpdateInfo_DOF.BallBlurCurveFactor, -60.00f, 60.00f);
            INPUT_AND_SLIDER_FLOAT("Ball Blur Brightness Threshhold", &UmaGUiShowData::postEffectUpdateInfo_DOF.BallBlurBrightnessThreshhold, -60.00f, 60.00f);
            INPUT_AND_SLIDER_FLOAT("Ball Blur Brightness Intensity", &UmaGUiShowData::postEffectUpdateInfo_DOF.BallBlurBrightnessIntensity, -60.00f, 60.00f);
            INPUT_AND_SLIDER_FLOAT("Ball Blur Spread", &UmaGUiShowData::postEffectUpdateInfo_DOF.BallBlurSpread, -60.00f, 60.00f);
            ImGui::Checkbox("Point Ball Blur", &UmaGUiShowData::postEffectUpdateInfo_DOF.IsPointBallBlur);

            ImGui::InputFloat3("Focal Position (x, y, z)", &UmaGUiShowData::liveDOFForcalPosition.x);
        }
        ImGui::End();
    }

    using namespace UmaGUiShowData;

    void postFilmUpdateInfoMainLoop() {
        if (!showLiveWnd) return;

        for (int loopIndex = 0; loopIndex <= 2; loopIndex++) {
            const auto currendTitle = std::format("Live PostFilm ({})", loopIndex);
            if (ImGui::Begin(currendTitle.c_str())) {
                ImGui::Text(currendTitle.c_str());
                
                // livePostFilmFollowGame
                ImGui::Checkbox("Use Game PostFilm Settings", &livePostFilmFollowGame[loopIndex]);

                // filmMode
                static const char* filmModeNames[] = {
                    "None", "Lerp", "Add", "Mul",
                    "VignetteLerp", "VignetteAdd", "VignetteMul",
                    "Monochrome", "ScreenBlend", "VignetteScreenBlend"
                };
                int filmModeIndex = static_cast<int>(postFilmUpdateInfo[loopIndex].filmMode);
                ImGui::Combo("filmMode", &filmModeIndex, filmModeNames, IM_ARRAYSIZE(filmModeNames));
                postFilmUpdateInfo[loopIndex].filmMode = static_cast<PostFilmMode>(filmModeIndex);

                // colorType
                static const char* colorTypeNames[] = {
                    "ColorAll", "Color2TopBottom", "Color2LeftRight", "Color4"
                };
                int colorTypeIndex = static_cast<int>(postFilmUpdateInfo[loopIndex].colorType);
                ImGui::Combo("colorType", &colorTypeIndex, colorTypeNames, IM_ARRAYSIZE(colorTypeNames));
                postFilmUpdateInfo[loopIndex].colorType = static_cast<PostColorType>(colorTypeIndex);

                // filmPower
                INPUT_AND_SLIDER_FLOAT("filmPower", &postFilmUpdateInfo[loopIndex].filmPower, -10.0f, 10.0f);

                // depthPower
                INPUT_AND_SLIDER_FLOAT("depthPower", &postFilmUpdateInfo[loopIndex].depthPower, -10.0f, 10.0f);

                // DepthClip
                INPUT_AND_SLIDER_FLOAT("DepthClip", &postFilmUpdateInfo[loopIndex].DepthClip, -10.0f, 10.0f);

                // layerMode
                static const char* layerModeNames[] = { "Color", "UVMovie" };
                int layerModeIndex = static_cast<int>(postFilmUpdateInfo[loopIndex].layerMode);
                ImGui::Combo("layerMode", &layerModeIndex, layerModeNames, IM_ARRAYSIZE(layerModeNames));
                postFilmUpdateInfo[loopIndex].layerMode = static_cast<LayerMode>(layerModeIndex);

                // colorBlend
                static const char* colorBlendNames[] = { "None", "Lerp", "Additive", "Multiply" };
                int colorBlendIndex = static_cast<int>(postFilmUpdateInfo[loopIndex].colorBlend);
                ImGui::Combo("colorBlend", &colorBlendIndex, colorBlendNames, IM_ARRAYSIZE(colorBlendNames));
                postFilmUpdateInfo[loopIndex].colorBlend = static_cast<ColorBlend>(colorBlendIndex);

                // inverseVignette
                ImGui::Checkbox("inverseVignette", &postFilmUpdateInfo[loopIndex].inverseVignette);

                // colorBlendFactor
                INPUT_AND_SLIDER_FLOAT("colorBlendFactor", &postFilmUpdateInfo[loopIndex].colorBlendFactor, -10.0f, 10.0f);

                // movieResId
                ImGui::InputInt("movieResId", &postFilmUpdateInfo[loopIndex].movieResId);

                // movieFrameOffset
                ImGui::InputInt("movieFrameOffset", &postFilmUpdateInfo[loopIndex].movieFrameOffset);

                // movieTime
                INPUT_AND_SLIDER_FLOAT("movieTime", &postFilmUpdateInfo[loopIndex].movieTime, -10.0f, 10.0f);

                // movieReverse
                ImGui::Checkbox("movieReverse", &postFilmUpdateInfo[loopIndex].movieReverse);

                // RollAngle
                INPUT_AND_SLIDER_FLOAT("RollAngle", &postFilmUpdateInfo[loopIndex].RollAngle, -10.0f, 10.0f);

                // filmOffsetParam
                ImGui::InputFloat2("filmOffsetParam", &filmOffsetParam[loopIndex].x);

                // filmOptionParam
                ImGui::InputFloat4("filmOptionParam", &filmOptionParam[loopIndex].x);

                // filmcolor0
                ImGui::ColorEdit4("filmcolor0", &filmcolor0[loopIndex].r);

                // filmcolor1
                ImGui::ColorEdit4("filmcolor1", &filmcolor1[loopIndex].r);

                // filmcolor2
                ImGui::ColorEdit4("filmcolor2", &filmcolor2[loopIndex].r);

                // filmcolor3
                ImGui::ColorEdit4("filmcolor3", &filmcolor3[loopIndex].r);

                // FilmScale
                ImGui::InputFloat2("FilmScale", &FilmScale[loopIndex].x);

            }
            ImGui::End();
        }
    }

    void lightProjectionMainLoop() {
        if (!showLiveWnd) return;

        if (ImGui::Begin("Live Light Projection")) {

            ImGui::Checkbox("Use Game Light Projection Settings", &liveLightProjectionFollowGame);
            ImGui::Checkbox("isEnable", &lightProjectionUpdateInfo.isEnable);
            // TextureId
            ImGui::InputInt("TextureId", &lightProjectionUpdateInfo.TextureId);
            // colorPower
            INPUT_AND_SLIDER_FLOAT("colorPower", &lightProjectionUpdateInfo.colorPower, -10.0f, 10.0f);
            // orthographic
            ImGui::Checkbox("orthographic", &lightProjectionUpdateInfo.orthographic);
            // orthographicSize
            INPUT_AND_SLIDER_FLOAT("orthographicSize", &lightProjectionUpdateInfo.orthographicSize, -10.0f, 10.0f);
            // nearClipPlane
            INPUT_AND_SLIDER_FLOAT("nearClipPlane", &lightProjectionUpdateInfo.nearClipPlane, -10.0f, 10.0f);
            // farClipPlane
            INPUT_AND_SLIDER_FLOAT("farClipPlane", &lightProjectionUpdateInfo.farClipPlane, -10.0f, 10.0f);
            // fieldOfView
            INPUT_AND_SLIDER_FLOAT("fieldOfView", &lightProjectionUpdateInfo.fieldOfView, -10.0f, 10.0f);
            // nodeHash
            ImGui::InputInt("nodeHash", &lightProjectionUpdateInfo.nodeHash);
            // CharacterAttachPosition
            ImGui::InputInt("CharacterAttachPosition", &lightProjectionUpdateInfo.CharacterAttachPosition);
            // IsCharacterAttach
            ImGui::Checkbox("IsCharacterAttach", &lightProjectionUpdateInfo.IsCharacterAttach);
            // UseMonitorMovie
            ImGui::Checkbox("UseMonitorMovie", &lightProjectionUpdateInfo.UseMonitorMovie);
            // AnimationTextureId
            ImGui::InputInt("AnimationTextureId", &lightProjectionUpdateInfo.AnimationTextureId);
            // AnimationDivX
            ImGui::InputInt("AnimationDivX", &lightProjectionUpdateInfo.AnimationDivX);
            // AnimationDivY
            ImGui::InputInt("AnimationDivY", &lightProjectionUpdateInfo.AnimationDivY);
            // AnimationMaxCut
            ImGui::InputInt("AnimationMaxCut", &lightProjectionUpdateInfo.AnimationMaxCut);
            // AnimationTime
            INPUT_AND_SLIDER_FLOAT("AnimationTime", &lightProjectionUpdateInfo.AnimationTime, -10.0f, 10.0f);
            // ProgressTime
            INPUT_AND_SLIDER_FLOAT("ProgressTime", &lightProjectionUpdateInfo.ProgressTime, -10.0f, 10.0f);
            // position
            ImGui::InputFloat3("position (x,y,z)", &lightProjectionUpdateInfo.position.x);
            // rotation
            ImGui::InputFloat4("rotation (x,y,z,w)", &lightProjectionUpdateInfo.rotation.x);
            // scale
            ImGui::InputFloat3("scale (x,y,z)", &lightProjectionUpdateInfo.scale.x);
            // color
            ImGui::ColorEdit4("color", &lightProjectionUpdateInfo.color.r);
            // AnimationScaleUV
            ImGui::InputFloat2("AnimationScaleUV", &lightProjectionUpdateInfo.AnimationScaleUV.x);
            // AnimationOffsetUV
            ImGui::InputFloat2("AnimationOffsetUV", &lightProjectionUpdateInfo.AnimationOffsetUV.x);
        }
        ImGui::End();
    }

    void radialBlurUpdateInfoMainLoop() {
        if (!showLiveWnd) return;

        if (ImGui::Begin("Live Radial Blur")) {
            ImGui::Checkbox("Use Game Radial Blur Settings", &liveRadialBlurFollowGame);

            const char* moveBlurTypeNames[] = { "None", "Circle", "Horizontal", "Vertical", "Ellipse", "Roll" };
            int currentMoveBlurTypeIndex = static_cast<int>(radialBlurUpdateInfo.moveBlurType);
            ImGui::Combo("MoveBlurType", reinterpret_cast<int*>(&radialBlurUpdateInfo.moveBlurType), moveBlurTypeNames, IM_ARRAYSIZE(moveBlurTypeNames));

            ImGui::InputFloat2("RadialBlurOffset", &radialBlurUpdateInfo.radialBlurOffset.x);
            ImGui::InputInt("RadialBlurDownsample", &radialBlurUpdateInfo.radialBlurDownsample);
            INPUT_AND_SLIDER_FLOAT("RadialBlurStartArea", &radialBlurUpdateInfo.radialBlurStartArea, -10.0f, 10.0f);
            INPUT_AND_SLIDER_FLOAT("RadialBlurEndArea", &radialBlurUpdateInfo.radialBlurEndArea, -10.0f, 10.0f);
            INPUT_AND_SLIDER_FLOAT("RadialBlurPower", &radialBlurUpdateInfo.radialBlurPower, -10.0f, 10.0f);
            ImGui::InputInt("RadialBlurIteration", &radialBlurUpdateInfo.radialBlurIteration);
            ImGui::InputFloat2("RadialBlurEllipseDir", &radialBlurUpdateInfo.radialBlurEllipseDir.x);
            INPUT_AND_SLIDER_FLOAT("RadialBlurRollEulerAngles", &radialBlurUpdateInfo.radialBlurRollEulerAngles, -10.0f, 10.0f);
            ImGui::Checkbox("IsEnabledDepth", &radialBlurUpdateInfo.isEnabledDepth);
            INPUT_AND_SLIDER_FLOAT("DepthPowerFront", &radialBlurUpdateInfo.depthPowerFront, -10.0f, 10.0f);
            INPUT_AND_SLIDER_FLOAT("DepthPowerBack", &radialBlurUpdateInfo.depthPowerBack, -10.0f, 10.0f);
            ImGui::Checkbox("IsEnabledDepthCancelRect", &radialBlurUpdateInfo.isEnabledDepthCancelRect);
            ImGui::InputFloat4("DepthCancelRect", &radialBlurUpdateInfo.depthCancelRect.x);
            INPUT_AND_SLIDER_FLOAT("DepthCancelBlendLength", &radialBlurUpdateInfo.depthCancelBlendLength, -10.0f, 10.0f);
            ImGui::Checkbox("IsExpandDepthCancelRect", &radialBlurUpdateInfo.isExpandDepthCancelRect);
        }
        ImGui::End();
    }

    void charaFootLightMainLoop() {
       if (!showLiveWnd) return;

       if (ImGui::Begin("Chara Foot Light")) {

            ImGui::Columns(4, nullptr, false);

            for (int i = 0; i < 36; i++) {
                ImGui::PushID(i);

                ImGui::Text("Index %d", charaFootLightUpdateInfo[i].CharacterIndex);

                ImGui::Checkbox("Follow Game Settings", &charaFootLightUpdateInfo[i].followGame);
                INPUT_AND_SLIDER_FLOAT("hightMax", &charaFootLightUpdateInfo[i].hightMax, -10.0f, 10.0f);
                ImGui::ColorEdit4("lightColor", &charaFootLightUpdateInfo[i].lightColor.r);
                ImGui::PopID();
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
        }
       ImGui::End();
    }

    void globalLightMainLoop() {
        if (!showLiveWnd) return;

        static int selectedIndex = 0;
        static auto keyNames = liveCharaPositionFlag.GetNames_c();

        if (ImGui::Begin("Global Light Update Info")) {
            // Create a dropdown menu to select the key
            ImGui::Combo("Select Chara", &selectedIndex, keyNames, liveCharaPositionFlag.GetSize());
            const auto currentCharIndex = liveCharaPositionFlag.GetValue(selectedIndex).second;

            GlobalLightUpdateInfo& info = globalLightUpdateInfo.at(currentCharIndex);
            ImGui::Text("CharaFlag: 0x%x", info.flags);
            ImGui::Checkbox("Use Game Global Light Settings", &info.followGame);
            INPUT_AND_SLIDER_FLOAT("Global Rim Shadow Rate", &info.globalRimShadowRate, -10.0f, 10.0f);
            INPUT_AND_SLIDER_FLOAT("Rim Step", &info.rimStep, -10.0f, 10.0f);
            INPUT_AND_SLIDER_FLOAT("Rim Feather", &info.rimFeather, -10.0f, 10.0f);
            INPUT_AND_SLIDER_FLOAT("Rim Spec Rate", &info.rimSpecRate, -10.0f, 10.0f);
            INPUT_AND_SLIDER_FLOAT("Rim Horizon Offset", &info.RimHorizonOffset, -10.0f, 10.0f);
            INPUT_AND_SLIDER_FLOAT("Rim Vertical Offset", &info.RimVerticalOffset, -10.0f, 10.0f);
            INPUT_AND_SLIDER_FLOAT("Rim Horizon Offset 2", &info.RimHorizonOffset2, -10.0f, 10.0f);
            INPUT_AND_SLIDER_FLOAT("Rim Vertical Offset 2", &info.RimVerticalOffset2, -10.0f, 10.0f);
            INPUT_AND_SLIDER_FLOAT("Rim Step 2", &info.rimStep2, -10.0f, 10.0f);
            INPUT_AND_SLIDER_FLOAT("Rim Feather 2", &info.rimFeather2, -10.0f, 10.0f);
            INPUT_AND_SLIDER_FLOAT("Rim Spec Rate 2", &info.rimSpecRate2, -10.0f, 10.0f);
            INPUT_AND_SLIDER_FLOAT("Global Rim Shadow Rate 2", &info.globalRimShadowRate2, -10.0f, 10.0f);
            ImGui::ColorEdit3("Rim Color", &info.rimColor.r);
            ImGui::ColorEdit3("Rim Color 2", &info.rimColor2.r);
        }

        ImGui::End();
    }
    
    void othersMainLoop() {
        if (!showLiveWnd) return;

        if (ImGui::Begin("Live Other Settings")) {
            if (ImGui::CollapsingHeader("Exposure")) {
                ImGui::Checkbox("Use Game Exposure Settings", &liveExposureFollowGame);
                ImGui::Checkbox("IsEnable Exposure", &exposureUpdateInfo.IsEnable);
                ImGui::InputFloat4("ExposureParameter", &exposureUpdateInfo.ExposureParameter.x);
                INPUT_AND_SLIDER_FLOAT("DepthMask", &exposureUpdateInfo.DepthMask, -3.5f, 7.0f);
            }
            if (ImGui::CollapsingHeader("Vortex")) {
                ImGui::Checkbox("Use Game Vortex Settings", &liveVortexFollowGame);
                ImGui::Checkbox("IsEnable Vortex", &vortexUpdateInfo.IsEnable);
                INPUT_AND_SLIDER_FLOAT("RotVolume", &vortexUpdateInfo.RotVolume, -10.0f, 10.0f);
                INPUT_AND_SLIDER_FLOAT("DepthClip", &vortexUpdateInfo.DepthClip, -10.0f, 10.0f);
                ImGui::InputFloat4("Area", &vortexUpdateInfo.Area.x);
            }
        }
        ImGui::End();
    }

    void AllLoop() {
        static bool DOF = true;
        static bool others = true;
        static bool postFilm = false;
        static bool lightProjection = false;
        static bool radialBlur = false;
        static bool charaFootLight = false;
        static bool globalLight = false;

        if (!showLiveWnd) return;
        UmaGUiShowData::initGuiGlobalData();
        if (ImGui::Begin("Live Controller Pannel")) {
            ImGui::Checkbox("DOF", &DOF);
            ImGui::Checkbox("RadialBlur", &radialBlur);
            ImGui::Checkbox("Others", &others);
            ImGui::Checkbox("GlobalLight", &globalLight);
            ImGui::Checkbox("PostFilm", &postFilm);
            ImGui::Checkbox("LightProjection", &lightProjection);
            ImGui::Checkbox("CharaFootLight", &charaFootLight);
        }
        ImGui::End();

        if (DOF) postEffectUpdateInfo_DOF_guiLoop();
        if (postFilm) postFilmUpdateInfoMainLoop();
        if (lightProjection) lightProjectionMainLoop();
        if (radialBlur) radialBlurUpdateInfoMainLoop();
        if (charaFootLight) charaFootLightMainLoop();
        if (globalLight) globalLightMainLoop();
        if (others) othersMainLoop();
    }

}
