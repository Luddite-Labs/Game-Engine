#pragma once

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <misc/log.hpp>
#include <misc/utils.hpp>
#include <vector>

#ifdef _MSC_VER
#define SLOT_MAP_FUNC_PRINT __FUNCTION__ // or __FUNCSIG__
#elif __GNUC__
#define SLOT_MAP_FUNC_PRINT __PRETTY_FUNCTION__
#else 
#define SLOT_MAP_FUNC_PRINT ""
#endif
#define CHUNK_SIZE 4096
struct Slot {
	union {
		uint32_t data_index;
		uint32_t next_free_slot;
	};
	uint32_t generation;
	uint32_t ref_count;
};

template <typename T, typename D, typename C>
class SlotMap {
private:
	std::vector<Slot> slots;
	T data;
	std::vector<uint32_t> index_slot_map;
	uint32_t live_node_count;
	uint32_t dead_node_count;
	uint32_t free_slot_count;
	uint32_t free_slot_head;
	uint32_t free_slot_tail;
	std::vector<bool> is_edited;

	void freeSlot(uint32_t slot_index) {
		free_slot_count += 1;
		if (free_slot_count == 1) {
			free_slot_head = slot_index;
			free_slot_tail = slot_index;
		} else {
			slots[free_slot_tail].next_free_slot = slot_index;
			free_slot_tail = slot_index;
		}
	}

	uint32_t getFreeSlotIndex() {
		if (free_slot_count == 0) {
			return live_node_count;
		} else {
			auto free_index = free_slot_head;
			free_slot_count -= 1;
			if (free_slot_count != 0) {
				free_slot_head = slots[free_index].next_free_slot;
			}
			return free_index;
		}
	}

	void swapIndices(uint32_t live_index, uint32_t dead_index) {
		D temp = data[dead_index];
		data[dead_index] = data[live_index];
		data[live_index] = temp;
	}
	void swapSlotMappings(uint32_t live_index, uint32_t dead_index) {
		uint32_t slot_index = index_slot_map[live_index];
		slots[index_slot_map[live_index]].data_index = dead_index;
		slots[index_slot_map[dead_index]].data_index = live_index;
		index_slot_map[live_index] = index_slot_map[dead_index];
		index_slot_map[dead_index] = slot_index;
	}

	void setData(uint32_t slot_index, uint32_t data_index, const D &val) {
		slots[slot_index].data_index = data_index;
		slots[slot_index].generation = std::max(slots[slot_index].generation, 1U);
		slots[slot_index].ref_count = 1 ;
		data[data_index] = val;
		index_slot_map[data_index] = slot_index;
		is_edited[data_index] = true;
	}

public:
	SlotMap() : slots(), data(), index_slot_map(), live_node_count(0), free_slot_head(0), free_slot_tail(0), free_slot_count(0), is_edited() {
		int size = CHUNK_SIZE / sizeof(T);
		slots.resize(size);
		data.resize(size);
		index_slot_map.resize(size);
		is_edited.resize(size);
	}
	bool isValid(const C &handle) {
		return 0 <= handle.slot_index and handle.slot_index < slots.size() and handle.generation != 0 and handle.generation == slots[handle.slot_index].generation and 0 <= slots[handle.slot_index].data_index and slots[handle.slot_index].data_index < live_node_count;
	}

	C insert(const D &val) {
		if (live_node_count + dead_node_count == data.size()) {
			LOG_DEBUG("%s: unrefing slot %d reallocing new size %d", SLOT_MAP_FUNC_PRINT, data.size());
			data.resize(data.size() + CHUNK_SIZE / sizeof(T));
			slots.resize(data.size() + CHUNK_SIZE / sizeof(T));
			index_slot_map.resize(data.size() + CHUNK_SIZE / sizeof(T));
			is_edited.resize(data.size() + CHUNK_SIZE / sizeof(T));
		}
		const uint32_t data_index = live_node_count;
		const uint32_t slot_index = getFreeSlotIndex();

		swapIndices(live_node_count, live_node_count + dead_node_count);
		setData(slot_index, data_index, val);
		index_slot_map[data_index] = slot_index;

		LOG_DEBUG("%s: inserting slot %d", SLOT_MAP_FUNC_PRINT, slot_index);
		live_node_count += 1;
		return {
			.slot_index = slot_index,
			.generation = slots[slot_index].generation
		};
	}

	void erase(C &handle) {
		assert(isValid(handle));
		const uint32_t slot_index = handle.slot_index;
		slots[slot_index].ref_count -= 1;
		LOG_DEBUG("%s: unrefing slot %d new ref count %d", SLOT_MAP_FUNC_PRINT, handle.slot_index, slots[slot_index].ref_count);
		if (slots[slot_index].ref_count == 0) {
			LOG_DEBUG("%s: deleting slot %d", SLOT_MAP_FUNC_PRINT, handle.slot_index);
			slots[slot_index].generation += 1;
			live_node_count -= 1;
			dead_node_count += 1;
			const uint32_t data_index = slots[slot_index].data_index;
			freeSlot(slot_index);
			swapIndices(data_index, live_node_count);
			swapSlotMappings(data_index, live_node_count);
		}
	}

	void ref(C handle) {
		assert(isValid(handle));
		LOG_DEBUG("%s: refing slot %d", SLOT_MAP_FUNC_PRINT, handle.slot_index);
		slots[handle.slot_index].ref_count += 1;
	}

	void update(std::function<void(D &val)> callback) {
		for (int i = 0; i < data.size(); i++) {
			if (is_edited[i]) {
				callback(data[i]);
			}
		}
	}

	D &get(const C &handle) {
		assert(isValid(handle));
		return data[slots[handle.slot_index].data_index];
	}

	uint32_t size() {
		return live_node_count;
	}

	void setIsEdited(C &handle) {
		is_edited[slots[handle.slot_index].data_index] = true;
	}
};
