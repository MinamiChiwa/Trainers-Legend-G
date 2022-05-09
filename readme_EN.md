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
Still not translated yet. Contact developers if you have any problems.
```
{
    "enableConsole": true, /*启用用来输出调试信息的控制台 (true/false)*/
    "enableLogger": true, /*将未覆盖的文本输出到 `dump.txt` (true/false)*/
    "dumpStaticEntries": true, /*需要上一条启用, 在游戏启动时将游戏内硬编码的文本条目输出到 `dump.txt`*/
    "maxFps": 60, /*帧率限制 (-1 = 游戏原版/0 = 无限/>0 = 锁定到特定帧数)——注: 垂直同步已开启*/
    "unlockSize": true, /*允许游戏使用`1080p`以上的分辨率 (true/false)*/
    "uiScale": 1.0, /*自定义UI缩放*/
    "readRequestPack": true, /*是否读取客户端发送包信息，开启此功能可在命令行中输入reboot快速重启游戏(true/false)*/
    "extraAssetBundlePath": null, /*图片等资源替换包的路径（需要用unity打包，一般情况下请不要改动此项）*/
    "replaceFont": true, /*替换字体，true:用系统默认字体或自定义字体替换；false：使用游戏原字体*/
    "customFontPath": null, /*extraAssetBundlePath项设置的资源包内的字体路径：自定义字体*/
    "customFontSizeOffset": -4, /*字体大小，-4即为减少4*/
    "customFontStyle": 1, /*字体风格，0为正常，1为粗体，2为斜体，3为粗斜体*/
    "customFontLinespacing": 1.05, /*文字行间距，基准值为1.0*/
    "replaceAssets": false, /*是否开启图片等资源替换(true/false)*/
    "assetLoadLog": false, /*是否在debug输出游戏资源调用情况(true/false)*/
    "autoFullscreen": false, /*在屏幕比例对应时自动设置为全屏 (true/false)
    - 如屏幕是16:9 并且横屏，将会在横屏播放剧情/live时全屏
    - 如屏幕是9:16，将会在绝大多数情况下全屏*/
    "LineBreakHotKey": "u", /*设置 "忽略换行符" 模式的热键。按下`Ctrl` + `设置的键`  即可切换。（若不需要可以直接删除此条命令）*/
    "autoChangeLineBreakMode": true, /*根据横竖屏模式自动切换"忽略换行符"模式*/
    "resolution_start": [-1,-1], /*设置启动时的分辨率, 格式: `array[w, h]`（[-1,-1]为默认）*/
    "aspect_ratio": [16.0,9.0], /*设置UI比例, 格式: `array[w, h]`（没有需求可以不填）*/
    "dicts": [ /*将会读取的字典列表` (相对/绝对路径)，可删除相应字典提取命令。例如删除"localized_data/Umaname.json",即可在游戏内显示角色名日文，不影响其他翻译*/
        "localized_data/chara.json", /*角色语音*/
        "localized_data/event.json", /*育成事件文本*/
        "localized_data/race_name.json", /*比赛名*/
        "localized_data/race_message.json", /*比赛实况・解说*/
        "localized_data/Umaname.json", /*角色名*/
        "localized_data/himitsumanga.json", /*漫画标题、小秘密*/
        "localized_data/item.json", /*物品及描述*/
        "localized_data/LIVE.json", /*LIVE歌词等*/
        "localized_data/scardname.json", /*支援卡名*/
        "localized_data/skillname.json", /*技能名*/
        "localized_data/skillcontent.json", /*技能描述*/
        "localized_data/storytitle.json", /*剧情故事标题*/
        "localized_data/designation.json", /*训练员称号及赛马娘称号(二つ名)*/
        "localized_data/hash_entries.json" /*杂项文本*/
    ],
    "static_dict": "localized_data/static.json", /*static字典文件的路径*/
    "no_static_dict_cache": true, /*是否禁用static字典文件缓存(true/false)*/
    "stories_path": "localized_data/stories", /*剧情文本文件的路径*/
    "autoUpdate": {
        "source": "github", /*自动更新的仓库源（gitee或github）*/
        "path": "http://uma.chinosk6.cn/api/get_localify_latest_releases" /*自动更新的抓取地址*/
    }
}
```

### Already known issues

出现崩溃后`C:\Users\（用户名）\AppData\LocalLow\Cygames\umamusume` 文件夹下会产生 `Player.log` 文件，里面包含崩溃的具体信息，如果有崩溃的用户能够在崩溃发生后立即上传此文件到仓库的`crash`文件夹内并注明报告使用的插件版本将会对调试有巨大帮助。

- 目前插件仅为部分汉化，很多文本仍未翻译。
- 各种网络连接错误：请检查您的加速器或者梯子，必要时联系相关客服，不要问此项目的贡献者。
- 如果按正常流程安装了插件，却发现所有文本都没汉化，或者仍然停留在旧版本汉化状态，那请检查你的电脑里是否装了两个赛马娘游戏。
- maxFPS设为0时可能会在LIVE中出现眼球锁死的BUG
- maxFPS过大时，部分画面会出现强烈抖动（应该是原本的镜头抖动在高帧率下被加强了）
- 特殊比例的显示器下，游戏窗口可能会显示不全，请将`config.json`文件中的`uiScale`值由1.0改为0.8。
- 观看剧情时若启用了`AUTO`（自动播放）会导致剧情快速跳过
- `autoFullscreen`设为true时，部分用户会出现汉化失效的问题，此时请将汉化文件重新覆盖一遍即可。
- 可能会出现内存报错（`内存不能为read`）无法启动游戏的情况，请尝试下载安装微软常用运行库。
- `Assertion failed`报错：请将`config.json`文件也一同更新，或检查插件内的所有json文件内是否出现语法错误。
