#include <windows.h>
#include "ShortcutManager.h"

using namespace System::ComponentModel;
using namespace System::Globalization;
using namespace System::Threading;
using namespace System::Windows::Forms;

LRESULT CALLBACK GameKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

ShortcutBinding::ShortcutBinding(ShortcutActionId actionId)
	: ShortcutBinding(actionId, 0, ShortcutModifiers::None, false)
{
}

ShortcutBinding::ShortcutBinding(ShortcutActionId actionId, int virtualKey, ShortcutModifiers modifiers, bool enabled)
{
	ActionId = actionId;
	VirtualKey = virtualKey;
	Modifiers = modifiers;
	Enabled = enabled;
}

ShortcutBinding^ ShortcutBinding::Clone()
{
	return gcnew ShortcutBinding(ActionId, VirtualKey, Modifiers, Enabled);
}

ShortcutManager::ShortcutManager()
{
	legacyShortcuts = gcnew Dictionary<int, List<ShortcutAction^>^>();
	bindings = gcnew Dictionary<ShortcutActionId, ShortcutBinding^>();
	actions = gcnew Dictionary<ShortcutActionId, ShortcutAction^>();
	suppressedKeys = gcnew Dictionary<int, bool>();
	syncRoot = gcnew Object();
	keyboardHook = IntPtr::Zero;
	gameWindow = IntPtr::Zero;
	gameThreadId = 0;

	for each (ShortcutActionId actionId in GetConfigurableActions())
		bindings[actionId] = gcnew ShortcutBinding(actionId);
}

ShortcutManager^ ShortcutManager::Instance()
{
	if (instance == nullptr)
		instance = gcnew ShortcutManager();

	return instance;
}

array<ShortcutActionId>^ ShortcutManager::GetConfigurableActions()
{
	return gcnew array<ShortcutActionId>
	{
		ShortcutActionId::ClickTeleport,
		ShortcutActionId::MouseFly,
		ShortcutActionId::MouseTeleport,
		ShortcutActionId::SwimInAir,
		ShortcutActionId::PortalLoop
	};
}

String^ ShortcutManager::GetActionDisplayName(ShortcutActionId actionId)
{
	switch (actionId)
	{
	case ShortcutActionId::ClickTeleport:
		return L"鼠标点击传送";
	case ShortcutActionId::MouseFly:
		return L"鼠标飞行 [CS]";
	case ShortcutActionId::MouseTeleport:
		return L"鼠标传送";
	case ShortcutActionId::SwimInAir:
		return L"在空中游泳";
	case ShortcutActionId::PortalLoop:
		return L"双图光柱循环";
	default:
		return actionId.ToString();
	}
}

void ShortcutManager::RegisterShortcut(int key, ShortcutAction^ action)
{
	if (action == nullptr)
		return;

	Monitor::Enter(syncRoot);
	try
	{
		if (!legacyShortcuts->ContainsKey(key))
			legacyShortcuts[key] = gcnew List<ShortcutAction^>();

		legacyShortcuts[key]->Add(action);
	}
	finally
	{
		Monitor::Exit(syncRoot);
	}
}

void ShortcutManager::RegisterAction(ShortcutActionId actionId, ShortcutAction^ action)
{
	if (action == nullptr)
		return;

	Monitor::Enter(syncRoot);
	try
	{
		actions[actionId] = action;
	}
	finally
	{
		Monitor::Exit(syncRoot);
	}
}

ShortcutBinding^ ShortcutManager::GetBinding(ShortcutActionId actionId)
{
	Monitor::Enter(syncRoot);
	try
	{
		ShortcutBinding^ binding;
		if (bindings->TryGetValue(actionId, binding))
			return binding->Clone();

		return gcnew ShortcutBinding(actionId);
	}
	finally
	{
		Monitor::Exit(syncRoot);
	}
}

Dictionary<ShortcutActionId, ShortcutBinding^>^ ShortcutManager::GetBindings()
{
	auto result = gcnew Dictionary<ShortcutActionId, ShortcutBinding^>();

	Monitor::Enter(syncRoot);
	try
	{
		for each (KeyValuePair<ShortcutActionId, ShortcutBinding^> entry in bindings)
			result[entry.Key] = entry.Value->Clone();
	}
	finally
	{
		Monitor::Exit(syncRoot);
	}

	return result;
}

void ShortcutManager::ApplyBindings(Dictionary<ShortcutActionId, ShortcutBinding^>^ proposedBindings)
{
	if (proposedBindings == nullptr)
		return;

	Monitor::Enter(syncRoot);
	try
	{
		for each (ShortcutActionId actionId in GetConfigurableActions())
		{
			ShortcutBinding^ binding;
			if (proposedBindings->TryGetValue(actionId, binding) && binding != nullptr)
				bindings[actionId] = binding->Clone();
			else
				bindings[actionId] = gcnew ShortcutBinding(actionId);
		}
	}
	finally
	{
		Monitor::Exit(syncRoot);
	}
}

