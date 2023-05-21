#pragma once

extern bool showLiveWnd;

namespace LiveGUILoops {
    void postEffectUpdateInfo_DOF_guiLoop();
    void postFilmUpdateInfoMainLoop();
    void lightProjectionMainLoop();

    void AllLoop();
}
