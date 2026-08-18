#include "Settings.h"
#include "Log.h"
#include "MainForm.h"

using namespace Timelapse;
using namespace System;
using namespace IO;
using namespace Collections::Generic;

Object^ MacroSystem::Settings::Deserialize(String^ path, XmlSerializer^ serializer) {
	if (File::Exists(path)) {
		FileStream^ stream = {};

		try {
			stream = gcnew FileStream(path, FileMode::Open, FileAccess::Read, FileShare::Read);
			return serializer->Deserialize(stream);
		}
		catch (Exception^ ex) {
			Log::WriteLine("读取设定档时发生异常 " + path + " : " + ex->Message);
		}
		finally {
			if (stream)
				delete static_cast<IDisposable^>(stream);

			Log::WriteLine("加载 " + path);
		}
	}

	return nullptr;
}

void MacroSystem::Settings::Serialize(String^ path, XmlSerializer^ serializer, Object^ object) {
	if (path != nullptr && serializer != nullptr && object != nullptr) {
		FileStream^ stream = {};

		try {
			stream = gcnew FileStream(path, FileMode::Create, FileAccess::Write, FileShare::None);
			serializer->Serialize(stream, object);
		}
		catch (Exception^ ex) {
			Log::WriteLine("写入设定档时发生异常 " + path + " : " + ex->Message);
		}
		finally {
			if (stream)
				delete static_cast<IDisposable^>(stream);

			Log::WriteLine("保存 " + path);
		}
	}
}

void MacroSystem::Settings::Serialize(Control^ c, String^ XmlFileName) {
	XmlTextWriter^ xmlSerializedForm = gcnew XmlTextWriter(XmlFileName, System::Text::Encoding::Default);

	xmlSerializedForm->Formatting = Formatting::Indented;
	xmlSerializedForm->WriteStartDocument();
	xmlSerializedForm->WriteStartElement("Timelapse");

	// enumerate all controls on the form, and serialize them as appropriate
	AddChildControls(xmlSerializedForm, c);

	xmlSerializedForm->WriteEndElement();
	xmlSerializedForm->WriteEndDocument();
	xmlSerializedForm->Flush();
	xmlSerializedForm->Close();

	Log::WriteLine("已保存 " + XmlFileName);
}

bool MacroSystem::Settings::isExcluded(Control^ ctrl) {
	const auto ctrlName = ctrl->Name;
	if (ctrlName == "lbConsoleLog" || ctrlName == "tbItemFilterSearch" || ctrlName == "tbItemFilterSearch" || ctrlName == "lbItemSearchLog" ||
		ctrlName == "lbMobSearchLog" || ctrlName == "tbMobFilterSearch" || ctrlName == "tbSendPacket" || ctrlName == "tbMapRusherSearch" ||
		ctrlName == "lvMapRusherSearch")
		return true;

	return false;
}

