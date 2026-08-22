#pragma once
#include "Memory.h"
#include "Addresses.h"
#include "ShortcutManager.h"

using namespace Timelapse;
using namespace System;
using namespace System::Threading;
using namespace System::Collections::Generic;

public delegate void UpdateTabControlDelegate(int index);


void toggleFullGodmode(System::Windows::Forms::CheckBox^ cb) {
	if (cb->Checked)
		WriteMemory(fullGodmodeAddr, 2, 0x0F, 0x84); // je 009596F7 [first 2 bytes]
	else
		WriteMemory(fullGodmodeAddr, 2, 0x0F, 0x85); // jne 009596F7 [first 2 bytes]
}

void toggleMpHack(System::Windows::Forms::CheckBox^ cb) {
	if (cb->Checked)
		Jump(mpHackAddr, Assembly::MpHackStartHook, 0);
	else
		Jump(mpHackAddr, Assembly::MpHackEndHook, 0);

}

void toggleDupeX() {
	Dictionary<String^, Control^>^ controls = Timelapse::MainForm::ControlMap;
	CheckBox^ cbDupeX = (CheckBox^)controls["DupeX"];
	TextBox^ tbDupeXFoothold = (TextBox^)controls["DupexFoothold"];
	Button^ bDupeXGetFoothold = (Button^)controls["DupexGetFoothold"];

	if (cbDupeX->Checked)
	{
		tbDupeXFoothold->Enabled = false;
		bDupeXGetFoothold->Enabled = false;
		Jump(dupeXAddr, Assembly::DupeXHook, 1);
	}
	else
	{
		tbDupeXFoothold->Enabled = true;
		bDupeXGetFoothold->Enabled = true;
		WriteMemory(dupeXAddr, 6, 0x89, 0xBE, 0x14, 0x01, 0x00, 0x00);
	}
}

void toggleFMA() {
	Dictionary<String^, Control^>^ controls = Timelapse::MainForm::ControlMap;
	CheckBox^ cbFullMapAttack = (CheckBox^)controls["FMA"];
	if (cbFullMapAttack->Checked)
		WriteMemory(fullMapAttackAddr, 2, 0x90, 0x90); // nop; nop;
	else
		WriteMemory(fullMapAttackAddr, 2, 0x74, 0x22); // je 006785EE
}

void setFilterTab(int index) {
	Dictionary<String^, Control^>^ controls = Timelapse::MainForm::ControlMap;
	TabControl^ tabControl1 = (TabControl^)controls["MainTabs"];
	tabControl1->SelectedIndex = index;
}

void addItemToFilter() {
	Dictionary<String^, Control^>^ controls = Timelapse::MainForm::ControlMap;
	if (Assembly::isItemFilterEnabled) return;

	TabControl^ tabControl1 = (TabControl^)controls["MainTabs"];
	ListBox^ lbItemFilter = (ListBox^)controls["ItemFilterListBox"];
	TextBox^ tbItemFilterID = (TextBox^)controls["ItemFilterID"];
	Button^ bItemFilterAdd = (Button^)controls["ItemFilterAdd"];
	lbItemFilter->Items->Clear();
	array<int>^ myArray = { 4031865,
							4031866,
							2340000,
							2049100 };

	tabControl1->Invoke(gcnew UpdateTabControlDelegate(setFilterTab), 5);
	List<int>^ myList = gcnew List<int>(myArray);

	for each (int item in myList)
	{
		tbItemFilterID->Text = String::Format("{0}", item);
		bItemFilterAdd->PerformClick();
	}

	tabControl1->Invoke(gcnew UpdateTabControlDelegate(setFilterTab), gcnew Int32(0));

}

void toggleAutoAttack() {
	Dictionary<String^, Control^>^ controls = Timelapse::MainForm::ControlMap;
	CheckBox^ cbAttack = (CheckBox^)controls["AutoAttack"];
	ComboBox^ comboAttackKey = (ComboBox^)controls["AutoAttackKey"];
	if (comboAttackKey->SelectedIndex != 13)
		comboAttackKey->SelectedIndex = 13; // D
}

void toggleMacros() {
	Dictionary<String^, Control^>^ controls = Timelapse::MainForm::ControlMap;
	CheckBox^ cbAttack = (CheckBox^)controls["AutoAttack"];
	CheckBox^ cbLoot = (CheckBox^)controls["AutoLoot"];
	BOOL isChecked = cbAttack->Checked || cbLoot->Checked;
	cbAttack->Checked = isChecked;
	cbLoot->Checked = isChecked;
	cbAttack->Checked = !cbAttack->Checked;
	cbLoot->Checked = !cbLoot->Checked;
}
