// GameInputBridge.cpp
// Unity <-> GameInput ブリッジ: 最大8台のゲームパッドを「完全バックグラウンド」で読む。
//
// 鍵: GameInputCreate -> SetFocusPolicy(GameInputEnableBackgroundInput)
//     これで非フォーカス（背面）時も入力が届く。
//
// ビルド: x64 DLL。GameInput.h / GameInput.lib（Microsoft GDK もしくは GameInput SDK）が必要。
// 出力 GameInputBridge.dll を Unity の Assets/Plugins/x86_64/ に置く。
//
// 注意: GameInput はバージョンで名前空間（GameInput::v2 等）やシグネチャが変わることがある。
//       コンパイルが通らない場合は、使用中バージョンのサンプルに合わせて include / 呼び出しを微調整。

#define NOMINMAX            // windows.h の min/max マクロ抑制（std::min を守る）
#define WIN32_LEAN_AND_MEAN
#include <GameInput.h>
#include <wrl/client.h>
#include <mutex>
#include <cstdint>
#include <cstring>
#include <algorithm>

using Microsoft::WRL::ComPtr;

// ---- Unity と共有する平坦な構造体（C#側 GamepadFrame と完全一致させること）----
extern "C" struct GamepadFrame
{
	int          connected;   // 0/1
	unsigned int buttons;     // GameInputGamepadButtons のビットマスク
	float        lx, ly;      // 左スティック -1..1
	float        rx, ry;      // 右スティック -1..1
	float        lt, rt;      // トリガー 0..1
};

namespace
{
	// ソフト上限。GameInput 自体に台数制限は無いので余裕を持って確保
	// （実際の天井は物理側: Xbox Wireless Adapter=8台/個, 有線USBで追加可）。
	// C# 側 MS_Gamepad.MaxPads と一致させること。
	constexpr int kMaxPads = 32;

	ComPtr<IGameInput>       g_gameInput;
	GameInputCallbackToken   g_deviceToken = 0;
	std::mutex               g_mutex;
	ComPtr<IGameInputDevice> g_slots[kMaxPads];

	int FindSlot(IGameInputDevice* dev)
	{
		for (int i = 0; i < kMaxPads; ++i)
			if (g_slots[i].Get() == dev) return i;
		return -1;
	}

	// デバイスの接続/切断通知（GameInput のワーカースレッドから呼ばれる）
	void CALLBACK OnDevice(
		GameInputCallbackToken /*token*/,
		void* /*context*/,
		IGameInputDevice* device,
		uint64_t /*timestamp*/,
		GameInputDeviceStatus currentStatus,
		GameInputDeviceStatus /*previousStatus*/)
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		const bool connected = (currentStatus & GameInputDeviceConnected) != 0;

		if (connected)
		{
			if (FindSlot(device) < 0)
			{
				for (int i = 0; i < kMaxPads; ++i)
					if (!g_slots[i]) { g_slots[i] = device; break; } // ComPtr が AddRef
			}
		}
		else
		{
			const int i = FindSlot(device);
			if (i >= 0) g_slots[i].Reset();
		}
	}
}

extern "C" __declspec(dllexport) int GIB_Init()
{
	if (g_gameInput) return 1; // 二重初期化を防ぐ

	if (FAILED(GameInputCreate(&g_gameInput)))
		return 0;

	// ★ 背面入力を有効化。
	// インストール済み GameInput ランタイムは v2 系（背面デフォルトOFF）なので、
	// GameInputEnableBackgroundInput (=0x40) を渡して明示的にONにする。
	// この SDK(10.0.26100) のヘッダには 0x40 の名前付き定数が無いため数値で指定
	// （ランタイム側がこの値を解釈する。IGameInput の vtable 互換は実機テスト済み）。
	g_gameInput->SetFocusPolicy((GameInputFocusPolicy)0x40);

	const HRESULT hr = g_gameInput->RegisterDeviceCallback(
		nullptr,                       // 全デバイス対象
		GameInputKindGamepad,          // ゲームパッドのみ
		GameInputDeviceConnected,      // 接続状態の変化を通知
		GameInputBlockingEnumeration,  // 既存の接続済みも即列挙
		nullptr,
		OnDevice,
		&g_deviceToken);

	return SUCCEEDED(hr) ? 1 : 0;
}

