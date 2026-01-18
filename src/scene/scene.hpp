#pragma once

#include "entt/entity/fwd.hpp"
#include <entt/entity/registry.hpp>
#include <variant>
#include <vector>

class Scene {
public:
	// all buffers are GPU buffers. CPU buffers discarded after load
	entt::entity active_camera_node;
	//! sort according BFS layout
	entt::registry nodes;
	std::string name;

	Scene() : name(), nodes(), active_camera_node() {}

	Scene(Scene &&other) : name(other.name), nodes(std::move(other.nodes)), active_camera_node(other.active_camera_node) {
  }
	void operator=(Scene &&other) {
		name = other.name;
    nodes= std::move(other.nodes);
		active_camera_node = other.active_camera_node;
	}
};