String^ ShortcutManager::GetLegacyShortcutName(int virtualKey)
{
	switch (virtualKey)
	{
	case VK_F1:
		return L"物品复制 / 全图攻击";
	case VK_F2:
		return L"添加物品过滤";
	case VK_F3:
		return L"自动攻击";
	default:
		return L"现有功能";
	}
}

bool ShortcutManager::ValidateBindings(Dictionary<ShortcutActionId, ShortcutBinding^>^ proposedBindings, String^% errorMessage)
{
	errorMessage = nullptr;
	if (proposedBindings == nullptr)
	{
		errorMessage = L"快捷键配置无效。";
		return false;
	}

	auto usedBindings = gcnew Dictionary<String^, ShortcutActionId>(StringComparer::OrdinalIgnoreCase);

	Monitor::Enter(syncRoot);
	try
	{
		for each (ShortcutActionId actionId in GetConfigurableActions())
		{
			ShortcutBinding^ binding;
			if (!proposedBindings->TryGetValue(actionId, binding) || binding == nullptr || !binding->Enabled)
				continue;

			if (binding->Modifiers == ShortcutModifiers::None &&
				legacyShortcuts->ContainsKey(binding->VirtualKey))
			{
				errorMessage = String::Format(
					L"{0} 已分配给“{1}”。",
					FormatBinding(binding),
					GetLegacyShortcutName(binding->VirtualKey));
				return false;
			}

			String^ chord = String::Format(
				CultureInfo::InvariantCulture,
				L"{0}:{1}",
				binding->VirtualKey,
				static_cast<int>(binding->Modifiers));

			ShortcutActionId existingAction;
			if (usedBindings->TryGetValue(chord, existingAction))
			{
				errorMessage = String::Format(
					L"{0} 已分配给“{1}”。",
					FormatBinding(binding),
					GetActionDisplayName(existingAction));
				return false;
			}

			usedBindings[chord] = actionId;
		}
	}
	finally
	{
		Monitor::Exit(syncRoot);
	}

	return true;
}

bool ShortcutManager::IsModifierKey(Keys key)
{
	return key == Keys::ControlKey || key == Keys::LControlKey || key == Keys::RControlKey ||
		key == Keys::ShiftKey || key == Keys::LShiftKey || key == Keys::RShiftKey ||
		key == Keys::Menu || key == Keys::LMenu || key == Keys::RMenu ||
		key == Keys::LWin || key == Keys::RWin;
}

bool ShortcutManager::TryParseBinding(
	ShortcutActionId actionId,
	String^ text,
	ShortcutBinding^% binding,
	String^% errorMessage)
{
	binding = gcnew ShortcutBinding(actionId);
	errorMessage = nullptr;

	if (String::IsNullOrWhiteSpace(text))
		return true;

	try
	{
		KeysConverter^ converter = gcnew KeysConverter();
		Keys parsedKeys = safe_cast<Keys>(converter->ConvertFromString(text->Trim()));
		Keys keyCode = parsedKeys & Keys::KeyCode;

		if (keyCode == Keys::None || IsModifierKey(keyCode))
		{
			errorMessage = L"必须包含一个非修饰键。";
			return false;
		}

		ShortcutModifiers modifiers = ShortcutModifiers::None;
		if ((parsedKeys & Keys::Control) == Keys::Control)
			modifiers = modifiers | ShortcutModifiers::Ctrl;
		if ((parsedKeys & Keys::Alt) == Keys::Alt)
			modifiers = modifiers | ShortcutModifiers::Alt;
		if ((parsedKeys & Keys::Shift) == Keys::Shift)
			modifiers = modifiers | ShortcutModifiers::Shift;

		bool hasCtrl = (modifiers & ShortcutModifiers::Ctrl) == ShortcutModifiers::Ctrl;
		bool hasAlt = (modifiers & ShortcutModifiers::Alt) == ShortcutModifiers::Alt;
		if ((hasAlt && keyCode == Keys::Tab) ||
			(hasAlt && keyCode == Keys::F4) ||
			(hasCtrl && hasAlt && keyCode == Keys::Delete))
		{
			errorMessage = L"不能使用 Windows 系统快捷键。";
			return false;
		}

		binding = gcnew ShortcutBinding(actionId, static_cast<int>(keyCode), modifiers, true);
		return true;
	}
	catch (Exception^)
	{
		errorMessage = L"格式无效，请输入类似 Ctrl+F 或 F5 的快捷键。";
		return false;
	}
}

String^ ShortcutManager::FormatBinding(ShortcutBinding^ binding)
{
	if (binding == nullptr || !binding->Enabled || binding->VirtualKey == 0)
		return String::Empty;

	auto parts = gcnew List<String^>();
	if ((binding->Modifiers & ShortcutModifiers::Ctrl) == ShortcutModifiers::Ctrl)
		parts->Add(L"Ctrl");
	if ((binding->Modifiers & ShortcutModifiers::Alt) == ShortcutModifiers::Alt)
		parts->Add(L"Alt");
	if ((binding->Modifiers & ShortcutModifiers::Shift) == ShortcutModifiers::Shift)
		parts->Add(L"Shift");

	KeysConverter^ converter = gcnew KeysConverter();
	String^ keyName = converter->ConvertToString(static_cast<Keys>(binding->VirtualKey));
	parts->Add(keyName);
	return String::Join(L"+", parts->ToArray());
}

