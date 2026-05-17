#pragma once

#include "glm/trigonometric.hpp"
#include "misc/slot-map.hpp"
#include <glm/common.hpp>
#include <renderer/types.hpp>

using LightStorageType = SlotMap<std::vector<RE::Light::Data>, RE::Light::Data, RE::Light::Handle>;

// light storage
namespace LS {
// Light
void init();
void destroy();
void setPosition(RE::Light::Handle light, const glm::vec3 &position);
void setColor(RE::Light::Handle light, const glm::vec3 &color);
glm::vec3 getPosition(RE::Light::Handle light);
glm::vec3 getColor(RE::Light::Handle light);
RE::Light::Handle createLight();
void refLight(RE::Light::Handle light);
void destroyLight(RE::Light::Handle light);
bool isValid(RE::Light::Handle light);
}; // namespace LS