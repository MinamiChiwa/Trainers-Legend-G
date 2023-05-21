#include "imgui/imgui.h"
#include "stdinclude.hpp"

extern bool showLiveWnd;

namespace LiveGUILoops {
    void postEffectUpdateInfo_DOF_guiLoop() {
        if (!showLiveWnd) return;

        if (ImGui::Begin("Live DOF")) {
            ImGui::Checkbox("Use Game DOF Settings", &UmaGUiShowData::dofColtrollerFollowGame);
            ImGui::Checkbox("Enable DOF", &UmaGUiShowData::postEffectUpdateInfo_DOF.IsEnableDOF);

            static float inputFloatWidth = 70.0f;

            ImGui::SetNextItemWidth(inputFloatWidth);
            ImGui::InputFloat("##Focal Size Input", &UmaGUiShowData::postEffectUpdateInfo_DOF.forcalSize);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
            ImGui::SliderFloat("Focal Size", &UmaGUiShowData::postEffectUpdateInfo_DOF.forcalSize, -60.00f, 60.00f);
            ImGui::SetNextItemWidth(inputFloatWidth);
            ImGui::InputFloat("##Blur Spread Input", &UmaGUiShowData::postEffectUpdateInfo_DOF.blurSpread);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
            ImGui::SliderFloat("Blur Spread", &UmaGUiShowData::postEffectUpdateInfo_DOF.blurSpread, -60.00f, 60.00f);

            const char* dofBlurTypeItems[] = { "Horizon", "Mixed", "Disc", "BallBlur" };
            ImGui::Combo("DOF Blur Type", (int*)&UmaGUiShowData::postEffectUpdateInfo_DOF.dofBlurType, dofBlurTypeItems, 4);

            const char* dofQualityItems[] = { "Only Background", "Background and Foreground" };
            int dofQualityIndex = UmaGUiShowData::postEffectUpdateInfo_DOF.dofQuality == UmaGUiShowData::DofQuality::OnlyBackground ? 0 : 1;
            ImGui::Combo("DOF Quality", &dofQualityIndex, dofQualityItems, 2);
            UmaGUiShowData::postEffectUpdateInfo_DOF.dofQuality = (UmaGUiShowData::DofQuality)(dofQualityIndex == 0 ? UmaGUiShowData::DofQuality::OnlyBackground : UmaGUiShowData::DofQuality::BackgroundAndForeground);

            ImGui::SetNextItemWidth(inputFloatWidth);
            ImGui::InputFloat("##DOF Foreground Size Input", &UmaGUiShowData::postEffectUpdateInfo_DOF.dofForegroundSize);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
            ImGui::SliderFloat("DOF Foreground Size", &UmaGUiShowData::postEffectUpdateInfo_DOF.dofForegroundSize, -60.00f, 60.00f);
            ImGui::SetNextItemWidth(inputFloatWidth);
            ImGui::InputFloat("##DOF Focal Point Input", &UmaGUiShowData::postEffectUpdateInfo_DOF.dofFocalPoint);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
            ImGui::SliderFloat("DOF Focal Point", &UmaGUiShowData::postEffectUpdateInfo_DOF.dofFocalPoint, -60.00f, 60.00f);
            ImGui::SetNextItemWidth(inputFloatWidth);
            ImGui::InputFloat("##DOF Smoothness Input", &UmaGUiShowData::postEffectUpdateInfo_DOF.dofSoomthness);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
            ImGui::SliderFloat("DOF Smoothness", &UmaGUiShowData::postEffectUpdateInfo_DOF.dofSoomthness, -60.00f, 60.00f);
            ImGui::Checkbox("Use Focal Point", &UmaGUiShowData::postEffectUpdateInfo_DOF.isUseFocalPoint);
            ImGui::SetNextItemWidth(inputFloatWidth);
            ImGui::InputFloat("##Ball Blur Curve Factor Input", &UmaGUiShowData::postEffectUpdateInfo_DOF.BallBlurCurveFactor);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
            ImGui::SliderFloat("Ball Blur Curve Factor", &UmaGUiShowData::postEffectUpdateInfo_DOF.BallBlurCurveFactor, -60.00f, 60.00f);
            ImGui::SetNextItemWidth(inputFloatWidth);
            ImGui::InputFloat("##Ball Blur Brightness Threshhold Input", &UmaGUiShowData::postEffectUpdateInfo_DOF.BallBlurBrightnessThreshhold);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
            ImGui::SliderFloat("Ball Blur Brightness Threshhold", &UmaGUiShowData::postEffectUpdateInfo_DOF.BallBlurBrightnessThreshhold, -60.00f, 60.00f);
            ImGui::SetNextItemWidth(inputFloatWidth);
            ImGui::InputFloat("##Ball Blur Brightness Intensity Input", &UmaGUiShowData::postEffectUpdateInfo_DOF.BallBlurBrightnessIntensity);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
            ImGui::SliderFloat("Ball Blur Brightness Intensity", &UmaGUiShowData::postEffectUpdateInfo_DOF.BallBlurBrightnessIntensity, -60.00f, 60.00f);
            ImGui::SetNextItemWidth(inputFloatWidth);
            ImGui::InputFloat("##all Blur Spread", &UmaGUiShowData::postEffectUpdateInfo_DOF.BallBlurSpread);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
            ImGui::SliderFloat("Ball Blur Spread", &UmaGUiShowData::postEffectUpdateInfo_DOF.BallBlurSpread, -60.00f, 60.00f);
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

                static float inputFloatWidth = 70.0f;
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
                ImGui::SetNextItemWidth(inputFloatWidth);
                ImGui::InputFloat("##filmPower Input", &postFilmUpdateInfo[loopIndex].filmPower);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
                ImGui::SliderFloat("filmPower", &postFilmUpdateInfo[loopIndex].filmPower, -10.0f, 10.0f);

                // depthPower
                ImGui::SetNextItemWidth(inputFloatWidth);
                ImGui::InputFloat("##depthPower Input", &postFilmUpdateInfo[loopIndex].depthPower);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
                ImGui::SliderFloat("depthPower", &postFilmUpdateInfo[loopIndex].depthPower, -10.0f, 10.0f);

                // DepthClip
                ImGui::SetNextItemWidth(inputFloatWidth);
                ImGui::InputFloat("##DepthClip Input", &postFilmUpdateInfo[loopIndex].DepthClip);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
                ImGui::SliderFloat("DepthClip", &postFilmUpdateInfo[loopIndex].DepthClip, -10.0f, 10.0f);

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
                ImGui::SetNextItemWidth(inputFloatWidth);
                ImGui::InputFloat("##colorBlendFactor Input", &postFilmUpdateInfo[loopIndex].colorBlendFactor);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
                ImGui::SliderFloat("colorBlendFactor", &postFilmUpdateInfo[loopIndex].colorBlendFactor, -10.0f, 10.0f);

                // movieResId
                ImGui::InputInt("movieResId", &postFilmUpdateInfo[loopIndex].movieResId);

                // movieFrameOffset
                ImGui::InputInt("movieFrameOffset", &postFilmUpdateInfo[loopIndex].movieFrameOffset);

                // movieTime
                ImGui::SliderFloat("movieTime", &postFilmUpdateInfo[loopIndex].movieTime, -10.0f, 10.0f);

                // movieReverse
                ImGui::Checkbox("movieReverse", &postFilmUpdateInfo[loopIndex].movieReverse);

                // RollAngle
                ImGui::SliderFloat("RollAngle", &postFilmUpdateInfo[loopIndex].RollAngle, -10.0f, 10.0f);

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
            static float inputFloatWidth = 70.0f;

            ImGui::Checkbox("Use Game Light Projection Settings", &liveLightProjectionFollowGame);
            ImGui::Checkbox("isEnable", &lightProjectionUpdateInfo.isEnable);
            // TextureId
            ImGui::InputInt("TextureId", &lightProjectionUpdateInfo.TextureId);
            // colorPower
            ImGui::SetNextItemWidth(inputFloatWidth);
            ImGui::InputFloat("##colorPower", &lightProjectionUpdateInfo.colorPower);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
            ImGui::SliderFloat("colorPower", &lightProjectionUpdateInfo.colorPower, -10.0f, 10.0f);
            // orthographic
            ImGui::Checkbox("orthographic", &lightProjectionUpdateInfo.orthographic);
            // orthographicSize
            ImGui::SetNextItemWidth(inputFloatWidth);
            ImGui::InputFloat("##orthographicSize", &lightProjectionUpdateInfo.orthographicSize);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
            ImGui::SliderFloat("orthographicSize", &lightProjectionUpdateInfo.orthographicSize, -10.0f, 10.0f);
            // nearClipPlane
            ImGui::SetNextItemWidth(inputFloatWidth);
            ImGui::InputFloat("##nearClipPlane", &lightProjectionUpdateInfo.nearClipPlane);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
            ImGui::SliderFloat("nearClipPlane", &lightProjectionUpdateInfo.nearClipPlane, -10.0f, 10.0f);
            // farClipPlane
            ImGui::SetNextItemWidth(inputFloatWidth);
            ImGui::InputFloat("##farClipPlane", &lightProjectionUpdateInfo.farClipPlane);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
            ImGui::SliderFloat("farClipPlane", &lightProjectionUpdateInfo.farClipPlane, -10.0f, 10.0f);
            // fieldOfView
            ImGui::SetNextItemWidth(inputFloatWidth);
            ImGui::InputFloat("##fieldOfView", &lightProjectionUpdateInfo.fieldOfView);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
            ImGui::SliderFloat("fieldOfView", &lightProjectionUpdateInfo.fieldOfView, -10.0f, 10.0f);
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
            ImGui::SetNextItemWidth(inputFloatWidth);
            ImGui::InputFloat("##AnimationTime", &lightProjectionUpdateInfo.AnimationTime);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
            ImGui::SliderFloat("AnimationTime", &lightProjectionUpdateInfo.AnimationTime, -10.0f, 10.0f);
            // ProgressTime
            ImGui::SetNextItemWidth(inputFloatWidth);
            ImGui::InputFloat("##ProgressTime", &lightProjectionUpdateInfo.ProgressTime);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth);
            ImGui::SliderFloat("ProgressTime", &lightProjectionUpdateInfo.ProgressTime, -10.0f, 10.0f);
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


    void AllLoop() {
        postEffectUpdateInfo_DOF_guiLoop();
        postFilmUpdateInfoMainLoop();
        lightProjectionMainLoop();
    }

}
