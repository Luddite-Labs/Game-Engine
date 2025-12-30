
#pragma once
#include <renderer/common.hpp>
#include <renderer/interface.hpp>
#include <variant>
#include <vector>

struct Node {
  glm::mat4x4 transform;
  interface::Mesh mesh;
  interface::Material material;
  uint32_t parent_id;
  uint32_t node_id;
  uint32_t depth;
  bool is_disabled;
};

class Scene {
public:
  // all buffers are GPU buffers. CPU buffers discarded after load
  std::vector<
      std::variant<interface::PerspectiveCamera, interface::OrthogonalCamera>>
      cameras;
  uint32_t active_camera_index;
  // sorted according BFS layout
  std::vector<Node> nodes;

  Scene() {}
};