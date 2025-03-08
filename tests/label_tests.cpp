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

TEST(LabelTest, RemoveDuplicateHubsWithDuplicates) {
  Label label;
  label.add(2, 10);
  label.add(2, 5);
  label.add(3, 7);
  label.add(2, 8);
  label.add(3, 6);
  label.add(4, 9);

  label.sort();

  label.removeDuplicateHubs();

  ASSERT_EQ(label.size(), 3u);
  EXPECT_EQ(label.getHub(0), 2);
  EXPECT_EQ(label.getDist(0), 5);
  EXPECT_EQ(label.getHub(1), 3);
  EXPECT_EQ(label.getDist(1), 6);
  EXPECT_EQ(label.getHub(2), 4);
  EXPECT_EQ(label.getDist(2), 9);
}

TEST(LabelTest, RemoveDuplicateHubsNoDuplicates) {
  Label label;
  label.add(1, 10);
  label.add(2, 20);

  label.sort();
  label.removeDuplicateHubs();

  ASSERT_EQ(label.size(), 2u);
  EXPECT_EQ(label.getHub(0), 1);
  EXPECT_EQ(label.getDist(0), 10);
  EXPECT_EQ(label.getHub(1), 2);
  EXPECT_EQ(label.getDist(1), 20);
}

TEST(LabelBitParallelLabelsQueryTest, SubQuery) {
  Label left, right;
  left.add(1, 5);
  left.add(2, 10);
  left.add(3, 13);
  right.add(2, 7);
  right.add(3, 1);
  EXPECT_EQ(sub_query(left, right, 11), 10 + 7);
}

TEST(LabelBitParallelLabelsQueryTest, MultipleCommonHubs) {
  Label left, right;
  left.add(1, 5);
  left.add(2, 10);
  left.add(3, 20);
  right.add(1, 7);
  right.add(2, 8);
  right.add(3, 15);
  EXPECT_EQ(query(left, right), 5 + 7);
}

TEST(BitParallelLabelsTest, AddAndGetTest) {
  BitParallelLabels<> labels;
  labels.reserve(10);
  labels.add(1, 10, 0x0F, 0xF0);
  labels.add(2, 20, 0xAA, 0x55);

  EXPECT_EQ(labels.getHub(0), 1);
  EXPECT_EQ(labels.getDist(0), 10);
  EXPECT_EQ(labels.getBitSetS_1(0), 0x0F);
  EXPECT_EQ(labels.getBitSetS_0(0), 0xF0);

  EXPECT_EQ(labels.getHub(1), 2);
  EXPECT_EQ(labels.getDist(1), 20);
  EXPECT_EQ(labels.getBitSetS_1(1), 0xAA);
  EXPECT_EQ(labels.getBitSetS_0(1), 0x55);
}

TEST(BitParallelLabelsTest, ClearTest) {
  BitParallelLabels<> labels;
  labels.add(1, 10, 0x0F, 0xF0);
  labels.clear();
  EXPECT_EQ(labels.size(), 0);
}

TEST(BitParallelLabelsTest, SortAndRemoveDuplicatesTest) {
  BitParallelLabels<> labels;
  labels.add(3, 30, 0x0F, 0xF0);
  labels.add(1, 10, 0xAA, 0x55);
  labels.add(2, 20, 0x55, 0xAA);

  labels.sort();
  EXPECT_EQ(labels.getHub(0), 1);
  EXPECT_EQ(labels.getHub(1), 2);
  EXPECT_EQ(labels.getHub(2), 3);

  labels.add(3, 25, 0xFF, 0x00);
  labels.sort();
  labels.removeDuplicateHubs();
  EXPECT_EQ(labels.size(), 3);

  EXPECT_EQ(labels.getDist(2), 25);
}

TEST(BitParallelLabelsTest, OrBitsetTest) {
  BitParallelLabels<> labels;
  labels.add(1, 10, 0x0F, 0xF0);
  labels.orBitsetS_1(0, 0xF0);
  labels.orBitsetS_0(0, 0x0F);
  EXPECT_EQ(labels.getBitSetS_1(0), 0xFF);
  EXPECT_EQ(labels.getBitSetS_0(0), 0xFF);
}

TEST(BitParallelLabelsQueryTest, NoCommonHub) {
  BitParallelLabels<> left;
  BitParallelLabels<> right;

  left.add(1, 10, 0x00, 0x00);
  left.add(3, 20, 0x00, 0x00);
  left.add(5, 30, 0x00, 0x00);

  right.add(2, 5, 0x00, 0x00);
  right.add(4, 15, 0x00, 0x00);
  right.add(6, 25, 0x00, 0x00);

  left.sort();
  right.sort();

  Distance res = query(left, right);
  EXPECT_EQ(res, infinity);
}

TEST(BitParallelLabelsQueryTest, CommonHubSubtractTwo) {
  BitParallelLabels<> left;
  BitParallelLabels<> right;

  left.add(3, 10, 0xFF, 0x00);
  right.add(3, 5, 0xFF, 0x00);

  left.sort();
  right.sort();

  EXPECT_EQ(query(left, right), 13);
}

TEST(BitParallelLabelsQueryTest, CommonHubSubtractOne) {
  BitParallelLabels<> left;
  BitParallelLabels<> right;

  left.add(3, 10, 0x00, 0xFF);
  right.add(3, 5, 0xFF, 0x00);

  left.sort();
  right.sort();

  EXPECT_EQ(query(left, right), 14);
}

TEST(BitParallelLabelsQueryTest, MultipleCommonHubs) {
  BitParallelLabels<> left;
  BitParallelLabels<> right;

  left.add(2, 10, 0xFF, 0x00);
  left.add(4, 20, 0x00, 0xFF);

  right.add(2, 5, 0xFF, 0x00);
  right.add(4, 15, 0x00, 0xFF);

  left.sort();
  right.sort();

  EXPECT_EQ(query(left, right), 13);
}