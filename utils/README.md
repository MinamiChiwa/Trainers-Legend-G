# 启用事件簿功能

- 将 `config.json` 内 `enableEventHelper` 设置为 `true` 即可



# 如何更新事件簿数据

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

