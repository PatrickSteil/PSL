#include <gtest/gtest.h>

#include "../datastructures/bitvector_storage.h"

TEST(BitVectorStorageTest, DefaultConstructor) {
  BitVectorStorage<int> storage(100);
  EXPECT_EQ(storage.size(), 0);
  EXPECT_FALSE(storage.isMarked(0));
  EXPECT_FALSE(storage.isMarked(99));
}

TEST(BitVectorStorageTest, MarkingAndChecking) {
  BitVectorStorage<int> storage(10);
  EXPECT_FALSE(storage.isMarked(3));
  storage.mark(3);
  EXPECT_TRUE(storage.isMarked(3));
  EXPECT_FALSE(storage.isMarked(5));
}

TEST(BitVectorStorageTest, AddAndRetrieveElements) {
  BitVectorStorage<int> storage(10);
  storage.add(3);
  storage.add(5);

  EXPECT_TRUE(storage.isMarked(3));
  EXPECT_TRUE(storage.isMarked(5));
  EXPECT_EQ(storage.size(), 2);

  const auto& values = storage.getStorage();
  EXPECT_EQ(values.size(), 2);
  EXPECT_EQ(values[0], 3);
  EXPECT_EQ(values[1], 5);
}

TEST(BitVectorStorageTest, ClearStorage) {
  BitVectorStorage<int> storage(10);
  storage.add(2);
  storage.add(4);
  EXPECT_EQ(storage.size(), 2);

  storage.clear();
  EXPECT_EQ(storage.size(), 0);
  EXPECT_FALSE(storage.isMarked(2));
  EXPECT_FALSE(storage.isMarked(4));
  EXPECT_TRUE(storage.getStorage().empty());
}

TEST(BitVectorStorageTest, MultipleMarks) {
  BitVectorStorage<int> storage(10);
  storage.mark(1);
  storage.mark(2);
  storage.mark(1);

  EXPECT_TRUE(storage.isMarked(1));
  EXPECT_TRUE(storage.isMarked(2));
  EXPECT_FALSE(storage.isMarked(3));
}