extern "C" __declspec(dllexport) int GIB_Poll(GamepadFrame* out, int maxCount)
{
	if (!g_gameInput || !out) return 0;

	std::lock_guard<std::mutex> lock(g_mutex);
	const int n = std::min(maxCount, kMaxPads);

	for (int i = 0; i < n; ++i)
	{
		GamepadFrame& f = out[i];
		f = GamepadFrame{};

		IGameInputDevice* dev = g_slots[i].Get();
		if (!dev) continue;

		f.connected = 1;

		ComPtr<IGameInputReading> reading;
		if (SUCCEEDED(g_gameInput->GetCurrentReading(GameInputKindGamepad, dev, &reading)) && reading)
		{
			GameInputGamepadState s{};
			if (reading->GetGamepadState(&s))
			{
				f.buttons = static_cast<unsigned int>(s.buttons);
				f.lx = s.leftThumbstickX;  f.ly = s.leftThumbstickY;
				f.rx = s.rightThumbstickX; f.ry = s.rightThumbstickY;
				f.lt = s.leftTrigger;      f.rt = s.rightTrigger;
			}
		}
	}
	return n;
}

// デバイスのユニークID・VID/PID・表示名を取得（接続中なら 1 を返す）。
// id = APP_LOCAL_DEVICE_ID（32バイト, 同一PC上で差し直しても不変）、name = UTF-8。
extern "C" __declspec(dllexport) int GIB_GetDeviceInfo(
	int index,
	unsigned short* vendorId,
	unsigned short* productId,
	unsigned char* id,        // 32 バイトのバッファ
	char* name,
	int nameLen)
{
	if (vendorId)  *vendorId = 0;
	if (productId) *productId = 0;
	if (id) std::memset(id, 0, APP_LOCAL_DEVICE_ID_SIZE);
	if (name && nameLen > 0) name[0] = '\0';
	if (index < 0 || index >= kMaxPads) return 0;

	std::lock_guard<std::mutex> lock(g_mutex);
	IGameInputDevice* dev = g_slots[index].Get();
	if (!dev) return 0;

	const GameInputDeviceInfo* info = dev->GetDeviceInfo();
	if (info)
	{
		if (vendorId)  *vendorId = info->vendorId;
		if (productId) *productId = info->productId;
		if (id) std::memcpy(id, info->deviceId.value, APP_LOCAL_DEVICE_ID_SIZE);

		const GameInputString* dn = info->displayName;
		if (name && nameLen > 0 && dn && dn->data && dn->sizeInBytes > 0)
		{
			int n = static_cast<int>(dn->sizeInBytes);
			if (n > nameLen - 1) n = nameLen - 1;
			std::memcpy(name, dn->data, static_cast<size_t>(n));
			name[n] = '\0';
		}
	}
	return 1;
}

// 指定スロットのコントローラを振動させる（low/high = 各モーター強度 0..1）。
// 注意: GameInput 仕様上、アプリが前面の時だけ実際にモーターが回る（背面は無効）。
extern "C" __declspec(dllexport) void GIB_SetRumble(int index, float low, float high)
{
	if (index < 0 || index >= kMaxPads) return;
	std::lock_guard<std::mutex> lock(g_mutex);
	IGameInputDevice* dev = g_slots[index].Get();
	if (!dev) return;
	GameInputRumbleParams p{};
	p.lowFrequency  = low;
	p.highFrequency = high;
	p.leftTrigger   = 0.0f;
	p.rightTrigger  = 0.0f;
	dev->SetRumbleState(&p);
}

extern "C" __declspec(dllexport) void GIB_Shutdown()
{
	// 進行中コールバックの完了を待ってから解除（g_mutex は持たない＝デッドロック回避）
	if (g_gameInput && g_deviceToken)
	{
		g_gameInput->UnregisterCallback(g_deviceToken, 5'000'000 /* マイクロ秒 */);
		g_deviceToken = 0;
	}

	std::lock_guard<std::mutex> lock(g_mutex);
	for (auto& s : g_slots) s.Reset();
	g_gameInput.Reset();
}
