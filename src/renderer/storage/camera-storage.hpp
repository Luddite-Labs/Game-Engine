#pragma once

#include "glm/trigonometric.hpp"
#include "misc/slot-map.hpp"
#include <glm/common.hpp>

#include <renderer/types.hpp>

using CameraStorageType = SlotMap<std::vector<data::Camera>, data::Camera>;

// camera storage
namespace CS {
// Camera
void init();
void destroy();
void setPerspectiveCamera(handle::Camera camera, float aspect_ratio,
		float fov = glm::radians(75.0f),
		float near_plane = 1.0f,
		float far_plane = 1000.0f);
handle::Camera createCamera();
void refCamera(handle::Camera camera);
void destroyCamera(handle::Camera camera);
void setOrthogonalCamera(handle::Camera camera, float xmag, float ymag,
		float near_plane, float far_plane);
float getCameraAspectRatio(handle::Camera camera);
float getCameraFOV(handle::Camera camera);
float getCameraXMag(handle::Camera camera);
float getCameraYMag(handle::Camera camera);
float getCameraNearPlane(handle::Camera camera);
float getCameraFarPlane(handle::Camera camera);
bool isCameraOrthogonal(handle::Camera camera);
void setCameraAspectRatio(handle::Camera camera, float aspect_ratio);
void setCameraFOV(handle::Camera camera, float fov);
void setCameraXMag(handle::Camera camera, float xmag);
void setCameraYMag(handle::Camera camera, float ymag);
void setCameraNearPlane(handle::Camera camera, float near_plane);
void setCameraFarPlane(handle::Camera camera, float far_plane);
void setCameraIsOrthogonal(handle::Camera camera, bool is_orthogonal);
}; // namespace CS