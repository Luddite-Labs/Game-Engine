#pragma once

#include "glm/trigonometric.hpp"
#include "misc/slot-map.hpp"
#include <glm/common.hpp>
#include <renderer/types.hpp>

using CameraStorageType = SlotMap<std::vector<RE::Camera::Data>, RE::Camera::Data, RE::Camera::Handle>;

// camera storage
namespace CS {
// Camera
void init();
void destroy();
void setPerspectiveCamera(RE::Camera::Handle camera, float aspect_ratio,
		float fov = glm::radians(75.0f),
		float near_plane = 1.0f,
		float far_plane = 1000.0f);
RE::Camera::Handle createCamera();
void refCamera(RE::Camera::Handle camera);
void destroyCamera(RE::Camera::Handle camera);
void setOrthogonalCamera(RE::Camera::Handle camera, float xmag, float ymag,
		float near_plane, float far_plane);
float getCameraAspectRatio(RE::Camera::Handle camera);
float getCameraFOV(RE::Camera::Handle camera);
float getCameraXMag(RE::Camera::Handle camera);
float getCameraYMag(RE::Camera::Handle camera);
float getCameraNearPlane(RE::Camera::Handle camera);
float getCameraFarPlane(RE::Camera::Handle camera);
bool isCameraOrthogonal(RE::Camera::Handle camera);
void setCameraAspectRatio(RE::Camera::Handle camera, float aspect_ratio);
void setCameraFOV(RE::Camera::Handle camera, float fov);
void setCameraXMag(RE::Camera::Handle camera, float xmag);
void setCameraYMag(RE::Camera::Handle camera, float ymag);
void setCameraNearPlane(RE::Camera::Handle camera, float near_plane);
void setCameraFarPlane(RE::Camera::Handle camera, float far_plane);
void setCameraIsOrthogonal(RE::Camera::Handle camera, bool is_orthogonal);
bool isValid(RE::Camera::Handle camera);
}; // namespace CS