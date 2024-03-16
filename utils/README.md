# 启用事件簿功能

- 将 `config.json` 内 `enableEventHelper` 设置为 `true` 即可
- 事件相关默认使用云端数据。若需要使用本地数据，请参考下方说明



# 使用本地数据

- 当插件存在 `localized_data/eventHelper` 目录，且该目录内存在和系统语言对应的 Json 文件时，将会启用本地数据。否则使用云端数据。

```javascript
events_scn.json  // 简体中文系统
events_tcn.json  // 繁體中文系統
events_jp.json  // 日本語システム
events_en.json  // The System of English and Other Languages
```



# 如何自行更新本地事件簿数据

- 事件簿数据位于 `resources/eventHelper` 文件夹内
- 当数据更新后，可以使用 [UmamusumeDeserializeDB5](https://github.com/UmamusumeResponseAnalyzer/UmamusumeDeserializeDB5) 工具生成新的 `events.br`，也可以直接 [下载](https://github.com/UmamusumeResponseAnalyzer/UmamusumeResponseAnalyzer/raw/master/GameData/events.br) 现成的
- 运行 `convert_db_data.py`

```shell
python convert_db_data.py
```

- 完成后可以在上述文件夹内找到新文件



# 数据来源/鸣谢

- [https://kamigame.jp/umamusume/page/152540608660042049.html](https://kamigame.jp/umamusume/page/152540608660042049.html)
- [UmamusumeDeserializeDB5](https://github.com/UmamusumeResponseAnalyzer/UmamusumeDeserializeDB5)