bool ShortcutManager::Start(IntPtr gameWindowHandle)
{
	Stop();

	HWND hwnd = static_cast<HWND>(gameWindowHandle.ToPointer());
	if (hwnd == nullptr || !IsWindow(hwnd))
		return false;

	DWORD processId = 0;
	DWORD threadId = GetWindowThreadProcessId(hwnd, &processId);
	if (threadId == 0 || processId != GetCurrentProcessId())
		return false;

	HHOOK hook = SetWindowsHookEx(WH_KEYBOARD, GameKeyboardProc, nullptr, threadId);
	if (hook == nullptr)
		return false;

	gameWindow = gameWindowHandle;
	gameThreadId = threadId;
	keyboardHook = IntPtr(hook);
	return true;
}

void ShortcutManager::Stop()
{
	if (keyboardHook != IntPtr::Zero)
	{
		UnhookWindowsHookEx(static_cast<HHOOK>(keyboardHook.ToPointer()));
		keyboardHook = IntPtr::Zero;
	}

	Monitor::Enter(syncRoot);
	try
	{
		suppressedKeys->Clear();
	}
	finally
	{
		Monitor::Exit(syncRoot);
	}

	gameWindow = IntPtr::Zero;
	gameThreadId = 0;
}

ShortcutModifiers ShortcutManager::GetCurrentModifiers()
{
	ShortcutModifiers modifiers = ShortcutModifiers::None;
	if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0)
		modifiers = modifiers | ShortcutModifiers::Ctrl;
	if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0)
		modifiers = modifiers | ShortcutModifiers::Alt;
	if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0)
		modifiers = modifiers | ShortcutModifiers::Shift;
	return modifiers;
}

bool ShortcutManager::IsGameWindowForeground()
{
	HWND hwnd = static_cast<HWND>(gameWindow.ToPointer());
	if (hwnd == nullptr || !IsWindow(hwnd))
		return false;

	HWND foreground = GetForegroundWindow();
	if (foreground == hwnd)
		return true;

	// The client can optionally be embedded in the Timelapse window.
	return GetAncestor(hwnd, GA_ROOT) == foreground;
}

bool ShortcutManager::HandleKeyboardMessage(int virtualKey, Int64 messageFlags)
{
	const bool isKeyUp = (messageFlags & 0x80000000LL) != 0;
	const bool wasPreviouslyDown = (messageFlags & 0x40000000LL) != 0;

	if (isKeyUp)
	{
		bool wasSuppressed = false;
		Monitor::Enter(syncRoot);
		try
		{
		wasSuppressed = suppressedKeys->Remove(virtualKey);
		}
		finally
		{
			Monitor::Exit(syncRoot);
		}
		return wasSuppressed;
	}

	if (!IsGameWindowForeground())
		return false;

	if (wasPreviouslyDown)
	{
		Monitor::Enter(syncRoot);
		try
		{
			return suppressedKeys->ContainsKey(virtualKey);
		}
		finally
		{
			Monitor::Exit(syncRoot);
		}
	}

	// Posted macro messages do not represent a physically pressed key.
	if ((GetAsyncKeyState(virtualKey) & 0x8000) == 0)
		return false;

	ShortcutModifiers modifiers = GetCurrentModifiers();
	ShortcutAction^ customAction = nullptr;
	List<ShortcutAction^>^ legacyActions = nullptr;

	Monitor::Enter(syncRoot);
	try
	{
		suppressedKeys->Remove(virtualKey);

		for each (KeyValuePair<ShortcutActionId, ShortcutBinding^> entry in bindings)
		{
			ShortcutBinding^ binding = entry.Value;
			if (!binding->Enabled || binding->VirtualKey != virtualKey || binding->Modifiers != modifiers)
				continue;

			if (actions->TryGetValue(entry.Key, customAction))
				suppressedKeys[virtualKey] = true;
			break;
		}

		List<ShortcutAction^>^ registeredLegacyActions;
		if (modifiers == ShortcutModifiers::None && legacyShortcuts->TryGetValue(virtualKey, registeredLegacyActions))
			legacyActions = gcnew List<ShortcutAction^>(registeredLegacyActions);
	}
	finally
	{
		Monitor::Exit(syncRoot);
	}

	try
	{
		if (customAction != nullptr)
			customAction->Invoke();

		if (legacyActions != nullptr)
		{
			for each (ShortcutAction^ action in legacyActions)
				action->Invoke();
		}
	}
	catch (Exception^)
	{
		// Never let a shortcut action break the game's keyboard message chain.
	}

	return customAction != nullptr;
}

LRESULT CALLBACK GameKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		bool suppress = ShortcutManager::Instance()->HandleKeyboardMessage(
			static_cast<int>(wParam),
			static_cast<Int64>(lParam));
		if (suppress)
			return 1;
	}

	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
