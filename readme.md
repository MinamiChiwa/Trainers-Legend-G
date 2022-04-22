[![License](https://mirrors.creativecommons.org/presskit/buttons/88x31/svg/by-nc-sa.svg)](https://creativecommons.org/licenses/by-nc-sa/4.0/deed.zh)

# 注意

- 此插件在[`umamusume-localify`](https://github.com/GEEKiDoS/umamusume-localify)基础上改善、扩展而成。
- 插件原作者@GEEKiDoS
- 原插件已实现的功能：帧数解锁、分辨率解锁、更换游戏内的文本、自动全屏、将游戏内字体更换为默认字体等。
- 此插件的汉化进度目前为 **部分汉化**
[翻译进度及比赛译名对照表](http://docs.qq.com/sheet/DYkFFZVJudWxTa1Vq)
- [![Download](https://img.shields.io/github/v/release/MinamiChiwa/umamusume-localify-zh-CN?color=orange&logoColor=orange&label=Download&logo=DocuSign)](https://github.com/MinamiChiwa/umamusume-localify-zh-CN/releases/latest)
- [![Chat](https://img.shields.io/badge/Join-QQ%E9%A2%91%E9%81%93-red?logo=tencent-qq&logoColor=red)](https://qun.qq.com/qqweb/qunpro/share?_wv=3&_wwv=128&inviteCode=1olqdK&from=246610&biz=ka)
密码：cygames 若无资格请挂梯子
- **_!由于本插件涉及到更改游戏文件，可能会有封号风险，本项目及其贡献者均与cygames无关并且不对使用该项目造成账号被ban负责，如果账号因使用此项目被ban请发issue!_**
- **此插件完全免费，若您发现有倒卖此插件的店家请立刻举报。
根据我国《计算机软件保护条例》第十七条规定：“为了学习和研究软件内含的设计思想和原理，通过安装、显示、传输或者存储软件等方式使用软件的，可以不经软件著作权人许可，不向其支付报酬。”您需知晓本插件仅供用户交流学习与研究使用，用户本人下载后不能用作商业或非法用途，需在24小时之内删除，否则后果均由用户承担责任。**

- 工程使用 WTFPL 协议开源，除了以下目录下的任何文件，这些文件使用 [署名-非商业性使用-相同方式共享 4.0 国际 (CC BY-NC-SA 4.0)](https://creativecommons.org/licenses/by-nc-sa/4.0/deed.zh) 协议开源，您仅能在符合此协议的规定条件下使用这些文件
  - resources/schinese

# 贡献者

## 翻译

### 目前的贡献者

#### 简体中文

- @MinamiChiwa（南千和）
- @晖轩
- @毫毛
- @Chihyuen
- @八意永恩
- @uss-enterprise1071
- @ManakaAo

## 技术协力

- @chinosk114514
- @akemimadoka

## 修图嵌字

- @KiraPoi
- @虚拟狗头人
- @狸

## 译文搬运授权

### 主线剧情

- @秋裤SE7EN（Bilibili）
- @凉月crella（Bilibili）@水银h2oag @ASD一ZERO @monsterwatari @シキ

### 角色语音

- @Linxesu（萌娘百科）

 **另外本插件内引用了较多[B站赛马娘中文WIKI](https://wiki.biligame.com/umamusume/%E9%A6%96%E9%A1%B5)和[萌娘百科](https://zh.moegirl.org.cn/%E8%B5%9B%E9%A9%AC%E5%A8%98_Pretty_Derby)的翻译内容，由于寻找所有翻译者较为困难，故没有特别列出。若相关翻译者认为此插件的引用内容侵犯了您的权利，请联系此仓库所有者@MinamiChiwa 或直接发布Issue。**

### 原贡献者

#### 简体中文

- @AirJerryWhite
- @Yaklo
- @july20026
- @RikkaTakanaishi
- @羲羲

#### 繁体中文

- @yotv2000tw

# 介绍

## 赛马娘 (DMM版) 本地化补丁

### 使用方法

1. 下载发行版`DMMUPD-Schinese`压缩包，将压缩包内的文件解压至游戏根目录`X:\Umamusume`( **请注意是包含`umamusume.exe`的文件夹** ）
2. 从DMM启动游戏

### 更新方法

可自动更新，当插件本体需要更新时需手动下载发行版最新压缩包。

### 设置选项（`config.json`）

```
{
    "enableConsole": true, /*启用用来输出调试信息的控制台 (true/false)*/
    "enableLogger": true, /*将未覆盖的文本输出到 `dump.txt` (true/false)*/
    "dumpStaticEntries": true, /*需要上一条启用, 在游戏启动时将游戏内硬编码的文本条目输出到 `dump.txt`*/
    "maxFps": 60, /*帧率限制 (-1 = 游戏原版/0 = 无限/>0 = 锁定到特定帧数)——注: 垂直同步已开启*/
    "unlockSize": true, /*允许游戏使用`1080p`以上的分辨率 (true/false)*/
    "uiScale": 1.0, /*自定义UI缩放*/
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
        "localized_data/race_name.json", /*比赛名*/
        "localized_data/race_message.json", /*比赛实况・解说*/
        "localized_data/race_comment.json", /*旧实况解说文件(简中版删掉即可)*/
        "localized_data/Umaname.json", /*角色名*/
        "localized_data/mainstory.json", /*主线剧情*/
        "localized_data/umastory.json", /*角色个人剧情*/
        "localized_data/extrastory.json", /*额外剧情*/
        "localized_data/himitsumanga.json", /*漫画标题、小秘密*/
        "localized_data/item.json", /*物品及描述*/
        "localized_data/LIVE.json", /*LIVE歌词等*/
        "localized_data/scardname.json", /*支援卡名*/
        "localized_data/skillname.json", /*技能名*/
        "localized_data/skillcontent.json", /*技能描述*/
        "localized_data/event.json", /*育成事件文本*/
        "localized_data/designation.json", /*训练员称号及赛马娘称号(二つ名)*/
        "localized_data/common.json", /*旧杂项文本(简中版可删掉)*/
        "localized_data/hash_entries.json" /*杂项文本*/
    ],
    "static_dict": "localized_data/static.json", /*static字典文件的路径*/
    "autoUpdate": {
        "source": "github", /*自动更新的仓库源（gitee或github）*/
        "path": "http://uma.chinosk6.cn/api/get_localify_latest_releases" /*自动更新的抓取地址*/
    }
}
```

### 已知问题

出现崩溃后`C:\Users\（用户名）\AppData\LocalLow\Cygames\umamusume` 文件夹下会产生 `Player.log` 文件，里面包含崩溃的具体信息，如果有崩溃的用户能够在崩溃发生后立即上传此文件到仓库的`crash`文件夹内并注明报告使用的插件版本将会对调试有巨大帮助。

- ~~有时候加载 yaml 时会卡住，原因不明~~ 换成了rapidjson, 再见了您内
- 目前插件仅为部分汉化，很多文本仍未翻译。
- 各种网络连接错误：请检查您的加速器或者梯子，必要时联系相关客服，不要问此项目的贡献者。
- 如果按正常流程安装了插件，却发现所有文本都没汉化，或者仍然停留在旧版本汉化状态，那请检查你的电脑里是否装了两个赛马娘游戏。
- maxFPS设为0时可能会在LIVE中出现眼球锁死的BUG
- maxFPS过大时，部分画面会出现强烈抖动（应该是原本的镜头抖动在高帧率下被加强了）
- 特殊比例的显示器下，游戏窗口可能会显示不全，请将`config.json`文件中的`resolution_start``aspect_ratio`两项设置根据您的显示器比例进行修改。
- 观看剧情时若启用了`AUTO`（自动播放）会导致剧情快速跳过
- `autoFullscreen`设为true时，部分用户会出现汉化失效的问题，此时请将汉化文件重新覆盖一遍即可。
- 可能会出现内存报错（`内存不能为read`）无法启动游戏的情况，请尝试下载安装微软常用运行库。
- `Assertion failed`报错：请将`config.json`文件也一同更新，或检查插件内的所有json文件内是否出现语法错误。

### 如何贡献【建议加频道，频道内可以提供更多汉化相关的工具】

- [烤肉教程](https://docs.qq.com/doc/DYk1Ia3h4UHRocGVJ)

- 代码
  - 想必各位都是老司机了

### ~~`static.json`更新工具~~【此问题已解决】

- ~~目前已知游戏在更新时有可能导致`static.json`文本序号改变，需要使用以下地址的软件进行更新~~
- github（原作者）：<https://github.com/AirJerryWhite/i18upgrade>
- gitee（搬运）：<https://gitee.com/Minami_Chiwa/i18upgrade>
