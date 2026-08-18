#pragma once

#include "ShortcutManager.h"
#include "CheatFunctions.h"
#include "MainForm.h"
#include "ShortcutHelper.h"
#include "Log.h"
#include "CheatNames.h"


//void GodModeAction() {
//	Dictionary<String^, Control^>^ controls = Timelapse::MainForm::ControlMap;
//	CheckBox^ cb = (CheckBox^)controls["GodMode"];
//	cb->Checked = !cb->Checked;
//	toggleFullGodmode(cb);
//	String^ state = cb->Checked ? "On" : "Off";
//	Log::WriteLineToConsole(String::Format("DupeX was toggled {0}", state));
//}
//
//void RegisterGodMode() {
//	ShortcutManager^ sm = ShortcutManager::Instance();
//	sm->RegisterShortcut(VK_F1, gcnew ShortcutAction(GodModeAction));
//}


void DupeXAction() {
	ShortcutHelper::ToggleControl("DupeX");
}

void FMAAction() {
	ShortcutHelper::ToggleControl("FMA");
}

void SetAutoAttackKeyAndToggle() {
	auto controls = Timelapse::MainForm::ControlMap;
	ComboBox^ comboAttackKey = safe_cast<ComboBox^>(controls["AutoAttackKey"]);
	CheckBox^ cbAttack = safe_cast<CheckBox^>(controls["AutoAttack"]);
	if (comboAttackKey->SelectedIndex != 13)
		comboAttackKey->SelectedIndex = 13;
	ShortcutHelper::ManualToggleCheckBox(cbAttack);
}

void AutoAttackAction() {
	ShortcutHelper::InvokeOnUI(gcnew Action(SetAutoAttackKeyAndToggle));
}

void AddItemToFilterAction() {
	ShortcutHelper::InvokeOnUI(gcnew Action(addItemToFilter));
}

void ClickTeleportAction() {
	ShortcutHelper::ToggleControl("ClickTeleport");
}

void MouseTeleportAction() {
	ShortcutHelper::ToggleControl("MouseTeleport");
}

void MouseFlyAction() {
	ShortcutHelper::ToggleControl("MouseFly");
}

void SwimInAirAction() {
	ShortcutHelper::ToggleControl("SwimInAir");
}




void RegisterShortcuts() {
	ShortcutManager^ sm = ShortcutManager::Instance();
	sm->RegisterShortcut(VK_F1, gcnew ShortcutAction(DupeXAction));
	sm->RegisterShortcut(VK_F1, gcnew ShortcutAction(FMAAction));
	sm->RegisterShortcut(VK_F2, gcnew ShortcutAction(AddItemToFilterAction));
	sm->RegisterShortcut(VK_F3, gcnew ShortcutAction(AutoAttackAction));
	sm->RegisterAction(ShortcutActionId::ClickTeleport, gcnew ShortcutAction(ClickTeleportAction));
	sm->RegisterAction(ShortcutActionId::MouseTeleport, gcnew ShortcutAction(MouseTeleportAction));
	sm->RegisterAction(ShortcutActionId::MouseFly, gcnew ShortcutAction(MouseFlyAction));
	sm->RegisterAction(ShortcutActionId::SwimInAir, gcnew ShortcutAction(SwimInAirAction));

}


