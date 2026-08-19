#pragma once

#include "ShortcutManager.h"
#include "ShortcutHelper.h"

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

void PortalLoopAction() {
	ShortcutHelper::TogglePortalLoop();
}




void RegisterShortcuts() {
	ShortcutManager^ sm = ShortcutManager::Instance();
	sm->RegisterAction(ShortcutActionId::ClickTeleport, gcnew ShortcutAction(ClickTeleportAction));
	sm->RegisterAction(ShortcutActionId::MouseTeleport, gcnew ShortcutAction(MouseTeleportAction));
	sm->RegisterAction(ShortcutActionId::MouseFly, gcnew ShortcutAction(MouseFlyAction));
	sm->RegisterAction(ShortcutActionId::SwimInAir, gcnew ShortcutAction(SwimInAirAction));
	sm->RegisterAction(ShortcutActionId::PortalLoop, gcnew ShortcutAction(PortalLoopAction));

}

