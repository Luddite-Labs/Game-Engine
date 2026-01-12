#pragma once
#include "entt/entity/fwd.hpp"
#include "renderer/wrappers.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <optional>

struct Transform {
	glm::vec3 translate;
	glm::quat rotate;
	glm::vec3 scale;
};

struct Tag {
public:
	std::string name;
	Tag(const std::string &name) : name(name) {}
};

struct Disabled {
};

struct Parent {
	entt::entity node;
};

struct Child {
	entt::entity node;
};

struct RenderableMesh {
	interface::Mesh mesh;
};