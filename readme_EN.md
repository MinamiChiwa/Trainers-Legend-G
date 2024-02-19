<div align="center">

# Trainers' Legend G
> A plugin for translating Umamusume Pretty Derby (DMM Client) to Chinese.

[![Download](https://img.shields.io/github/v/release/MinamiChiwa/umamusume-localify-zh-CN?color=orange&logoColor=red&label=Download&logo=DocuSign)](https://github.com/MinamiChiwa/Trainers-Legend-G/releases/latest)
[![Discord](https://img.shields.io/discord/973208860217200653?color=orange&label=Discord&logo=Discord&logoColor=red)](https://discord.com/invite/TBCSv5hU69)

[Simplified Chinese](https://github.com/MinamiChiwa/Trainers-Legend-G) | [Traditional Chinese](https://github.com/yotv2000tw/Trainers-Legend-G-TRANS-zh-tw/) | English | [日本語](https://github.com/MinamiChiwa/Trainers-Legend-G/blob/main/readme_JA.md)

</div>


# Introduction

- This plugin can give Umamusume Pretty Derby (DMM Client) players from Mainland China a better experience. There's also a folk of [Traditional Chinese version](https://github.com/yotv2000tw/Trainers-Legend-G-TRANS-zh-tw) .
- The plugin is forked from [`umamusume-localify`](https://github.com/GEEKiDoS/umamusume-localify) by @GEEKiDoS.


# Notice

- The plugin will modify the game file and **chances are that you may be warned or banned by Cygames because of using the plugin** , though we haven't met any players who use it get restricted.
- This repo **ONLY** provides Chinese translation. We currently have no plans for translating the game into other languages.
- However, the plugin itself is **universal**. If you have any motivation to translate the game into other languages, we fully encourage you to set up your own translation repo. If you have any technical problem such as translation files format or how to use the plugin, you can join our [Discord](https://discord.com/invite/TBCSv5hU69) and contact to our developers.  

- **The plugin source code follows WTFPL license. Any pull request to the plugin source code is welcomed.**

# Usage

1. Download the zip file from `release` and unzip it at the root of the game (folder such as `X:\Umamusume` , notice that the folder should contains `umamusume.exe`)
2. Launch the game normally, enjoy.


# Plugin Setting(`config.json`)
```javascript
{
    "enableConsole": true, /*Show the cmd console (true/false)*/
    "enableLogger": false, /*Dump the untranslated text to `dump.txt` (true/false)*/
    "dumpStaticEntries": false, /*Dump the untranslated hardcored text to `dump.txt`, require enabling 'enableLogger'*/
    "maxFps": 60, /*Fps setting (-1 = Default fps|0 = Infinity|>0 = Limit game to specific fps)——Vertical synchronization is enabled*/
    "unlockSize": true, /*Allow to use the resolution higher than `1080p` (true/false)*/
    "uiScale": 1.0, /*Customize the scale of ui (set 0.8 if you find your game windows can't show all the graphic)*/
    "readRequestPack": false, /*Read the msgpack sent by game, you can reboot the game quicker by typing 'reboot' in the console if you enable it (true/false)*/
    "extraAssetBundlePath": "localized_data/umamusumelocalify", /*The location of asset bundle that is used to replace the original asset file of the game (Do not change it if you don't understand what it is)*/
    "replaceFont": true, /*Use system default font to show the Chinese characters that is not contained in the orignal game font (true/false)*/
    "customFontPath": "assets/bundledassets/umamusumelocalify/fonts/MSYH.TTC", /*the font's asset bundle location (in extraAssetBundlePath)*/
    "customFontSizeOffset": -4, /*Size of font*/
    "customFontStyle": 1, /*Style of font (if exist), 0-Normal, 1-Bold, 2-Italic, 3-Bold italic*/
    "customFontLinespacing": 0.9, /*Text line spacing, Default 1.0*/
    "replaceAssets": true, /*Replacing the game picture/texture with resource in extraAssetBundlePath if exist (true/false)*/
    "assetLoadLog": false, /*Print the asset bundle path of game resource that the game is using (true/false)*/
    "autoFullscreen": false, /*Full screen the game automaticlly (true/false)
                            - If monitor resolution is 16:9, game will be full screened during watching live/story
                            - If monitor resolution is 9:16, game will be full screened in most circumstances*/
    "autoChangeLineBreakMode": true, /*Automatically toggle "Ignore line break" mode based on whether the game is 16:9 or 9:16*/
    "resolution_start": [-1, -1], /*set the game windows size when booting the game (Use [-1, -1] if you don't need it)*/
    "aspect_ratio_new": {
        "w": 16.0,
        "h": 9.0
    }, /*Set aspect ratio. w is the longer side, h is the shorter side.*/
    "httpServerPort": 4321,  /*HTTP Server Port*/
    "externalPlugin": {  /*Set external plugin. If no plug-ins are configured, delete this configuration item*/
        "hotkey": "u", /*Hotkey, press ctrl + `the key you set` to open it*/
        "path": "main.exe", /*plugin path*/
        "openExternalPluginOnLoad": false /*Open external plugin on load.*/
    },
    "dicts": [ /*The translation files' path (Absolute/Relative path)*/
        "localized_data/event.json",
        "localized_data/LIVE.json",
        "localized_data/hash_entries.json"
    ], 
    "static_dict": "localized_data/static.json", /* The path of translated static text*/
    "no_static_dict_cache": true, /*Disable cache using of static dict(true/false)*/
    "stories_path": "localized_data/stories", /*The path of translated story text*/
    "text_data_dict": "localized_data/text_data.json", /*The path of translated text data*/
    "character_system_text_dict": "localized_data/character_system_text.json", /*The path of translated character system text*/
    "race_jikkyo_comment_dict": "localized_data/race_jikkyo_comment.json", /*The path of translated race jikkyo comment*/
    "race_jikkyo_message_dict": "localized_data/race_jikkyo_message.json", /*The path of translated race jikkyo message*/
    "autoUpdate": {
        "source": "github",
        "path": "http://uma.chinosk6.cn/api/get_localify_latest_releases" /*auto updating url */
    },
    "enableBuiltinAutoUpdate": false, /*enable built-in auto update*/
    "live": {
        "free_camera": false,
        "force_changeVisibility_false": false,
        "moveStep": 0.1,
        "close_all_blur": false
    },
    "race_camera": {
        "free_camera": false,
        "moveStep": 5.0,
        "defaultFOV": 40.0,
        "freecam_lookat_target": false,
        "freecam_follow_target": false,
        "follow_offset": {
            "distance": 4.0,
            "x": 0.0,
            "y": 1.0,
            "z": 0.0
        }
    },
    "replaceHomeStandChar": {
        "enable": false,
        "data": [
            {
                "origCharId": 1046,
                "newChrId": 2001,
                "newClothId": 9
            }
        ]
    },
    "replaceGlobalChar": {
        "enable": false,
        "data": [
            {
                "origCharId": 1046,
                "newChrId": 1030,
                "newClothId": 103001,
                "replaceMini": false
            }
        ]
    },
    "loadDll": []  /*Load other dll plugins. (path string array)*/
}
```



# Free Camera

----

## Live Free Camera

- Adjust FOV: `Q`/`E`, Reset Camera:`R`, Change mode: `F`



### Free Camera Mode (Default)

- Move: `WSAD`, Down: `Ctrl`, Up: `Space`, 
- Adjust Angle: `↑↓←→` or hold down the right mouse.



### Follow Umamusume Mode

- **Note: The plugin cannot hook some LIVE character locations. You need to click `external plugin - More - Make Live All Character Position Hookable`. This will modify the LIVE files.**

 - Pulling out/pulling in distance:`W`/`S` 
 - Switch Lock Position: `↑`/`↓` 
 - Switch Lock Character`←`/`→` 
 - Adjust Angle:
   - Note: In the case of `Follow Umamusume Mode`, adjusting the viewing angle directly adds or subtracts the coordinate axis and does not adapt the direction according to the line of sight angle.
   - Use Mouse: Hold down the right mouse. (**Adjust Only `CameraPosition`**)
   - Use Keyboard:
     - `I`/`K`/`J`/`L` (**Adjust Only `CameraPosition`**)
     -  `A`/`D`/`CTRL`/`SPACE`. (**Adjust `CameraPosition` and `CameraLookAt Position`**)



### First Person Mode

- Change camera offset: `W`/`S`/`ctrl`/`Space`
- Change with/without character roll: `V`



## Race Free Camera

- Down: `Ctrl`, Up: `Space`, Adjust FOV: `Q`/`E`, Reset Camera:`R`, Change mode: `F`

- Follow Umamusume Mode
  - Change Offset: `WSAD`, Adjust Distance: `↑↓`, Switching observation targets: `←→`, Switching front / back views or first persion: `V`
    - First Person Mode:
      - Change camera offset: `W`/`S`/`ctrl`/`Space`
      - First person mode has two mode. `Enable Roll` uses the native camera, while `Disable Roll` uses a stabilized camera. In `Disable Roll` mode, scrolling the mouse wheel adjusts the amplitude.
  
- Free Camera Mode
  - Move: `WSAD`, Adjust Angle: `↑↓←→` or hold down right mouse button.
  


# Already known issues

Game will create `Player.log` at `%UserProfile%\AppData\LocalLow\Cygames\umamusume` if game crash, which contains the details of it. Report the log file to developers if you met any bugs or problems.

- The characters' eyebow may be locked in live when 'maxFPS' is 0.
- Too intensive shaking of camera when 'maxFPS' is bigger than 60.
- If you find your game windows can't show all the game graphic, set `uiScale` to 0.8.
- The `AUTO` mode when viewing the story will broken if 'maxFPS' is bigger than 60.
- Chances are that the game will raise errors such as `Memory could not be read` when launching the game, please try to install MS runtime library.
- Raise `Assertion failed`: Check whether `config.json` is updated or is there any json syntax error in it.
- Character replace doesn't work properly since v0.89.5.

# Contributors（GitHub）

<a href="https://github.com/MinamiChiwa/Trainers-Legend-G/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=MinamiChiwa/Trainers-Legend-G" />
</a>
