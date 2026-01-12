#include "gtest/gtest.h"
#include <gtest/gtest.h>
#include <iostream>
#include <misc/slot-map.hpp>

TEST(SlotMapTest, InsertTest) {
  std::vector<Handle> handles;
  SlotMap<std::vector<int>, int> map;

  for (int i = 0; i < 8; i++) {
    handles.push_back(map.insert(i));
  }

  for (int i = 0; i < map.size(); i++) {
    ASSERT_EQ(map[i], i);
  }

  for (int i = 0; i < map.size(); i++) {
    ASSERT_EQ(map.get(handles[i]), i);
  }
}

TEST(SlotMapTest, EraseTest) {
  std::vector<Handle> handles;
  SlotMap<std::vector<int>, int> map;

  for (int i = 0; i < 8; i++) {
    handles.push_back(map.insert(i));
  }
  
  for (int i = map.size() - 1; i >= 0; i -= 2) {
    map.erase(handles[i]);
    handles.erase(handles.begin() + i);
  }

  ASSERT_EQ(map.size(), 4);
}