<div align="center">

# Trainers' Legend G
> ウマ娘 プリティーダービー(DMM版)を中国語に翻訳するプラグインです。

[![ダウンロード](https://img.shields.io/github/v/release/MinamiChiwa/umamusume-localify-zh-CN?color=orange&logoColor=red&label=Download&logo=DocuSign)](https://github.com/MinamiChiwa/Trainers-Legend-G/releases/latest)
[![Discord](https://img.shields.io/discord/973208860217200653?color=orange&label=Discord&logo=Discord&logoColor=red)](https://discord.com/invite/TBCSv5hU69)

[中国語(簡体)](https://github.com/MinamiChiwa/Trainers-Legend-G) | [中国語(繁体)](https://github.com/yotv2000tw/Trainers-Legend-G-TRANS-zh-tw/) | [英語](https://github.com/MinamiChiwa/Trainers-Legend-G/blob/main/readme_EN.md) | 日本語

</div>


# 始めに

- このプラグインは、ウマ娘 プリティーダービー(DMM版)を中国や様々の国のトレーナー達により良いエクスペリエンスで提供します。[繁体字中国語](https://github.com/yotv2000tw/Trainers-Legend-G-TRANS-zh-tw)のフォークもあります。
- このプラグインは、@GEEKiDoS氏の[`umamusume-localify`](https://github.com/GEEKiDoS/umamusume-localify)からフォークをしています。

# お知らせ

- このプラグインはゲームファイルを変更するため、**Cygamesから警告またはBANを受ける可能性があります。**
- このリポジトリは、**中国語の翻訳のみを提供** しています。現在のところ、他の言語に翻訳する予定はありません。
- ですが、**プラグインは世界共通です。** ウマ娘を他の言語に翻訳したい意欲があるならば、あなた自身で翻訳のリポジトリを開設してください。翻訳ファイルのフォーマットやプラグインの使い方など技術的な問題があれば、私たちの[Discord](https://discord.com/invite/TBCSv5hU69)に参加して開発者に連絡をしてください。 

- **このプラグインのソースは、WTFPLのライセンスに基づいています。プラグインのソースコードに対するプルリクエストは歓迎します。**

# 使い方

1. `release`からzipファイルをダウンロードをして、ウマ娘のルートに展開をしたファイルを配置します。(`X:\Umamusume`のようにフォルダーが作成されています。フォルダー内には`umamusume.exe`が含まれている事が必須です)

2. ゲームを普通に起動で素敵なトレーナーライフを楽しみましょう。


# プラグインの設定(`config.json`)
```javascript
{
    "enableConsole": true, /*CMDコンソールを表示 (true/false)*/
    "enableLogger": false, /*未翻訳のテキストを`dump.txt`でダンプ (true/false)*/
    "dumpStaticEntries": false, /*未翻訳のハードコアテキストを`dump.txt`でダンプ、'enableLogger'を有効化する必要があります*/
    "maxFps": 60, /*FPS設定 (-1 = 既定のFPS|0 = 無制限|>0 = ウマ娘を特定のFPSに制限)——垂直同期が有効化されています*/
    "unlockSize": true, /*`1080p`より上の解像度の使用を許可する (true/false)*/
    "uiScale": 1.0, /*UIの大きさをカスタマイズします。(ウマ娘のウィンドウにすべてのグラフィックが表示されない場合は、0.8を設定)*/
    "readRequestPack": false, /*ゲームから送信されたメッセージパックを読み込み。有効化している場合は、コンソールで'reboot'と入力でウマ娘をより高速に再起動できます (true/false)*/
    "extraAssetBundlePath": "localized_data/umamusumelocalify", /*ウマ娘の元のアセットファイルを置換するために使用されるアセットバンドルの場所です。(よくわからない場合は変更しないでください)*/
    "replaceFont": true, /*ウマ娘のフォントに含まれていない中国語の文字を表示するためにシステムのフォントを使用します。 (true/false)*/
    "customFontPath": "assets/bundledassets/umamusumelocalify/fonts/MSYH.TTC", /*フォントのアセットバンドルの場所 (extraAssetBundlePath内)*/
    "customFontSizeOffset": -4, /*フォントのサイズ*/
    "customFontStyle": 1, /*フォントのスタイル (存在する場合)、0-標準、1-太字、2-斜体、 3-太字斜体*/
    "customFontLinespacing": 0.9, /*テキストの行間隔、既定は 1.0*/
    "replaceAssets": true, /*画像やテクスチャをextraAssetBundlePath内のリソースに置換(存在する場合) (true/false)*/
    "assetLoadLog": false, /*使用しているリソースのアセットバンドルパスを出力します (true/false)*/
    "autoFullscreen": false, /*自動的にフルスクリーンで表示します (true/false)
                            - ディスプレイの解像度が16:9の場合、ライブとストーリーの視聴時にフルスクリーンで表示されます
                            - ディスプレイの解像度が9:16の場合、ほとんどの状況でフルスクリーンで表示されます*/
    "autoChangeLineBreakMode": true, /*16:9か9:16かに基づいて"Ignore line break"モードを自動的に切り替えます。*/
    "resolution_start": [-1, -1], /*起動時のウィンドウサイズを設定します (不要な場合は [-1, -1] を使用してください)*/
    "aspect_ratio_new": {
        "w": 16.0,
        "h": 9.0
    }, /*アスペクト比を設定します。「w」は長辺、「h」は短辺です。*/
    "httpServerPort": 4321,  /*HTTPサーバーのポート番号*/
    "externalPlugin": {  /*external pluginを設定します。プラグインが未設定の場合は、この設定項目を削除してください。*/
        "hotkey": "u", /*ホットキー、Ctrl + `設定したキー`を押して開きます。*/
        "path": "main.exe", /*プラグインのパス*/
        "openExternalPluginOnLoad": false /*ロード時にexternal pluginを開きます。*/
    },
    "dicts": [ /*翻訳ファイルのパス (絶対/相対パス)*/
        "localized_data/event.json",
        "localized_data/LIVE.json",
        "localized_data/hash_entries.json"
    ], 
    "static_dict": "localized_data/static.json", /*翻訳された静的テキストのパス*/
    "no_static_dict_cache": true, /*静的辞書を使用したキャッシュを無効化します(true/false)*/
    "stories_path": "localized_data/stories", /*翻訳されたストーリーのテキストのパス*/
    "text_data_dict": "localized_data/text_data.json", /*翻訳されたテキストデータのパス*/
    "character_system_text_dict": "localized_data/character_system_text.json", /*翻訳されたキャラクターシステムテキストのパス*/
    "race_jikkyo_comment_dict": "localized_data/race_jikkyo_comment.json", /*翻訳されたレースと実況のコメントのパス*/
    "race_jikkyo_message_dict": "localized_data/race_jikkyo_message.json", /*翻訳されたレースと実況のメッセージのパス*/
    "autoUpdate": {
        "source": "github",
        "path": "http://uma.chinosk6.cn/api/get_localify_latest_releases" /*自動アップデートのURL*/
    },
    "enableBuiltinAutoUpdate": false, /*内蔵の自動アップデート機能を有効化します*/
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
    "loadDll": []  /*他のDLLプラグインを読み込む。 (string配列パス)*/
}
```



# フリーカメラ

----

## ライブのフリーカメラ

- FOVの調整: `Q`/`E`、カメラのリセット:`R`、モード変更: `F`



### フリーカメラモード (既定)

- 移動: `WSAD`、下: `Ctrl`、上: `Space` 
- アングルの調整: `↑↓←→` または マウスの右ボタンを押したまま



### フォローウマ娘モード

- **注意: このプラグインは一部のライブ時のキャラクターの配置をフックできません。 `external plugin - その他 - ライブのすべてのキャラクターの配置をフック`を使用する必要があります。それであればライブのファイルが変更されます。**

 - 距離の引き出しと引き込み:`W`/`S` 
 - 配置のロックを切り替え: `↑`/`↓` 
 - キャラクターのロックを切り替え:`←`/`→` 
 - アングルを調整:
   - 注意: `フォローウマ娘モード`の場合、アングルの調整は座標軸を直接的に加減をしてアングルの角度に応じた方向の調整は行ないません。
   - マウスの使用時: マウスの右ボタンを押したまま (**`カメラ位置`の調整のみ**)
   - キーボードの使用時:
     - `I`/`K`/`J`/`L` (**`カメラ位置`の調整のみ**)
     -  `A`/`D`/`Ctrl`/`Space`. (**`カメラ位置` と `カメラの視点位置`の調整が可能**)



### 一人称視点モード

- カメラのオフセットを変更: `W`/`S`/`Ctrl`/`Space`
- キャラクターロールの有無の変更: `V`



## レースのフリーカメラ

- 下: `Ctrl`, 上: `Space`、FOVの調整: `Q`/`E`、カメラのリセット:`R`、モード変更: `F`

- フォローウマ娘モード
  - オフセットの変更: `WSAD`、距離の調整: `↑↓`、観察するターゲットの切り替え: `←→`、正面または背面表示、一人称視点の切り替え: `V`
    - 一人称視点モード:
      - カメラのオフセットを変更: `W`/`S`/`Ctrl`/`Space`
      - 一人称視点モードには2種類のモードがあります。`Enable Roll`はネイティブのカメラを使用します。`Disable Roll`モードは、スタビライズドカメラを使用します。`Disable Roll`モードはマウスホイールをスクロールで振幅を調整できます。
  
- フリーカメラモード
  - 移動: `WSAD`、アングルの調整: `↑↓←→` または マウスの右ボタンを押したまま
  


# 既知の問題

クラッシュをした場合に、`%UserProfile%\AppData\LocalLow\Cygames\umamusume`内に`Player.log`が生成されます。バグや問題があった場合は、このログファイルを開発者に送ってください。

- 'maxFPS'が0の場合、ウマ娘の眉がライブでロックされる事があります。
- 'maxFPS'が60より大きい場合、カメラの揺れが激しくなる事があります。
- ウィンドウ内にすべてのグラフィックを表示できない場合は`uiScale`を0.8に設定してください。
- 'maxFPS'が60より大きい場合、ストーリー表示時の`AUTO`が壊れます。
- ウマ娘の起動時に`メモリが "read" になることはできませんでした`などのエラーが発生する場合は、MSランタイムライブラリをインストールしてみてください。
- `Assertion failed`が発生する: `config.json`が更新されているか、JSONの構文エラーがないかを確認してください。
- v0.89.5以降でテキストの置換が正しく動作しない。

# 貢献者（GitHub）

<a href="https://github.com/MinamiChiwa/Trainers-Legend-G/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=MinamiChiwa/Trainers-Legend-G" />
</a>
