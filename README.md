<div align="center">

# MudShip Gamepad Input

Xbox コントローラーを最大 32 台接続・バックグラウンド動作を可能にする入力方式を提供する Unity パッケージ。

</div>

- 接続台数は最大 32 台
- Unity 非フォーカス時も入力取得
- 個体識別ユニーク ID

---

## インストール

```
https://github.com/MudShipProject/MudShip-Gamepad-Input.git
```

---

## API

すべて `MS_Gamepad`（static クラス）のメンバー。初回呼び出しで自動初期化される。`deviceId` は `GetDeviceId` / `GetDeviceIds` が返すコントローラ固有の ID（同一 PC では差し直し・別ポートでも不変）で、スロット番号と違い物理個体を固定指定できる。

| 関数名 | 引数 | オーバーロード | 戻り値 | 説明 |
|---|---|---|---|---|
| `GetState` | `int index, GamepadInput input` | `string deviceId` | `float` | 入力の生値。軸（LStick/RStick の X/Y）は -1〜1、トリガー（LT/RT）は 0〜1、ボタンは押下 1・非押下 0。未接続/範囲外は 0。 |
| `GetButton` | `int index, GamepadInput input` | `string deviceId` | `bool` | 押されている間 true（軸を渡すと値が 0 以外で true）。 |
| `GetButtonDown` | `int index, GamepadInput input` | `string deviceId` | `bool` | 押した瞬間の 1 フレームだけ true。ボタン専用（軸は常に false）。 |
| `GetButtonUp` | `int index, GamepadInput input` | `string deviceId` | `bool` | 離した瞬間の 1 フレームだけ true。ボタン専用（軸は常に false）。 |
| `IsConnected` | `int index` | `string deviceId` | `bool` | 対象が接続中なら true。 |
| `ConnectedCount` | （なし・プロパティ） | なし | `int` | 現在接続中の台数。 |
| `GetDeviceId` | `int index` | なし | `string` | スロット index のユニーク ID（`APP_LOCAL_DEVICE_ID` 32 バイトの大文字 hex）。未接続は `""`。 |
| `GetDeviceIds` | （なし） | なし | `string[]` | 接続中の全ユニーク ID（スロット昇順、未接続は除外）。 |
| `GetDeviceName` | `int index` | なし | `string` | スロット index の表示名。Xbox パッドは空文字（機種により入る）。未接続は `""`。 |
| `GetIndexById` | `string deviceId` | なし | `int` | 保存した ID から現在のスロット番号を取得。該当なしは -1。 |
| `VibrateController` | `int index, float strength, float seconds` | `string deviceId` | `void` | 対象を強さ `strength`（0〜1）で `seconds` 秒（実時間）振動させ自動停止。`strength`≤0 か `seconds`≤0 で即停止。**前面時のみ動作**。 |

定数 `MS_Gamepad.MaxPads`（`int` = 32）— スロット数の上限。`index` の有効範囲は `0`〜`MaxPads - 1`。

### GamepadInput

| 値 | 種別 | 説明 |
|---|---|---|
| `DPadUp` | ボタン | 十字キー上 |
| `DPadDown` | ボタン | 十字キー下 |
| `DPadLeft` | ボタン | 十字キー左 |
| `DPadRight` | ボタン | 十字キー右 |
| `A` | ボタン | A |
| `B` | ボタン | B |
| `X` | ボタン | X |
| `Y` | ボタン | Y |
| `LB` | ボタン | 左バンパー |
| `RB` | ボタン | 右バンパー |
| `Start` | ボタン | ≡（メニュー）ボタン |
| `Menu` | ボタン | ⧉（ビュー）ボタン |
| `LStickPress` | ボタン | 左スティック押し込み |
| `RStickPress` | ボタン | 右スティック押し込み |
| `LStickX` | 軸 | 左スティック X（-1〜1） |
| `LStickY` | 軸 | 左スティック Y（-1〜1） |
| `RStickX` | 軸 | 右スティック X（-1〜1） |
| `RStickY` | 軸 | 右スティック Y（-1〜1） |
| `LT` | 軸 | 左トリガー（0〜1） |
| `RT` | 軸 | 右トリガー（0〜1） |

---

## 構成・制約

```
Xbox パッド (GIP) → GameInput.dll → MS_GamepadBridge.dll → P/Invoke → MS_Gamepad (C#)
```

- 初回 API 呼び出し時に `Application.runInBackground = true` を自動設定（背面でポーリングを止めないため。Player Settings 側も ON 推奨）。
- **Guide（Xbox ロゴ）/ Share ボタンは取得不可**（OS 予約）。
- 台数の実上限は物理接続側:
  - Xbox Wireless Adapter = 1 個で最大 8 台
  - アダプタの複数挿しは Windows 10 でドライバ不具合報告あり（片方が Code 10）→ 8 台超は有線 USB 併用が現実的
  - Bluetooth は Xbox パッド 1 台のみ（Windows 制限）
- 対応: Windows x64（Editor / Standalone）。GameInput ランタイムは新しめの Windows なら inbox（無い環境向けに [GameInput redist](https://www.nuget.org/packages/Microsoft.GameInput) を同梱）。
