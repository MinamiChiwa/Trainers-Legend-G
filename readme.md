# 中文
## 赛马娘 (DMM版) 本地化补丁

### 使用方法:
1. 将 `version.dll` 和 `config.json` 以及 `config.json` 中引用的字典放置于 `umamusume.exe` 边上
2. 启动游戏

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

### 如何贡献
 - 文本条目: 
    - 位于 `resources/original_data` 内
    - 请在汉化完条目之后将其移动到`localized_data`内并提交 Pull Reqeust
    - 如要添加新条目，请先将`config.json` 内的 `enableLogger` 设置为 `true` 之后启动游戏
        - 随后未覆盖的文本会出现在 `dump.txt`
 - 代码
    - 想必各位都是老司机了

# English
## Uma Musume: Pretty Derby localify patch

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
