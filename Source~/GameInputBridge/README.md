# MS_GamepadBridge — ネイティブブリッジ (GameInput)

8台の Xbox コントローラを**完全バックグラウンド**で読むためのネイティブ DLL のソース。
Unity 側 `MS_Gamepad`（Runtime/MS_Gamepad.cs）から P/Invoke で呼ばれる。

> ✅ 2026-06-12 実機検証済み: 非フォーカスプロセスでの30秒ポーリングで実入力449ヒット、
> Unity Editor 内でも背面入力の取得を確認。

## 仕組み
- `GameInputCreate` → **`SetFocusPolicy((GameInputFocusPolicy)0x40)`** ← 背面入力ONの鍵
  - `0x40` = 新GDKでの `GameInputEnableBackgroundInput`。
    SDK 10.0.26100 のヘッダにはこの名前付き定数が**無い**（古い列挙体）が、
    実ランタイム（v2系）はこの値を解釈する。名前付き定数が無いため数値キャストで指定。
- `RegisterDeviceCallback`（接続/切断、最大8スロット自前管理）
- `GetCurrentReading` → `GamepadFrame`（C# 側 struct と完全一致のレイアウト）
- 公開C関数: `GIB_Init` / `GIB_Poll` / `GIB_Shutdown`

## ビルド
必要: **VS2022 (C++ ワークロード)** + **Windows SDK 10.0.26100 以降**（GameInput.h/lib 同梱）

```bat
build.bat
```
成功すると `../../Runtime/Plugins/x86_64/MS_GamepadBridge.dll` に自動配置される。
（VSのバージョン/パスが異なる環境では build.bat 冒頭の vcvars64.bat パスを修正）

## ハマりどころ（実際に踏んだもの）
- **DLL名は `MS_GamepadBridge.dll` 固定。** Microsoft 純正 redist の同名
  `GameInputBridge.dll`（`C:\Program Files\Microsoft GameInput\x64\`）とローダ衝突するため。
- **`/utf-8` 必須**（日本語コメントが CP932 で誤読される）
- **`#define NOMINMAX` 必須**（windows.h の min マクロが std::min を壊す）
- **背面ポリシーのバージョン差**: ヘッダ(10.0.26100)は「背面デフォルトON」世代の列挙体だが、
  実ランタイムは v2（背面デフォルトOFF）。`0x40` を明示的に渡すこと。
- **Unity 起動中は DLL ロック**で上書き不可 → Unity を閉じるか「起動直後・Play前」に差し替え。
- Unity 側はループ停止対策として `Application.runInBackground = true` が必要
  （C#側 MS_Gamepad が自動設定。Player Settings でも ON 推奨）。

## 診断ツール
- `verify.bat` … DLL のエクスポート関数を確認（GIB_Init/Poll/Shutdown が出ればOK）
- `poll_test.ps1` … Unity を介さず、このコンソールプロセス（非フォーカス）で30秒
  ポーリングして背面入力が来るかを実測。Unity側で問題が出た時の切り分けに使う。
