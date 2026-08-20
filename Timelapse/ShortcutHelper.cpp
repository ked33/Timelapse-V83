#include "ShortcutHelper.h"
#include "ShortcutManager.h"
#include "MainForm.h"
#include "Log.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Windows::Forms;

public delegate void ToggleAndLogDelegate(CheckBox^ cb);
public delegate void ToggleControlDelegate(String^ controlName);

void ToggleAndLog(CheckBox^ cb) {
	ShortcutHelper::ManualToggleCheckBox(cb);
	String^ state = cb->Checked ? "on" : "off";
	Log::WriteLineToConsole(String::Format("{0} was toggled {1}", cb->Name, state));
}

void ShortcutHelper::ManualToggleCheckBox(CheckBox^ cb) {
	cb->Checked = !cb->Checked;
}

static void ToggleControlOnUI(String^ controlName) {
	if (String::IsNullOrWhiteSpace(controlName))
		return;

	auto controls = Timelapse::MainForm::ControlMap;
	if (controls == nullptr)
		return;

	Control^ control;
	if (!controls->TryGetValue(controlName, control))
		return;

	CheckBox^ cb = dynamic_cast<CheckBox^>(control);
	// A control on an unselected TabPage may not have a native handle yet.
	// It is still safe to update it from the form UI thread.
	if (cb == nullptr || cb->IsDisposed || cb->Disposing)
		return;

	ToggleAndLog(cb);
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
	Timelapse::MainForm^ form = Timelapse::MainForm::TheInstance;
	if (form == nullptr || form->IsDisposed || form->Disposing || !form->IsHandleCreated)
		return;

	try {
		if (form->InvokeRequired)
			form->BeginInvoke(
				gcnew ToggleControlDelegate(&ToggleControlOnUI),
				gcnew array<Object^> { controlName });
		else
			ToggleControlOnUI(controlName);
	}
	catch (ObjectDisposedException^) {
		// The form can begin closing between the lifecycle check and BeginInvoke.
	}
	catch (InvalidOperationException^) {
		// The form can begin closing between the lifecycle check and BeginInvoke.
	}
}

void ShortcutHelper::ToggleControl(String^ controlName, Action^ additionalAction) {
	auto controls = Timelapse::MainForm::ControlMap;
	CheckBox^ cb = (CheckBox^)controls[controlName];
	cb->Invoke(gcnew ToggleAndLogDelegate(ToggleAndLog), cb);
	additionalAction();
}

void ShortcutHelper::TriggerPortalLoop() {
	Timelapse::MainForm^ form = Timelapse::MainForm::TheInstance;
	if (form == nullptr || form->IsDisposed || !form->IsHandleCreated)
		return;

	InvokeOnUI(gcnew Action(form, &Timelapse::MainForm::TriggerPortalLoop));
}
