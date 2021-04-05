////////////////////////////////////////////////////////////////////////////////

#include "../ecs.hpp"
#include <gtest/gtest.h>

////////////////////////////////////////////////////////////////////////////////

TEST(ecs, add_remove_component) {
  ecs::scene scene;
  auto health = scene.create_component<int>();
  auto e0 = scene.spawn();
  EXPECT_EQ((int)scene.size(), 1);
  EXPECT_EQ((int)health->size(), 0);
  EXPECT_FALSE(e0->has(health));
  e0->add(health) = 42;
  EXPECT_EQ((int)health->size(), 1);
  EXPECT_TRUE(e0->has(health));
  EXPECT_EQ(e0->get(health), 42);
  e0->remove(health);
  EXPECT_EQ((int)health->size(), 0);
  EXPECT_FALSE(e0->has(health));
}

TEST(ecs, despawn) {
  ecs::scene scene;
  auto health = scene.create_component<int>();
  auto e0 = scene.spawn();
  EXPECT_EQ((int)scene.size(), 1);
  EXPECT_EQ((int)health->size(), 0);
  EXPECT_FALSE(e0->has(health));
  e0->add(health) = 42;
  EXPECT_EQ((int)health->size(), 1);
  EXPECT_TRUE(e0->has(health));
  EXPECT_EQ(e0->get(health), 42);
  scene.despawn(e0);
  EXPECT_EQ((int)health->size(), 0);
  EXPECT_EQ((int)scene.size(), 0);
}

struct pos { int x; int y; };
TEST(ecs, all_with) {
  ecs::scene scene;
  auto health = scene.create_component<int>();
  auto position = scene.create_component<pos>();
  auto e0 = scene.spawn();
  auto e1 = scene.spawn();
  auto e2 = scene.spawn();
  e0->add(health) = 42;
  e1->add(health) = 23;
  e1->add(position) = { 10, -15 };
  e2->add(position) = { -5, 0 };
  int count = 0;
  for (auto e : scene.with(health, position)) {
    EXPECT_TRUE(e->has(health, position));
    EXPECT_EQ(e->get(health), 23);
    auto& p = e->get(position);
    EXPECT_EQ(p.x, 10);
    EXPECT_EQ(p.y, -15);
    count++;
  }
  EXPECT_EQ(count, 1);
}

////////////////////////////////////////////////////////////////////////////////
