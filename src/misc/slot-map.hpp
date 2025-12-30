#pragma once

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <vector>
#include <misc/utils.hpp>

struct Slot {
  union {
    uint32_t data_index;
    uint32_t next_free_slot;
  };
  uint32_t generation;
  uint32_t ref_count;
};

template <typename T, typename D> class SlotMap {
private:
  std::vector<Slot> slots;
  T data;
  std::vector<uint32_t> index_slot_map;
  uint32_t live_node_count;
  uint32_t free_slot_count;
  uint32_t free_slot_head;
  uint32_t free_slot_tail;
  std::vector<bool> is_edited;

public:
  SlotMap()
      : slots(), data(), index_slot_map(), live_node_count(0),
        free_slot_head(0), free_slot_tail(0), free_slot_count(0), is_edited() {
    slots.reserve(256);
    data.reserve(256);
    index_slot_map.reserve(256);
    is_edited.reserve(256);
  }

  Handle insert(const D &val) {
    Handle handle;
    if (free_slot_count == 0) {
      const uint32_t data_index = data.size();
      const uint32_t slot_index = slots.size();
      slots.push_back(
          {.data_index = data_index, .generation = 0, .ref_count = 1});
      data.push_back(val);
      index_slot_map.push_back(slot_index);
      is_edited.push_back(true);
      handle.slot_index = slot_index;
      handle.generation = slots[slot_index].generation;
    } else {
      const uint32_t slot_index = free_slot_head;
      const uint32_t data_index = data.size();
      free_slot_head = slots[slot_index].next_free_slot;
      free_slot_count -= 1;
      slots[slot_index].data_index = data_index;
      slots[slot_index].generation += 1;
      slots[slot_index].ref_count = 1;
      is_edited[data_index] = true;
      data.push_back(val);
      index_slot_map.push_back(slot_index);
      handle.slot_index = slot_index;
      handle.generation = slots[slot_index].generation;
    }
    live_node_count += 1;
    return handle;
  }
  //! ref counting not working correctly
  void erase(Handle &handle) {
    const uint32_t slot_index = handle.slot_index;
    slots[slot_index].ref_count -= 1;
    if (slots[slot_index].ref_count == 0) {
      const uint32_t data_index = slots[slot_index].data_index;
      live_node_count -= 1;
      D temp = data[live_node_count];
      data[live_node_count] = data[data_index];
      data[data_index] = temp;
      slots[slot_index].data_index = live_node_count;
      std::swap(index_slot_map[live_node_count], index_slot_map[data_index]);
      bool temp_edited = is_edited[live_node_count];
      is_edited[live_node_count] = is_edited[data_index];
      is_edited[data_index] = temp_edited;
    }
  }

  void ref(Handle handle) { slots[handle.slot_index].ref_count += 1; }

  void update(std::function<void(D &val)> callback) {
    for (int i = 0; i < data.size(); i++) {
      if (is_edited[i]) {
        callback(data[i]);
      }
    }
  }
  void cleanup(std::function<void(D &val)> callback) {
    for (int i = live_node_count; i < data.size(); i++) {
      callback(data[i]);
      data.erase(data.begin() + i);
      if (free_slot_count == 0) {
      }
    }
  }
  D &get(const Handle &handle) { return data[slots[handle.slot_index].data_index]; }
  D &operator[](int index) { return data[index]; }
  uint32_t size() { return live_node_count; }

  void setIsEdited(Handle &handle) {
    is_edited[slots[handle.slot_index].data_index] = true;
  }
};