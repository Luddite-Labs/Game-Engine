#include <renderer/storage/camera-storage.hpp>

namespace {
CameraStorageType camera_storage;
};

// camera storage
namespace CS {
// Camera
void init() {}

void destroy() {}

void setPerspectiveCamera(handle::Camera camera, float aspect_ratio,
		float fov,
		float near_plane,
		float far_plane) {
	data::Camera &camera_data = camera_storage.get(camera);
	camera_data.aspect_ratio = aspect_ratio;
	camera_data.fov = fov;
	camera_data.near_plane = near_plane;
	camera_data.far_plane = far_plane;
	camera_data.is_orthogonal = false;
	camera_storage.setIsEdited(camera);
}
handle::Camera createCamera() {
	handle::Camera camera = camera_storage.insert({});
	setPerspectiveCamera(camera, 1.77);
	return camera;
}
void refCamera(handle::Camera camera) {
	camera_storage.ref(camera);
}
void destroyCamera(handle::Camera camera) {
	camera_storage.erase(camera);
}
void setOrthogonalCamera(handle::Camera camera, float xmag, float ymag,
		float near_plane, float far_plane) {
	data::Camera &camera_data = camera_storage.get(camera);
	camera_data.xmag = xmag;
	camera_data.ymag = ymag;
	camera_data.near_plane = near_plane;
	camera_data.far_plane = far_plane;
	camera_data.is_orthogonal = true;
	camera_storage.setIsEdited(camera);
}
float getCameraAspectRatio(handle::Camera camera) {
	data::Camera camera_data = camera_storage.get(camera);
	return camera_data.aspect_ratio;
}
float getCameraFOV(handle::Camera camera) {
	data::Camera camera_data = camera_storage.get(camera);
	return camera_data.fov;
}
float getCameraXMag(handle::Camera camera) {
	data::Camera camera_data = camera_storage.get(camera);
	return camera_data.xmag;
}
float getCameraYMag(handle::Camera camera) {
	data::Camera camera_data = camera_storage.get(camera);
	return camera_data.ymag;
}
float getCameraNearPlane(handle::Camera camera) {
	data::Camera camera_data = camera_storage.get(camera);
	return camera_data.near_plane;
}
float getCameraFarPlane(handle::Camera camera) {
	data::Camera camera_data = camera_storage.get(camera);
	return camera_data.far_plane;
}
bool isCameraOrthogonal(handle::Camera camera) {
	data::Camera camera_data = camera_storage.get(camera);
	return camera_data.is_orthogonal;
}
void setCameraAspectRatio(handle::Camera camera, float aspect_ratio) {
	data::Camera &camera_data = camera_storage.get(camera);
	camera_data.aspect_ratio = aspect_ratio;
	camera_storage.setIsEdited(camera);
}
void setCameraFOV(handle::Camera camera, float fov) {
	data::Camera &camera_data = camera_storage.get(camera);
	camera_data.fov = fov;
	camera_storage.setIsEdited(camera);
}
void setCameraXMag(handle::Camera camera, float xmag) {
	data::Camera &camera_data = camera_storage.get(camera);
	camera_data.xmag = xmag;
	camera_storage.setIsEdited(camera);
}
void setCameraYMag(handle::Camera camera, float ymag) {
	data::Camera &camera_data = camera_storage.get(camera);
	camera_data.ymag = ymag;
	camera_storage.setIsEdited(camera);
}
void setCameraNearPlane(handle::Camera camera, float near_plane) {
	data::Camera &camera_data = camera_storage.get(camera);
	camera_data.near_plane = near_plane;
	camera_storage.setIsEdited(camera);
}
void setCameraFarPlane(handle::Camera camera, float far_plane) {
	data::Camera &camera_data = camera_storage.get(camera);
	camera_data.far_plane = far_plane;
	camera_storage.setIsEdited(camera);
}
void setCameraIsOrthogonal(handle::Camera camera, bool is_orthogonal) {
	data::Camera &camera_data = camera_storage.get(camera);
	camera_data.is_orthogonal = is_orthogonal;
	camera_storage.setIsEdited(camera);
}
}; // namespace CS