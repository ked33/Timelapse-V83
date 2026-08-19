#include "ShortcutHelper.h"
#include "ShortcutManager.h"
#include "MainForm.h"
#include "Log.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Windows::Forms;

public delegate void ToggleAndLogDelegate(CheckBox^ cb);

void ToggleAndLog(CheckBox^ cb) {
	ShortcutHelper::ManualToggleCheckBox(cb);
	String^ state = cb->Checked ? "on" : "off";
	Log::WriteLineToConsole(String::Format("{0} was toggled {1}", cb->Name, state));
}

void ShortcutHelper::ManualToggleCheckBox(CheckBox^ cb) {
	cb->Checked = !cb->Checked;
}

void ShortcutHelper::InvokeOnUI(Action^ action) {
	Timelapse::MainForm^ form = Timelapse::MainForm::TheInstance;
	if (form == nullptr || form->IsDisposed || !form->IsHandleCreated || action == nullptr)
		return;

	if (form->InvokeRequired)
		form->BeginInvoke(action);
	else
		action->Invoke();
}

void ShortcutHelper::ToggleControl(String^ controlName) {
	auto controls = Timelapse::MainForm::ControlMap;
	Control^ control;
	if (!controls->TryGetValue(controlName, control))
		return;

	CheckBox^ cb = dynamic_cast<CheckBox^>(control);
	if (cb == nullptr || cb->IsDisposed || !cb->IsHandleCreated)
		return;

	if (cb->InvokeRequired)
		cb->BeginInvoke(gcnew ToggleAndLogDelegate(ToggleAndLog), cb);
	else
		ToggleAndLog(cb);
}

void ShortcutHelper::ToggleControl(String^ controlName, Action^ additionalAction) {
	auto controls = Timelapse::MainForm::ControlMap;
	CheckBox^ cb = (CheckBox^)controls[controlName];
	cb->Invoke(gcnew ToggleAndLogDelegate(ToggleAndLog), cb);
	additionalAction();
}

void ShortcutHelper::TogglePortalLoop() {
	Timelapse::MainForm^ form = Timelapse::MainForm::TheInstance;
	if (form == nullptr || form->IsDisposed || !form->IsHandleCreated)
		return;

	InvokeOnUI(gcnew Action(form, &Timelapse::MainForm::TogglePortalLoop));
}
