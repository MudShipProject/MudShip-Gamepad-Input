# MudShip Gamepad Input

**Xboxコントローラを多数台（ソフト上限32）、完全バックグラウンドで読む** Unity パッケージ（Windows x64 専用）。

XInput は4台まで・Windows.Gaming.Input はフォーカス必須、という両方の制限を
**GameInput API + ネイティブプラグイン**で回避している。

- ✅ **台数制限は実質なし**（API上の制限なし。ソフト上限 `MaxPads=32`、物理側の天井は後述）
- ✅ **完全バックグラウンド動作**（ウィンドウが非フォーカスでも入力を取得）
- ✅ 正規 Xbox コントローラ（GIP / Xbox Wireless Adapter 含む）
- ✅ トリガー独立アナログ（LT/RT 0〜1）、スティック ±1
- ✅ 静的APIのみ・セットアップ不要（初回呼び出しで自動初期化）

## インストール

`Packages/manifest.json` に追加（ローカル参照の例）:

```json
"com.yamamotoryo0212.gamepad-input": "file:../../../MudShip-Gamepad-Input"
```

または Package Manager → Add package from disk → この `package.json` を選択。

## 使い方

```csharp
// どこからでも呼ぶだけ（初期化・ポーリングは自動）
float rt   = MS_Gamepad.GetState(0, GamepadInput.RT);        // トリガー 0..1
float lx   = MS_Gamepad.GetState(0, GamepadInput.LStickX);   // スティック -1..1
bool  a    = MS_Gamepad.GetButton(3, GamepadInput.A);        // 押している間 true
bool  down = MS_Gamepad.GetButtonDown(0, GamepadInput.A);    // 押した瞬間の1フレームだけ true
bool  up   = MS_Gamepad.GetButtonUp(0, GamepadInput.A);      // 離した瞬間の1フレームだけ true
bool  on   = MS_Gamepad.IsConnected(7);
int   n    = MS_Gamepad.ConnectedCount;
```

※ `GetButtonDown/Up` はボタン専用（軸 `LStickX/Y` `LT/RT` 等を渡すと常に false）。
※ どのスクリプトの `Update` から呼んでも同一フレーム内は一貫したスナップショットを参照する
（フレームに1回だけポーリングする方式。Unity 標準 Input と同じ感覚で使える）。

`GamepadInput` enum: `DPadUp/Down/Left/Right, A, B, X, Y, LB, RB, Start(≡), Menu(⧉),
LStickPress, RStickPress, LStickX/Y, RStickX/Y, LT, RT`

### 動作確認

空の GameObject に `MS_GamepadProbe` を付けて Play。
Console に毎秒ハートビート（`[HB] ...`）と、入力変化のあったパッドの行が出る。
**背面テスト**: Play 中に別アプリをアクティブにしてパッドを操作し、ログが動き続ければOK。

## 仕組み・注意点

```
Xbox パッド (GIP) → GameInput.dll → MS_GamepadBridge.dll → P/Invoke → MS_Gamepad (C#)
                                    SetFocusPolicy(0x40) = 背面入力ON
```

- 初回 API 呼び出し時に `Application.runInBackground = true` を自動設定する
  （背面でポーリングループを止めないため。Player Settings の Run In Background も ON 推奨）
- **Guide（Xboxロゴ）/ Share ボタンは取れない**（OS予約）
- **台数の実際の天井は物理接続側**:
  - **Xbox Wireless Adapter = 1個で最大8台**（無線の本命）
  - アダプタの**複数挿しは Windows 10 でドライバ不具合報告あり**（片方が Code 10 で死ぬ）→ 8台超は**有線USB併用**が現実的
  - **Bluetooth は Windows の制限で Xbox パッド1台のみ**なので注意
- 対応: Windows x64（Editor / Standalone）。GameInput ランタイムは新しめの
  Windows なら inbox（無い環境は [GameInput redist](https://www.nuget.org/packages/Microsoft.GameInput) を同梱）

## ネイティブプラグインのビルド（C++を変更した時だけ）

ソースは `Source~/GameInputBridge/`。Visual Studio 2022 (C++) + Windows SDK 10.0.26100 以降で:

```bat
Source~\GameInputBridge\build.bat
```

→ `Runtime/Plugins/x86_64/MS_GamepadBridge.dll` に自動配置される。
詳細・ハマりどころは [Source~/GameInputBridge/README.md](Source~/GameInputBridge/README.md) を参照。

⚠ Unity 起動中は DLL がロックされるため、差し替えは「Unity終了後」か「起動直後の Play 前」に。
