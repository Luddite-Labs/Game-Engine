#include <renderer/storage/light-storage.hpp>

namespace {
    LightStorageType light_storage;
};

// camera storage
namespace LS {
// Camera
void init() {}

void destroy() {}

void setPosition(RE::Light::Handle light, const glm::vec3 &position){
    RE::Light::Data &light_data = light_storage.get(light);
	light_data.position = position;
    light_storage.setIsEdited(light);
}
void setColor(RE::Light::Handle light, const glm::vec3 &color){
    RE::Light::Data &light_data = light_storage.get(light);
	light_data.color = color;
    light_storage.setIsEdited(light);
}
glm::vec3 getPosition(RE::Light::Handle light){
    RE::Light::Data &light_data = light_storage.get(light);
	return light_data.position;
}
glm::vec3 getColor(RE::Light::Handle light) {
    RE::Light::Data &light_data = light_storage.get(light);
	return light_data.color;
}
RE::Light::Handle createLight() {
    RE::Light::Handle light = light_storage.insert({});
    return light;
}
void refLight(RE::Light::Handle light){
    light_storage.ref(light);
}
void destroyLight(RE::Light::Handle light){
    light_storage.erase(light);
}
bool isValid(RE::Light::Handle camera) {
	return light_storage.isValid(camera);
}
}; // namespace CS