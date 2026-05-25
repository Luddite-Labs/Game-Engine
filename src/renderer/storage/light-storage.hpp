#pragma once

#include "glm/trigonometric.hpp"
#include "misc/slot-map.hpp"
#include <glm/common.hpp>
#include <renderer/types.hpp>
#include <span>

using LightStorageType = SlotMap<std::vector<RE::Light::Data>, RE::Light::Data, RE::Light::Handle>;

// light storage
namespace LS {
// Light
void drawLightDebugUI(RE::Light::Data &light_data);
void drawLightDebugUI(RE::Light::Handle &light_handle);
void init(SDL_GPUDevice *device);
void destroy();
void setPosition(RE::Light::Handle light, const glm::vec3 &position);
void setColor(RE::Light::Handle light, const glm::vec3 &color);
void setDirection(RE::Light::Handle light, glm::vec3 direction);
glm::vec3 getPosition(RE::Light::Handle light);
glm::vec3 getColor(RE::Light::Handle light);
glm::vec3 getDirection(RE::Light::Handle light);
std::span<RE::Light::Data> getLightBuffer();
RE::Light::Handle createLight();
void refLight(RE::Light::Handle light);
void destroyLight(RE::Light::Handle light);
bool isValid(RE::Light::Handle light);
}; // namespace LS