void MacroSystem::Settings::AddChildControls(XmlTextWriter^ xmlSerializedForm, Control^ c) {
	for each (Control ^ childCtrl in c->Controls) {
		if (c->Name == "lvBuff")
		{
			Console::WriteLine("lvBuff");
		}
		auto ctrlType = childCtrl->GetType();
		auto ctrlName = childCtrl->Name;

		//TODO: save press state of buttons?
		if (childCtrl->HasChildren || ctrlType == ComboBox::typeid || ctrlType == NumericUpDown::typeid || ctrlType == CheckBox::typeid ||
			ctrlType == TextBox::typeid || ctrlType == ListBox::typeid || ctrlType == ListView::typeid && c && !isExcluded(childCtrl)) {
			// serialize this control
			xmlSerializedForm->WriteStartElement("Control");
			xmlSerializedForm->WriteAttributeString("Name", ctrlName);

			if (ctrlType == ComboBox::typeid)
				xmlSerializedForm->WriteAttributeString("SelectedIndex", safe_cast<ComboBox^>(childCtrl)->SelectedIndex.ToString());
			else if (ctrlType == NumericUpDown::typeid)
				xmlSerializedForm->WriteAttributeString("Value", safe_cast<NumericUpDown^>(childCtrl)->Value.ToString());
			else if (ctrlType == CheckBox::typeid)
				xmlSerializedForm->WriteAttributeString("Checked", safe_cast<CheckBox^>(childCtrl)->Checked.ToString());
			else if (ctrlType == TextBox::typeid)
				xmlSerializedForm->WriteAttributeString("Text", safe_cast<TextBox^>(childCtrl)->Text->ToString());
			else if (ctrlType == ListBox::typeid) {
				ListBox^ listBox = safe_cast<ListBox^>(childCtrl);
				int listBoxItemCnt = listBox->Items->Count;

				xmlSerializedForm->WriteAttributeString("ItemCnt", listBoxItemCnt.ToString());

				for (int i = 0; i < listBoxItemCnt; i++) {
					String^ listBoxItemStr = listBox->Items[i]->ToString();
					String^ listBoxItemNmbStr = ctrlName + "Item" + i.ToString();
					xmlSerializedForm->WriteAttributeString(listBoxItemNmbStr, listBoxItemStr);
				}
			}
			else if (ctrlType == ListView::typeid) {
				ListView^ lv = (ListView^)(childCtrl);
				int lvItemCnt = lv->Items->Count;

				if (ctrlName == "lvBuff") {
					xmlSerializedForm->WriteAttributeString("ControlName", "lvBuff");
					xmlSerializedForm->WriteAttributeString("ItemCnt", lvItemCnt.ToString());

					for (int i = 0; i < lvItemCnt; i++) {
						ListViewItem^ lvItem = lv->Items[i];
						String^ lvItemStr = lvItem->Text->ToString();
						String^ lvItemNmbStr = ctrlName + "Item" + i.ToString();

						xmlSerializedForm->WriteStartElement("Control");
						xmlSerializedForm->WriteAttributeString("Name", lvItemNmbStr);
						xmlSerializedForm->WriteAttributeString("Text", lvItemStr);

						MacroData^ m = (MacroData^)(lvItem->Tag);
						if (m != nullptr) {
							xmlSerializedForm->WriteAttributeString("Interval", m->interval.ToString());
							xmlSerializedForm->WriteAttributeString("KeyName", m->keyName);
							xmlSerializedForm->WriteAttributeString("KeyCode", m->keyCode.ToString());
							xmlSerializedForm->WriteAttributeString("Checked", lvItem->Checked.ToString());
						}

						xmlSerializedForm->WriteEndElement(); // Close ListViewItem element
					}
				}
			}


			// see if this control has any children, and if so, serialize them
			if (childCtrl->HasChildren && ctrlType != NumericUpDown::typeid)
			{
				AddChildControls(xmlSerializedForm, childCtrl);
			}

			xmlSerializedForm->WriteEndElement(); // Control
		}
	}
}

void MacroSystem::Settings::Deserialize(Control^ c, String^ XmlFileName) {
	if (File::Exists(XmlFileName)) {
		try {
			XmlDocument^ xmlSerializedForm = gcnew XmlDocument();
			xmlSerializedForm->Load(XmlFileName);

			XmlNode^ topLevel = xmlSerializedForm->ChildNodes[1];
			for each (XmlNode ^ n in topLevel->ChildNodes)
				SetControlProperties(safe_cast<Control^>(c), n);
		}
		catch (Exception^ ex) {
			Log::WriteLine("反序列化 \"" + c->Name + "\" 时发生异常: \"" + ex->Message + "\"");
		}
	}
}

