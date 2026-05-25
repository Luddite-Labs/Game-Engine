#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <renderer/storage/light-storage.hpp>

namespace {
LightStorageType light_storage;
SDL_GPUDevice *m_GPU_device;
}; // namespace

// camera storage
namespace LS {
// Camera
void drawLightDebugUI(RE::Light::Data &light_data) {
	ImGui::PushID(reinterpret_cast<size_t>(&light_data));
	if (ImGui::CollapsingHeader(("Light - " + std::to_string(reinterpret_cast<size_t>(&light_data))).c_str())) {
		ImGui::Text("Type: %s", getString(light_data.type));
		switch (light_data.type) {
			case RE::Light::DIRECTIONAL:
				ImGui::InputFloat3("Direction", glm::value_ptr(light_data.direction));
				break;
			case RE::Light::POINT:
				ImGui::InputFloat3("Position", glm::value_ptr(light_data.position));
				break;
		}
		ImGui::ColorEdit3("Color", glm::value_ptr(light_data.color));
	}
	ImGui::PopID();
}
void drawLightDebugUI(RE::Light::Handle &light_handle) {
	drawLightDebugUI(light_storage.get(light_handle));
}

void init(SDL_GPUDevice *device) {
	m_GPU_device = device;
	// A default light
	createLight();
	registerUIDebugCallback("light-storage", [&]() {
		if (ImGui::Button("Add")) {
			createLight();
		}
		ImGui::TextUnformatted(("Light count:" + std::to_string(light_storage.size())).c_str());
		for (int i = 0; i < light_storage.size(); i++) {
			drawLightDebugUI(light_storage[i]);
		}
	});
}

void destroy() {}

void setDirection(RE::Light::Handle light, glm::vec3 direction) {
	RE::Light::Data &light_data = light_storage.get(light);
	light_data.direction = direction;
	light_storage.setIsEdited(light);
}
void setPosition(RE::Light::Handle light, const glm::vec3 &position) {
	RE::Light::Data &light_data = light_storage.get(light);
	light_data.position = position;
	light_storage.setIsEdited(light);
}
void setColor(RE::Light::Handle light, const glm::vec3 &color) {
	RE::Light::Data &light_data = light_storage.get(light);
	light_data.color = color;
	light_storage.setIsEdited(light);
}
glm::vec3 getPosition(RE::Light::Handle light) {
	RE::Light::Data &light_data = light_storage.get(light);
	return light_data.position;
}
glm::vec3 getColor(RE::Light::Handle light) {
	RE::Light::Data &light_data = light_storage.get(light);
	return light_data.color;
}
glm::vec3 getDirection(RE::Light::Handle light) {
	RE::Light::Data &light_data = light_storage.get(light);
	return light_data.direction;
}
std::span<RE::Light::Data> getLightBuffer() {
	return { &light_storage[0], light_storage.size() };
}
RE::Light::Handle createLight() {
	RE::Light::Handle light = light_storage.insert({ .direction = { 1.f, 0.f, 0.f }, .color = { 1.f, 1.f, 1.f }, .type = RE::Light::DIRECTIONAL });
	return light;
}
void refLight(RE::Light::Handle light) {
	light_storage.ref(light);
}
void destroyLight(RE::Light::Handle light) {
	light_storage.erase(light);
}
bool isValid(RE::Light::Handle camera) {
	return light_storage.isValid(camera);
}
}; // namespace LS