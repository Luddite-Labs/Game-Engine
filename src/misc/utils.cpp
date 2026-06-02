#include <misc/utils.hpp>
#include <unordered_map>
#include <vector>
#include <imgui.h>

static std::string status_message = "";
std::string getStatusMessage() {
	return status_message;
}

void setStatusMessage(std::string message) {
	status_message = message;
}

static std::unordered_map<std::string, std::function<void()>> ui_debug_callback_map;
void registerUIDebugCallback(std::string name, std::function<void()> callback) {
	if (ui_debug_callback_map.find(name) == ui_debug_callback_map.end()) {
		ui_debug_callback_map[name] = callback;
	}
}

void replaceUIDebugCallback(std::string name, std::function<void()> callback) {
	ui_debug_callback_map[name] = callback;
}

std::vector<std::string> getRegisteredUIDebugCallbacks() {
	std::vector<std::string> keys;
	for (const auto &pair : ui_debug_callback_map) {
		keys.push_back(pair.first);
	}
	return std::move(keys);
}

std::function<void()> getUIDebugCallbackByName(std::string name) {
	return ui_debug_callback_map[name];
}

