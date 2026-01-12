#pragma once

#include "entt/entity/fwd.hpp"
#include <entt/entity/registry.hpp>
#include <variant>
#include <vector>

class Scene {
public:
  // all buffers are GPU buffers. CPU buffers discarded after load
  entt::entity active_camera_node;
  // sorted according BFS layout
  entt::registry nodes;
  std::string name;

  Scene() {}
};