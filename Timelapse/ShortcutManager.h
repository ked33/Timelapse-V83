#pragma once

using namespace System;
using namespace System::Collections::Generic;

public enum class ShortcutActionId
{
	ClickTeleport,
	MouseFly,
	MouseTeleport,
	SwimInAir
};

[FlagsAttribute]
public enum class ShortcutModifiers
{
	None = 0,
	Ctrl = 1,
	Alt = 2,
	Shift = 4
};

public ref class ShortcutBinding
{
public:
	ShortcutActionId ActionId;
	int VirtualKey;
	ShortcutModifiers Modifiers;
	bool Enabled;

	ShortcutBinding(ShortcutActionId actionId);
	ShortcutBinding(ShortcutActionId actionId, int virtualKey, ShortcutModifiers modifiers, bool enabled);
	ShortcutBinding^ Clone();
};

public delegate void ShortcutAction();

public ref class ShortcutManager
{
private:
	static ShortcutManager^ instance;
	Dictionary<int, List<ShortcutAction^>^>^ legacyShortcuts;
	Dictionary<ShortcutActionId, ShortcutBinding^>^ bindings;
	Dictionary<ShortcutActionId, ShortcutAction^>^ actions;
	Dictionary<int, bool>^ suppressedKeys;
	Object^ syncRoot;
	IntPtr keyboardHook;
	IntPtr gameWindow;
	UInt32 gameThreadId;

	ShortcutManager();
	ShortcutModifiers GetCurrentModifiers();
	bool IsGameWindowForeground();
	String^ GetLegacyShortcutName(int virtualKey);
	static bool IsModifierKey(System::Windows::Forms::Keys key);

public:
	static ShortcutManager^ Instance();
	static array<ShortcutActionId>^ GetConfigurableActions();
	static String^ GetActionDisplayName(ShortcutActionId actionId);

	void RegisterShortcut(int key, ShortcutAction^ action);
	void RegisterAction(ShortcutActionId actionId, ShortcutAction^ action);
	ShortcutBinding^ GetBinding(ShortcutActionId actionId);
	Dictionary<ShortcutActionId, ShortcutBinding^>^ GetBindings();
	void ApplyBindings(Dictionary<ShortcutActionId, ShortcutBinding^>^ proposedBindings);
	bool ValidateBindings(Dictionary<ShortcutActionId, ShortcutBinding^>^ proposedBindings, String^% errorMessage);
	bool TryParseBinding(ShortcutActionId actionId, String^ text, ShortcutBinding^% binding, String^% errorMessage);
	String^ FormatBinding(ShortcutBinding^ binding);

	bool Start(IntPtr gameWindowHandle);
	void Stop();
	bool HandleKeyboardMessage(int virtualKey, Int64 messageFlags);
};
