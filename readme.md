# 注意
- 此库复刻自github：https://github.com/GEEKiDoS/umamusume-localify
- 原作者@GEEKiDoS
- 由于长时间没有更新，现由@Minami_Chiwa自行判断接手

-  **_!由于本插件涉及到更改游戏文件，可能会有封号风险，本项目及其贡献者均与cygames无关并且不对使用该项目造成账号被ban负责，如果账号因使用此项目被ban请发issue!_** 
-  **本汉化插件完全免费，若您发现有倒卖此插件的店家请立刻举报。
根据我国《计算机软件保护条例》第十七条规定：“为了学习和研究软件内含的设计思想和原理，通过安装、显示、传输或者存储软件等方式使用软件的，可以不经软件著作权人许可，不向其支付报酬。”您需知晓本插件仅供用户交流学习与研究使用，用户本人下载后不能用作商业或非法用途，需在24小时之内删除，否则后果均由用户承担责任。** 
# 中文
## 赛马娘 (DMM版) 本地化补丁

### 使用方法:
1. 下载发行版`DMMUPD-Schinese`压缩包和`umamusume_update.exe`自动更新程序，将压缩包内的文件和`umamusume_update.exe`解压至游戏根目录`X:\Umamusume`
2. 启动游戏

或者

1. 将 `version.dll`（仓库version文件夹内） 和 `config.json`（仓库resources文件夹内） 以及 `config.json` 中引用的字典（仓库resources文件夹内的任意 **localized_data 文件夹**  <目前仅schinese处于更新状态>）放置于游戏根目录 `X:\Umamusume` 
2. 启动游戏
### 更新方法:
启动`umamusume_update.exe`（若没有请前往发行版下载此程序），或手动下载发行版最新压缩包。
### 设置选项:
- `enableConsole` 启用用来输出调试信息的控制台 (true/false)
- `enableLogger` 将未覆盖的文本输出到 `dump.txt` (true/false)
- `dumpStaticEntries` 需要上一条启用, 在游戏启动时将游戏内硬编码的文本条目输出到 `dump.txt`
- `maxFps` 帧率限制 (-1 = 游戏原版/0 = 无限/>0 = 锁定到特定帧数)
    - 注: 垂直同步已开启
- `unlockSize` 允许游戏使用`1080p`以上的分辨率 (true/false)
- `uiScale` 自定义UI缩放
- `replaceFont` 将所有字体替换为默认字体, 解决缺字问题 (true/false)
- `autoFullscreen` 在屏幕比例对应时自动设置为全屏 (true/false)
    - 如屏幕是16:9 并且横屏，将会在横屏播放剧情/live时全屏
    - 如屏幕是9:16，将会在绝大多数情况下全屏
- `dicts` 将会读取的字典列表 (相对/绝对路径)

### 已知问题
- ~~有时候加载 yaml 时会卡住，原因不明~~ 换成了rapidjson, 再见了您内
- maxFPS设为0时可能会在LIVE中出现眼球锁死的BUG
- maxFPS过大时，部分画面会出现强烈抖动（应该是原本的镜头抖动在高帧率下被加强了）
- `unlockSize`在部分分辨率下会出现界面显示不全or点击错位的bug，若出现请尝试将此项设为false
- 剧情文本跳字时可能会出现混乱
- 观看剧情时若启用了`AUTO`（自动播放）会导致剧情快速跳过
- 主线剧情由于训练员个人资料男女性设定不同会导致自称不同，目前还未翻译女性自称的文本，请暂时使用男性资料观看，我会在主线剧情翻译完之后补上女性自称的文本。
- `autoFullscreen`设为true时，部分用户会出现汉化失效的问题，此时请将汉化文件重新覆盖一遍即可。
- 文本错位问题请看下方`static.json`更新工具，或者等待仓库更新。
- 可能会出现内存报错无法启动游戏的情况，请尝试下载安装微软常用运行库。

### 如何贡献
 - 文本条目: 
    - 如要添加新条目，请先将`config.json` 内的 `enableLogger` 设置为 `true` 之后启动游戏
        - 随后未覆盖的文本会出现在 `dump.txt`
    - 部分原文本位于 `resources/original_data` 内，有需要翻译的原文本代码可以加入到相应的分类文件中。
    - 请在汉化完条目之后将其移动到`localized_data`内并提交 Pull Reqeust
    
 - 代码
    - 想必各位都是老司机了

### `static.json`更新工具
- 目前已知游戏在更新时有可能导致`static.json`文本序号改变，需要使用以下地址的软件进行更新
- github（原作者）：https://github.com/AirJerryWhite/i18upgrade
- gitee（搬运）：https://gitee.com/Minami_Chiwa/i18upgrade


# English
## Uma Musume: Pretty Derby localify patch

##Disclaimer: 
- **_This project(and its contributors) is not related with Cygames and will not take any responsbility of possible account bans because of using this project. USE IT AT YOUR RISK._** 
- **_If your account is banned because of using this project, please open an issue about that._** 

### Usage:
1. Put `version.dll`, `config.json` and the `dicts` referenced by `config.json` near the `umamusume.exe`.
2. Launch the game

### Config:
- `enableConsole` Enable the console for printing debug infomations (true/false)
- `enableLogger` Output uncovered text entries into `dump.txt` (true/false)
- `dumpStaticEntries` Requires ^, Dump hardcoded text entries into `dump.txt`
- `maxFps` Max fps limit (-1 = Unmodified/0 = Unlimited/>0 = Lock to #fps)
    - Note: VSync is enabled
- `unlockSize` Allow game to use resolutions that bigger than `1080p` (true/false)
- `uiScale` Custom UI scale
- `replaceFont` Replace all font to default font, solves missing word issue (true/false)
- `autoFullscreen` The game will be fullscreen if screen aspect ratio is correct (true/false)
    - If screen aspect ratio is 16:9, the game will be fullscreen when playing story/live with landscape.
    - If screen aspect ratio is 9:16, the game will be fullscreen in most case.
- `dicts` A list of dicts that read by this (Path)

### Known issue
- None

# Resources
- `static.json` upgrade tool 
- github（original）：https://github.com/AirJerryWhite/i18upgrade
- gitee：https://gitee.com/Minami_Chiwa/i18upgrade

# Credits
## Translation
### Original
#### schinese
- @AirJerryWhite
- @Yaklo
- @july20026
- @RikkaTakanaishi
- @羲羲
#### tchinese
- @yotv2000tw

### Now
#### schinese
- @MinamiChiwa
- @ManakaAo
- @晖轩
- @毫毛
- @uss-enterprise1071
#### 技术协力
- @chinosk114514
