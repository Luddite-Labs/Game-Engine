#include "gtest/gtest.h"
#include <gtest/gtest.h>
#include <iostream>
#include <misc/slot-map.hpp>

TEST(SlotMapTest, BasicAssertions) {
  std::vector<Handle> handles;
  SlotMap<std::vector<int>, int> map;

  for (int i = 0; i < map.size(); i++) {
    handles.push_back(map.insert(i));
  }

  for (int i = 0; i < map.size(); i++) {
    ASSERT_EQ(map[i], i);
  }

  for (int i = 0; i < map.size(); i++) {
    ASSERT_EQ(map.get(handles[i]), i);
  }
}