void MacroSystem::Settings::SetControlProperties(Control^ currentCtrl, XmlNode^ n) {
	try {
		// get the control's name and type
		String^ ctrlName = n->Attributes["Name"]->Value;
		// find the control
		auto ctrl = currentCtrl->Controls->Find(ctrlName, true);

		// the right type too
		if (n->Attributes["SelectedIndex"])
			safe_cast<ComboBox^>(ctrl[0])->SelectedIndex = Convert::ToInt32(n->Attributes["SelectedIndex"]->Value);
		else if (n->Attributes["Value"])
			safe_cast<NumericUpDown^>(ctrl[0])->Value = Convert::ToDecimal(n->Attributes["Value"]->Value);
		else if (n->Attributes["Checked"])
			safe_cast<CheckBox^>(ctrl[0])->Checked = Convert::ToBoolean(n->Attributes["Checked"]->Value);
		else if (n->Attributes["Text"])
			safe_cast<TextBox^>(ctrl[0])->Text = Convert::ToString(n->Attributes["Text"]->Value);
		else if (n->Attributes["ItemCnt"]) {
			// if listview
			int itemCnt = Convert::ToInt32(n->Attributes["ItemCnt"]->Value);
			if (itemCnt < 1) return;

			if (n != nullptr && n->Attributes != nullptr && n->Attributes["ControlName"] != nullptr) {
				if (Convert::ToString(n->Attributes["ControlName"]->Value)) {
					ListView^ listView = safe_cast<ListView^>(ctrl[0]);
					listView->Items->Clear();  // Clear existing items before deserializing new ones
					XmlNodeList^ listViewItems = n->SelectNodes("Control");
					for each (XmlNode ^ listViewItemNode in listViewItems) {
						String^ itemText = Convert::ToString(listViewItemNode->Attributes["Text"]->Value);
						int keyCode = Convert::ToInt32(listViewItemNode->Attributes["KeyCode"]->Value);
						String^ keyName = listViewItemNode->Attributes["KeyName"]->Value;
						int interval = Convert::ToInt32(listViewItemNode->Attributes["Interval"]->Value);
						bool checked = Convert::ToBoolean(listViewItemNode->Attributes["Checked"]->Value);
						MacroData^ m = gcnew MacroData(keyCode, interval, keyName);
						ListViewItem^ listViewItem = gcnew ListViewItem(gcnew array<String^>{itemText, keyName, Convert::ToString(interval) });
						listViewItem->Tag = m;
						listView->Items->Add(listViewItem);
						listViewItem->Checked = checked;
					}
				}
			}
			else if (currentCtrl->GetType() == ListBox::typeid) {
				auto collection = safe_cast<ListBox^>(ctrl[0])->Items;

				for (int i = 0; i < itemCnt; i++) {
					String^ indexedListBoxItemStr = ctrlName + "Item" + i.ToString();
					String^ itemToAddStr = Convert::ToString(n->Attributes[indexedListBoxItemStr]->Value);
					collection->Add(itemToAddStr);
				}
			}



		}

		// if n has any children that are controls, deserialize them as well
		if (n->HasChildNodes && ctrl[0]->HasChildren) {
			XmlNodeList^ xnlControls = n->SelectNodes("Control");
			for each (XmlNode ^ n2 in xnlControls)
				SetControlProperties(ctrl[0], n2);
		}
	}
	catch (Exception^ ex) {
		Log::WriteLine("反序列化 \"" + n->Attributes["Name"]->Value + "\" 时发生异常: \"" + ex->Message + "\"");
	}
}

String^ MacroSystem::Settings::GetSettingsPath() {
	String^ AppDataFolder = Environment::GetFolderPath(Environment::SpecialFolder::ApplicationData);
	String^ SettingsFilePath = {};

	try {
		String^ TimelapseFolderPath = Path::Combine(AppDataFolder, "Timelapse");
		SettingsFilePath = Path::Combine(TimelapseFolderPath, "Settings.xml");
	}
	catch (Exception^ ex) {
		Log::WriteLine("生成设定档路径时发生异常: " + ex->Message);
	}

	Log::WriteLine("Timelapse 设定档路径: " + SettingsFilePath);
	return SettingsFilePath;
}
