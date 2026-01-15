#include <renderer/storage/camera-storage.hpp>

namespace {
CameraStorageType camera_storage;
};

// camera storage
namespace CS {
// Camera
void init() {}

void destroy() {}

void setPerspectiveCamera(RE::Camera::Handle camera, float aspect_ratio,
		float fov,
		float near_plane,
		float far_plane) {
	RE::Camera::Data &camera_data = camera_storage.get(camera);
	camera_data.aspect_ratio = aspect_ratio;
	camera_data.fov = fov;
	camera_data.near_plane = near_plane;
	camera_data.far_plane = far_plane;
	camera_data.is_orthogonal = false;
	camera_storage.setIsEdited(camera);
}
RE::Camera::Handle createCamera() {
	RE::Camera::Handle camera = camera_storage.insert({});
	setPerspectiveCamera(camera, 1.77);
	return camera;
}
void refCamera(RE::Camera::Handle camera) {
	camera_storage.ref(camera);
}
void destroyCamera(RE::Camera::Handle camera) {
	camera_storage.erase(camera);
}
void setOrthogonalCamera(RE::Camera::Handle camera, float xmag, float ymag,
		float near_plane, float far_plane) {
	RE::Camera::Data &camera_data = camera_storage.get(camera);
	camera_data.xmag = xmag;
	camera_data.ymag = ymag;
	camera_data.near_plane = near_plane;
	camera_data.far_plane = far_plane;
	camera_data.is_orthogonal = true;
	camera_storage.setIsEdited(camera);
}
float getCameraAspectRatio(RE::Camera::Handle camera) {
	RE::Camera::Data camera_data = camera_storage.get(camera);
	return camera_data.aspect_ratio;
}
float getCameraFOV(RE::Camera::Handle camera) {
	RE::Camera::Data camera_data = camera_storage.get(camera);
	return camera_data.fov;
}
float getCameraXMag(RE::Camera::Handle camera) {
	RE::Camera::Data camera_data = camera_storage.get(camera);
	return camera_data.xmag;
}
float getCameraYMag(RE::Camera::Handle camera) {
	RE::Camera::Data camera_data = camera_storage.get(camera);
	return camera_data.ymag;
}
float getCameraNearPlane(RE::Camera::Handle camera) {
	RE::Camera::Data camera_data = camera_storage.get(camera);
	return camera_data.near_plane;
}
float getCameraFarPlane(RE::Camera::Handle camera) {
	RE::Camera::Data camera_data = camera_storage.get(camera);
	return camera_data.far_plane;
}
bool isCameraOrthogonal(RE::Camera::Handle camera) {
	RE::Camera::Data camera_data = camera_storage.get(camera);
	return camera_data.is_orthogonal;
}
void setCameraAspectRatio(RE::Camera::Handle camera, float aspect_ratio) {
	RE::Camera::Data &camera_data = camera_storage.get(camera);
	camera_data.aspect_ratio = aspect_ratio;
	camera_storage.setIsEdited(camera);
}
void setCameraFOV(RE::Camera::Handle camera, float fov) {
	RE::Camera::Data &camera_data = camera_storage.get(camera);
	camera_data.fov = fov;
	camera_storage.setIsEdited(camera);
}
void setCameraXMag(RE::Camera::Handle camera, float xmag) {
	RE::Camera::Data &camera_data = camera_storage.get(camera);
	camera_data.xmag = xmag;
	camera_storage.setIsEdited(camera);
}
void setCameraYMag(RE::Camera::Handle camera, float ymag) {
	RE::Camera::Data &camera_data = camera_storage.get(camera);
	camera_data.ymag = ymag;
	camera_storage.setIsEdited(camera);
}
void setCameraNearPlane(RE::Camera::Handle camera, float near_plane) {
	RE::Camera::Data &camera_data = camera_storage.get(camera);
	camera_data.near_plane = near_plane;
	camera_storage.setIsEdited(camera);
}
void setCameraFarPlane(RE::Camera::Handle camera, float far_plane) {
	RE::Camera::Data &camera_data = camera_storage.get(camera);
	camera_data.far_plane = far_plane;
	camera_storage.setIsEdited(camera);
}
void setCameraIsOrthogonal(RE::Camera::Handle camera, bool is_orthogonal) {
	RE::Camera::Data &camera_data = camera_storage.get(camera);
	camera_data.is_orthogonal = is_orthogonal;
	camera_storage.setIsEdited(camera);
}
bool isValid(RE::Camera::Handle camera){
	return CameraStorageType::isValid(camera);
}
}; // namespace CS