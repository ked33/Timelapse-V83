#pragma once

#include "ShortcutManager.h"

namespace Timelapse
{
	using namespace System;
	using namespace System::Collections::Generic;
	using namespace System::Drawing;
	using namespace System::Windows::Forms;

	public ref class ShortcutSettingsForm : public Form
	{
	private:
		Dictionary<ShortcutActionId, TextBox^>^ editors;
		ErrorProvider^ errorProvider;
		ToolTip^ toolTip;
		TableLayoutPanel^ shortcutTable;
		FlowLayoutPanel^ buttonPanel;
		Button^ clearAllButton;
		Button^ cancelButton;
		Button^ applyButton;

		Button^ CreateDialogButton(String^ text, int width)
		{
			Button^ button = gcnew Button();
			button->AutoSize = false;
			button->BackColor = Color::FromArgb(35, 35, 35);
			button->FlatAppearance->BorderColor = Color::FromArgb(85, 85, 85);
			button->FlatStyle = FlatStyle::Flat;
			button->ForeColor = Color::WhiteSmoke;
			button->Margin = System::Windows::Forms::Padding(6, 8, 0, 8);
			button->Size = Drawing::Size(width, 32);
			button->Text = text;
			button->UseVisualStyleBackColor = false;
			return button;
		}

		void AddShortcutRow(int row, ShortcutActionId actionId)
		{
			Label^ nameLabel = gcnew Label();
			nameLabel->Anchor = AnchorStyles::Left;
			nameLabel->AutoSize = true;
			nameLabel->ForeColor = Color::WhiteSmoke;
			nameLabel->Margin = System::Windows::Forms::Padding(0, 0, 12, 0);
			nameLabel->Text = ShortcutManager::GetActionDisplayName(actionId);

			TextBox^ editor = gcnew TextBox();
			editor->Anchor = AnchorStyles::Left | AnchorStyles::Right;
			editor->BackColor = Color::FromArgb(35, 35, 35);
			editor->BorderStyle = BorderStyle::FixedSingle;
			editor->ForeColor = Color::White;
			editor->Margin = System::Windows::Forms::Padding(0, 5, 10, 5);
			editor->MaxLength = 32;
			editor->Name = L"shortcutEditor" + actionId.ToString();
			editor->TextChanged += gcnew EventHandler(this, &ShortcutSettingsForm::ShortcutEditor_TextChanged);

			Button^ clearButton = CreateDialogButton(L"\x00D7", 32);
			clearButton->Anchor = AnchorStyles::None;
			clearButton->Margin = System::Windows::Forms::Padding(0, 4, 0, 4);
			clearButton->Name = L"clear" + actionId.ToString();
			clearButton->Tag = static_cast<int>(actionId);
			clearButton->Click += gcnew EventHandler(this, &ShortcutSettingsForm::ClearShortcut_Click);
			toolTip->SetToolTip(clearButton, L"清除此快捷键");

			shortcutTable->Controls->Add(nameLabel, 0, row);
			shortcutTable->Controls->Add(editor, 1, row);
			shortcutTable->Controls->Add(clearButton, 2, row);
			editors[actionId] = editor;
		}

		void InitializeComponent()
		{
			editors = gcnew Dictionary<ShortcutActionId, TextBox^>();
			errorProvider = gcnew ErrorProvider();
			toolTip = gcnew ToolTip();
			shortcutTable = gcnew TableLayoutPanel();
			buttonPanel = gcnew FlowLayoutPanel();
			clearAllButton = CreateDialogButton(L"全部清除", 88);
			cancelButton = CreateDialogButton(L"取消", 76);
			applyButton = CreateDialogButton(L"应用", 76);

			SuspendLayout();

			shortcutTable->ColumnCount = 3;
			shortcutTable->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 170.0F));
			shortcutTable->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0F));
			shortcutTable->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 36.0F));
			shortcutTable->Dock = DockStyle::Fill;
			shortcutTable->Margin = System::Windows::Forms::Padding(0);
			shortcutTable->Padding = System::Windows::Forms::Padding(18, 12, 18, 4);
			shortcutTable->RowCount = 5;
			shortcutTable->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 30.0F));
			for (int row = 1; row <= 4; ++row)
				shortcutTable->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 25.0F));

			Label^ functionHeader = gcnew Label();
			functionHeader->Anchor = AnchorStyles::Left;
			functionHeader->AutoSize = true;
			functionHeader->ForeColor = Color::Silver;
			functionHeader->Text = L"功能";

			Label^ shortcutHeader = gcnew Label();
			shortcutHeader->Anchor = AnchorStyles::Left;
			shortcutHeader->AutoSize = true;
			shortcutHeader->ForeColor = Color::Silver;
			shortcutHeader->Text = L"快捷键";

			shortcutTable->Controls->Add(functionHeader, 0, 0);
			shortcutTable->Controls->Add(shortcutHeader, 1, 0);
			AddShortcutRow(1, ShortcutActionId::ClickTeleport);
			AddShortcutRow(2, ShortcutActionId::MouseFly);
			AddShortcutRow(3, ShortcutActionId::MouseTeleport);
			AddShortcutRow(4, ShortcutActionId::SwimInAir);

			buttonPanel->AutoSize = false;
			buttonPanel->Dock = DockStyle::Bottom;
			buttonPanel->FlowDirection = FlowDirection::RightToLeft;
			buttonPanel->Height = 52;
			buttonPanel->Padding = System::Windows::Forms::Padding(12, 0, 18, 0);
			buttonPanel->WrapContents = false;

			applyButton->Click += gcnew EventHandler(this, &ShortcutSettingsForm::ApplyButton_Click);
			cancelButton->DialogResult = System::Windows::Forms::DialogResult::Cancel;
			clearAllButton->Click += gcnew EventHandler(this, &ShortcutSettingsForm::ClearAllButton_Click);
			buttonPanel->Controls->Add(applyButton);
			buttonPanel->Controls->Add(cancelButton);
			buttonPanel->Controls->Add(clearAllButton);

			errorProvider->BlinkStyle = ErrorBlinkStyle::NeverBlink;
			errorProvider->ContainerControl = this;

			AcceptButton = applyButton;
			AutoScaleDimensions = SizeF(9.0F, 18.0F);
			AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			BackColor = Color::FromArgb(25, 25, 25);
			CancelButton = cancelButton;
			ClientSize = Drawing::Size(500, 300);
			Controls->Add(shortcutTable);
			Controls->Add(buttonPanel);
			Font = gcnew Drawing::Font(L"Microsoft YaHei UI", 9.0F, FontStyle::Regular, GraphicsUnit::Point, 134);
			FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
			MaximizeBox = false;
			MinimizeBox = false;
			Name = L"ShortcutSettingsForm";
			ShowIcon = false;
			ShowInTaskbar = false;
			StartPosition = FormStartPosition::CenterParent;
			Text = L"移动快捷键设置";

			ResumeLayout(false);
		}

		void LoadBindings()
		{
			ShortcutManager^ manager = ShortcutManager::Instance();
			for each (ShortcutActionId actionId in ShortcutManager::GetConfigurableActions())
				editors[actionId]->Text = manager->FormatBinding(manager->GetBinding(actionId));
		}

		void ShortcutEditor_TextChanged(Object^ sender, EventArgs^ e)
		{
			Control^ editor = safe_cast<Control^>(sender);
			errorProvider->SetError(editor, String::Empty);
		}

		void ClearShortcut_Click(Object^ sender, EventArgs^ e)
		{
			Button^ button = safe_cast<Button^>(sender);
			ShortcutActionId actionId = static_cast<ShortcutActionId>(safe_cast<int>(button->Tag));
			editors[actionId]->Clear();
			editors[actionId]->Focus();
		}

		void ClearAllButton_Click(Object^ sender, EventArgs^ e)
		{
			for each (TextBox^ editor in editors->Values)
				editor->Clear();
		}

		void ApplyButton_Click(Object^ sender, EventArgs^ e)
		{
			ShortcutManager^ manager = ShortcutManager::Instance();
			auto proposedBindings = gcnew Dictionary<ShortcutActionId, ShortcutBinding^>();
			bool hasErrors = false;

			for each (ShortcutActionId actionId in ShortcutManager::GetConfigurableActions())
			{
				ShortcutBinding^ binding;
				String^ errorMessage;
				TextBox^ editor = editors[actionId];
				if (!manager->TryParseBinding(actionId, editor->Text, binding, errorMessage))
				{
					errorProvider->SetError(editor, errorMessage);
					hasErrors = true;
					continue;
				}

				proposedBindings[actionId] = binding;
				editor->Text = manager->FormatBinding(binding);
			}

			if (hasErrors)
				return;

			String^ conflictMessage;
			if (!manager->ValidateBindings(proposedBindings, conflictMessage))
			{
				MessageBox::Show(this, conflictMessage, L"快捷键冲突", MessageBoxButtons::OK, MessageBoxIcon::Warning);
				return;
			}

			manager->ApplyBindings(proposedBindings);
			DialogResult = System::Windows::Forms::DialogResult::OK;
			Close();
		}

	public:
		ShortcutSettingsForm()
		{
			InitializeComponent();
			LoadBindings();
		}
	};
}
