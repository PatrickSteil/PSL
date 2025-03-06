#include <gtest/gtest.h>

#include "../datastructures/hub_labels.h"

TEST(LabelTest, DefaultConstructor) {
  Label label;
  EXPECT_EQ(label.size(), 0);
}

TEST(LabelTest, AddAndAccess) {
  Label label;
  label.add(1, 10);
  label.add(2, 20);

  EXPECT_EQ(label.size(), 2);
  EXPECT_EQ(label.getHub(0), 1);
  EXPECT_EQ(label.getDist(0), static_cast<Distance>(10));
  EXPECT_EQ(label.getHub(1), 2);
  EXPECT_EQ(label.getDist(1), static_cast<Distance>(20));
}

TEST(LabelTest, CopyConstructor) {
  Label label1;
  label1.add(1, 10);
  label1.add(2, 20);

  Label label2 = label1;
  EXPECT_EQ(label2.size(), 2);
  EXPECT_EQ(label2.getHub(0), 1);
  EXPECT_EQ(label2.getDist(0), static_cast<Distance>(10));
  EXPECT_EQ(label2.getHub(1), 2);
  EXPECT_EQ(label2.getDist(1), static_cast<Distance>(20));
}

TEST(LabelTest, MoveConstructor) {
  Label label1;
  label1.add(1, 10);
  label1.add(2, 20);

  Label label2 = std::move(label1);
  EXPECT_EQ(label2.size(), 2);
  EXPECT_EQ(label2.getHub(0), 1);
  EXPECT_EQ(label2.getDist(0), static_cast<Distance>(10));
  EXPECT_EQ(label2.getHub(1), 2);
  EXPECT_EQ(label2.getDist(1), static_cast<Distance>(20));
}

TEST(LabelTest, Sort) {
  Label label;
  label.add(3, 30);
  label.add(1, 10);
  label.add(2, 20);

  label.sort();

  EXPECT_EQ(label.getHub(0), 1);
  EXPECT_EQ(label.getDist(0), static_cast<Distance>(10));
  EXPECT_EQ(label.getHub(1), 2);
  EXPECT_EQ(label.getDist(1), static_cast<Distance>(20));
  EXPECT_EQ(label.getHub(2), 3);
  EXPECT_EQ(label.getDist(2), static_cast<Distance>(30));
}

TEST(LabelTest, Clear) {
  Label label;
  label.add(1, 10);
  label.add(2, 20);
  label.clear();
  EXPECT_EQ(label.size(), 0);
}

TEST(LabelTest, Reserve) {
  Label label;
  label.reserve(10);
  EXPECT_GE(label.capacity(), 10);
}

TEST(LabelTest, Contains) {
  Label label;
  label.add(1, 10);
  label.add(2, 20);
  EXPECT_TRUE(label.contains(1));
  EXPECT_TRUE(label.contains(2));
  EXPECT_FALSE(label.contains(3));
}

TEST(LabelTest, DoForAll) {
  Label label;
  label.add(1, 10);
  label.add(2, 20);
  int sumHubs = 0;
  int sumDists = 0;
  label.doForAll([&](const Vertex hub, const Distance dist) {
    sumHubs += hub;
    sumDists += dist;
  });
  EXPECT_EQ(sumHubs, 3);
  EXPECT_EQ(sumDists, 30);
}

TEST(LabelQueryTest, SubQuery) {
  Label left, right;
  left.add(1, 5);
  left.add(2, 10);
  left.add(3, 13);
  right.add(2, 7);
  right.add(3, 1);
  EXPECT_EQ(sub_query(left, right, 11), 10 + 7);
}

TEST(LabelQueryTest, MultipleCommonHubs) {
  Label left, right;
  left.add(1, 5);
  left.add(2, 10);
  left.add(3, 20);
  right.add(1, 7);
  right.add(2, 8);
  right.add(3, 15);
  EXPECT_EQ(query(left, right), 5 + 7);
}
