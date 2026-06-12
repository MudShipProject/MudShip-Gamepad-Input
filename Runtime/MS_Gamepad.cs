using System;
using System.Runtime.InteropServices;
using UnityEngine;

public enum GamepadInput
{
	DPadUp, DPadDown, DPadLeft, DPadRight,
	A, B, X, Y,
	LB, RB,
	Start, Menu,
	LStickPress, RStickPress,
	LStickX, LStickY, RStickX, RStickY,
	LT, RT,
}

public static class MS_Gamepad
{
	public const int MaxPads = 32;

	const string Dll = "MS_GamepadBridge";

	[StructLayout(LayoutKind.Sequential)]
	struct GamepadFrame
	{
		public int connected;
		public uint buttons;
		public float lx, ly, rx, ry;
		public float lt, rt;
	}

	[DllImport(Dll)] static extern int GIB_Init();
	[DllImport(Dll)] static extern int GIB_Poll([Out] GamepadFrame[] frames, int maxCount);
	[DllImport(Dll)] static extern void GIB_Shutdown();

	static readonly GamepadFrame[] _frames = new GamepadFrame[MaxPads];
	static readonly uint[] _prevButtons = new uint[MaxPads];
	static int _polledFrame = -1;
	static bool _ready;
	static bool _initTried;

	static void EnsureInit()
	{
		if (_initTried) return;
		_initTried = true;

		if (!Application.isPlaying) return;

		try
		{
			_ready = GIB_Init() != 0;
		}
		catch (DllNotFoundException)
		{
			return;
		}
		catch (Exception e)
		{
			return;
		}

		if (!_ready)
		{
			return;
		}

		if (!Application.runInBackground)
		{
			Application.runInBackground = true;
		}

		var go = new GameObject("~MS_GamepadPump") { hideFlags = HideFlags.HideAndDontSave };
		UnityEngine.Object.DontDestroyOnLoad(go);
		go.AddComponent<Pump>();

		Application.quitting += Shutdown;
#if UNITY_EDITOR
		UnityEditor.EditorApplication.playModeStateChanged += OnPlayModeChanged;
#endif
	}

#if UNITY_EDITOR
	static void OnPlayModeChanged(UnityEditor.PlayModeStateChange change)
	{
		if (change == UnityEditor.PlayModeStateChange.ExitingPlayMode)
		{
			UnityEditor.EditorApplication.playModeStateChanged -= OnPlayModeChanged;
			Shutdown();
			_initTried = false;
		}
	}
#endif
	static void Poll()
	{
		if (!_ready) return;
		int frame = Time.frameCount;
		if (frame == _polledFrame) return;
		_polledFrame = frame;
		for (int i = 0; i < MaxPads; i++) _prevButtons[i] = _frames[i].buttons;
		GIB_Poll(_frames, MaxPads);
	}

	static void Shutdown()
	{
		if (!_ready) return;
		_ready = false;
		try { GIB_Shutdown(); } catch { }
		Array.Clear(_frames, 0, _frames.Length);
		Array.Clear(_prevButtons, 0, _prevButtons.Length);
		_polledFrame = -1;
	}

	// ================= 公開API =================

	public static float GetState(int index, GamepadInput input)
	{
		EnsureInit();
		Poll();
		if (!_ready || index < 0 || index >= MaxPads) return 0f;

		var f = _frames[index];
		if (f.connected == 0) return 0f;

		switch (input)
		{
			case GamepadInput.LStickX: return f.lx;
			case GamepadInput.LStickY: return f.ly;
			case GamepadInput.RStickX: return f.rx;
			case GamepadInput.RStickY: return f.ry;
			case GamepadInput.LT: return f.lt;
			case GamepadInput.RT: return f.rt;
			default: return (f.buttons & ButtonBit(input)) != 0 ? 1f : 0f;
		}
	}

	/// ボタンが押されている間 true（Unity の GetButton 相当）。
	public static bool GetButton(int index, GamepadInput input) => GetState(index, input) != 0f;

	/// ボタンが押された瞬間のフレームだけ true（Unity の GetButtonDown 相当）。
	/// 軸（スティック/トリガー）を指定した場合は常に false。
	public static bool GetButtonDown(int index, GamepadInput input)
	{
		EnsureInit();
		Poll();
		if (!_ready || index < 0 || index >= MaxPads) return false;
		var f = _frames[index];
		if (f.connected == 0) return false;
		uint bit = ButtonBit(input);
		return bit != 0 && (f.buttons & bit) != 0 && (_prevButtons[index] & bit) == 0;
	}

	/// ボタンが離された瞬間のフレームだけ true（Unity の GetButtonUp 相当）。
	public static bool GetButtonUp(int index, GamepadInput input)
	{
		EnsureInit();
		Poll();
		if (!_ready || index < 0 || index >= MaxPads) return false;
		if (_frames[index].connected == 0) return false;
		uint bit = ButtonBit(input);
		return bit != 0 && (_frames[index].buttons & bit) == 0 && (_prevButtons[index] & bit) != 0;
	}

	/// index のコントローラが接続中か。
	public static bool IsConnected(int index)
	{
		EnsureInit();
		Poll();
		return _ready && index >= 0 && index < MaxPads && _frames[index].connected != 0;
	}

	/// 接続中の台数。
	public static int ConnectedCount
	{
		get
		{
			EnsureInit();
			Poll();
			if (!_ready) return 0;
			int c = 0;
			for (int i = 0; i < MaxPads; i++) if (_frames[i].connected != 0) c++;
			return c;
		}
	}

	// GameInputGamepadButtons のビット（ネイティブ GameInput.h と一致させる）
	static uint ButtonBit(GamepadInput i)
	{
		switch (i)
		{
			case GamepadInput.Start: return 0x0001; // ≡  GameInputGamepadMenu
			case GamepadInput.Menu: return 0x0002; // ⧉  GameInputGamepadView
			case GamepadInput.A: return 0x0004;
			case GamepadInput.B: return 0x0008;
			case GamepadInput.X: return 0x0010;
			case GamepadInput.Y: return 0x0020;
			case GamepadInput.DPadUp: return 0x0040;
			case GamepadInput.DPadDown: return 0x0080;
			case GamepadInput.DPadLeft: return 0x0100;
			case GamepadInput.DPadRight: return 0x0200;
			case GamepadInput.LB: return 0x0400;
			case GamepadInput.RB: return 0x0800;
			case GamepadInput.LStickPress: return 0x1000;
			case GamepadInput.RStickPress: return 0x2000;
			default: return 0;
		}
	}

	// 毎フレーム Poll を回すための隠しコンポーネント（他スクリプトより先に実行され、
	// 誰も API を呼ばないフレームでも状態を前進させてエッジの取りこぼしを防ぐ）
	[DefaultExecutionOrder(-32000)]
	class Pump : MonoBehaviour
	{
		void Update() => Poll();
	}
}
