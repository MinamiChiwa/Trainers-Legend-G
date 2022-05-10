# Trainer's Legend G
A plugin for translating Umamusume Pretty Derby (DMM Client) to Chinese.  
[![Discord](https://img.shields.io/discord/973208860217200653?color=orange&label=Discord&logo=Discord&logoColor=red)](https://discord.com/invite/TBCSv5hU69)
# Notice
- The plugin is forked from [`umamusume-localify`](https://github.com/GEEKiDoS/umamusume-localify) by @GEEKiDoS
- This repo **ONLY** provides Chinese translation. We currently have no plans for translating the game into other languages.
- However, the plugin itself is **universal**. If you have any motivation to make the translation of other languages, we fully encourage you to set up your own translation repo. If you have any technical problem such as translation files format or how to use the plugin, you can join our discord and contact to our developers.  
- **The plugin will modify the game file and chances are that you may be warned or banned by Cygames because of using the plugin, though we haven't met any players who use it get restricted.**
- **The plugin source code follows WTFPL license. Any pull request to the plugin source code is welcomed.**

### Usage

1. Download the zip file from `release` and unzip it at the root of the game (folder such as `X:\Umamusume` , notice that the folder should contains `umamusume.exe`)
2. Launch the game normally, enjoy.


### Plugin Setting(`config.json`)
```
{
    "enableConsole": true, /*Show the cmd console (true/false)*/
    "enableLogger": true, /*Dump the untranslated text to `dump.txt` (true/false)*/
    "dumpStaticEntries": true, /*Dump the untranslated hardcored text to `dump.txt`, require enabling 'enableLogger'*/
    "maxFps": 60, /*Fps setting (-1 = Default fps|0 = Infinity|>0 = Limit game to specific fps)——Vertical synchronization is enabled*/
    "unlockSize": true, /*Allow to use the resolution higher than `1080p` (true/false)*/
    "uiScale": 1.0, /*Customize the scale of ui (set 0.8 if you find your game windows can't show all the graphic)*/
    "readRequestPack": true, /*Read the msgpack sent by game, you can reboot the game quicker by typing 'reboot' in the console if you enable it (true/false)*/
    "extraAssetBundlePath": null, /*The location of asset bundle that is used to replace the original asset file of the game (Do not change it if you don't understand what it is)*/
    "replaceFont": true, /*Use system default font to show the Chinese characters that is not contained in the orignal game font (true/false)*/
    "customFontPath": null, /*the font's asset bundle location (in extraAssetBundlePath)*/
    "customFontSizeOffset": -4, /*Size of font*/
    "customFontStyle": 1, /*Style of font (if exist), 0-Normal, 1-Bold, 2-Italic, 3-Bold italic*/
    "customFontLinespacing": 1.05, /*Text line spacing, Default 1.0*/
    "replaceAssets": false, /*Replacing the game picture/texture with resource in extraAssetBundlePath if exist (true/false)*/
    "assetLoadLog": false, /*Print the asset bundle path of game resource that the game is using (true/false)*/
    "autoFullscreen": false, /*Full screen the game automaticlly (true/false)
                            - If monitor resolution is 16:9, game will be full screened during watching live/story
                            - If monitor resolution is 9:16, game will be full screened in most circumstances*/
    "LineBreakHotKey": "u", /*Set the hotkey of toggling "Ignore line break" mode. Press `Ctrl` + `the key you set` to toggle it.*/
    "autoChangeLineBreakMode": true, /*Automatically toggle "Ignore line break" mode based on whether the game is 16:9 or 9:16*/
    "resolution_start": [-1, -1], /*set the game windows size when booting the game (Use [-1, -1] if you don't need it)*/
    "aspect_ratio": [16.0, 9.0], /*set UI scale*/
    "dicts": [ /*The translation files' path (Absolute/Relative path)*/
        "localized_data/chara.json",
        "localized_data/event.json",
        "localized_data/race_name.json",
        "localized_data/race_message.json",
        "localized_data/Umaname.json",
        "localized_data/himitsumanga.json",
        "localized_data/item.json",
        "localized_data/LIVE.json",
        "localized_data/scardname.json",
        "localized_data/skillname.json",
        "localized_data/skillcontent.json",
        "localized_data/storytitle.json",
        "localized_data/designation.json",
        "localized_data/hash_entries.json"
    ], 
    "static_dict": "localized_data/static.json", /* The path of translated static text*/
    "no_static_dict_cache": true, /*Disable cache using of static dict(true/false)*/
    "stories_path": "localized_data/stories", /*The path of translated story text*/
    "autoUpdate": {
        "path": "" /*auto updating url */
    }
}
```

### Already known issues

Game will create `Player.log` at `C:\Users\（用户名）\AppData\LocalLow\Cygames\umamusume` if game crash, which contains the details of it. Report the log file to developers if you met any bugs or problems.

- The characters' eyebow may be locked in live when 'maxFPS' is 0
- Too intensive shaking of camera when 'maxFPS' is bigger than 60
- If you find your game windows can't show all the game graphic, set `uiScale` to 0.8.
- The `AUTO` mode when viewing the story will broken if 'maxFPS' is bigger than 60.
- When `autoFullscreen` is true, serverl texts may be left untranslated. Replace the translation file to solve the problem.
- Chances are that the game will raise errors such as `Memory could not be read` when launching the game, please try to install MS runtime library.
- Raise `Assertion failed`: Check whether `config.json` is updated or is there any json syntax error in it.
