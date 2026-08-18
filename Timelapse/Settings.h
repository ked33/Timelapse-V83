#pragma once
using namespace System;
using namespace System::Windows::Forms;
using namespace Xml;
using namespace Serialization;
using namespace Collections::Generic;

namespace MacroSystem {
	ref class Macro; // Forward declaration of Macro as a reference class.

	public ref class Settings {
	public:
		static Object^ Deserialize(String^ Path, XmlSerializer^ Deserializer);
		static void Deserialize(Control^ c, String^ Path);
		static void Serialize(String^ Path, XmlSerializer^ Serializer, Object^ Collection);
		static void Serialize(Control^ c, String^ Path);
		static bool isExcluded(Control^ ctrl);
		static String^ GetSettingsPath();

	private:
		static void AddChildControls(XmlTextWriter^ xmlSerialisedForm, Control^ c);
		static void AddShortcutSettings(XmlTextWriter^ xmlSerializedForm);
		static void LoadShortcutSettings(XmlNode^ shortcutsNode);
		static void SetControlProperties(Control^ currentCtrl, XmlNode^ n);
	};

	ref struct MacroData {
		int keyCode;
		String^ keyName;
		int interval;
		Macro^ macro;
		MacroData(int keyCode, int interval, String^ keyName) {
			this->keyCode = keyCode;
			this->interval = interval;
			this->keyName = keyName;
		}
	};
}
