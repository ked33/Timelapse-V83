#pragma once
#include "ShortcutManager.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Windows::Forms;

public ref class ShortcutHelper {
public:
	static void InvokeOnUI(Action^ action);
	static void ToggleControl(String^ controlName);
	static void ToggleControl(String^ controlName, Action^ additionalAction);
	static void TogglePortalLoop();
	static void ManualToggleCheckBox(CheckBox^ cb);
};
	
