# 中文
## 赛马娘 (DMM版) 本地化补丁

### 使用方法:
1. 将 `version.dll` 和 `localized_strings.yaml` 放置于 `umamusume.exe` 边上
2. 启动游戏

### 已知问题
- ~~有时候加载 yaml 时会卡住，原因不明~~ 换成了rapidjson, 再见了您内

### 如何贡献
 - 文本条目: 
    - 位于 `resources/localized_data/hash_entries.json` 以及 `static_entries.json`
    - 请在汉化完条目之后提交 Pull Reqeust
    - 如要添加新条目，请先将`config.json` 内的 `enableLogger` 设置为 `true` 之后启动游戏
        - 随后未覆盖的文本会出现在 `dump.txt`
 - 代码
    - 想必各位都是老司机了

# English
## Uma Musume: Pretty Derby localify patch
