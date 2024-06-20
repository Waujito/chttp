#include <gtest/gtest.h>
#include "server/utils.h"

TEST(VectorTest, VectorCreates) {
	struct vector vc;
	initVector(&vc, 8);

	ASSERT_EQ(vc.size, 0);
	ASSERT_EQ(vc.capacity, 8);

	growVector(&vc);
	ASSERT_EQ(vc.capacity, 16);

	growVector(&vc);
	ASSERT_EQ(vc.capacity, 32);

	vectorDestroy(&vc);
}

TEST(VectorTest, VectorInsertsAndSets) {
	struct vector vc;
	initVector(&vc, 2);

	ASSERT_EQ(vc.size, 0);
	ASSERT_EQ(vc.capacity, 2);

	int a = 1;
	int b = 2;
	int c = 3;
	int d = 4;
	int e = 5;

	size_t i = vectorInsertEl(&vc, &a);
	ASSERT_EQ(vc.size, 1);
	ASSERT_EQ(vc.capacity, 2);
	ASSERT_EQ(i, 0);
	ASSERT_EQ(*(int *)vectorGetEl(&vc, 0), a);

	i = vectorInsertEl(&vc, &b);
	ASSERT_EQ(vc.size, 2);
	ASSERT_EQ(vc.capacity, 2);
	ASSERT_EQ(i, 1);
	ASSERT_EQ(*(int *)vectorGetEl(&vc, 0), a);
	ASSERT_EQ(*(int *)vectorGetEl(&vc, 1), b);

	i = vectorInsertEl(&vc, &c);
	ASSERT_EQ(vc.size, 3);
	ASSERT_EQ(vc.capacity, 4);
	ASSERT_EQ(i, 2);
	ASSERT_EQ(*(int *)vectorGetEl(&vc, 0), a);
	ASSERT_EQ(*(int *)vectorGetEl(&vc, 1), b);
	ASSERT_EQ(*(int *)vectorGetEl(&vc, 2), c);

	vectorSetEl(&vc, 1, &c);
	ASSERT_EQ(*(int *)vectorGetEl(&vc, 1), c);

	i = vectorInsertEl(&vc, &d);
	ASSERT_EQ(vc.size, 4);
	ASSERT_EQ(vc.capacity, 4);
	ASSERT_EQ(i, 3);
	ASSERT_EQ(*(int *)vectorGetEl(&vc, 0), a);
	ASSERT_EQ(*(int *)vectorGetEl(&vc, 1), c);
	ASSERT_EQ(*(int *)vectorGetEl(&vc, 2), c);
	ASSERT_EQ(*(int *)vectorGetEl(&vc, 3), d);

	i = vectorInsertEl(&vc, &e);
	ASSERT_EQ(vc.size, 5);
	ASSERT_EQ(vc.capacity, 8);
	ASSERT_EQ(i, 4);
	ASSERT_EQ(*(int *)vectorGetEl(&vc, 0), a);
	ASSERT_EQ(*(int *)vectorGetEl(&vc, 1), c);
	ASSERT_EQ(*(int *)vectorGetEl(&vc, 2), c);
	ASSERT_EQ(*(int *)vectorGetEl(&vc, 3), d);
	ASSERT_EQ(*(int *)vectorGetEl(&vc, 4), e);

	vectorDestroy(&vc);
}

TEST(VectorPTest, VectorCreates) {
	struct vector_p vc;
	initVector_p(&vc, 4, 8);

	ASSERT_EQ(vc.size, 0);
	ASSERT_EQ(vc.capacity, 8);
	ASSERT_EQ(vc.elsz, 4);

	growVector_p(&vc);
	ASSERT_EQ(vc.capacity, 16);

	growVector_p(&vc);
	ASSERT_EQ(vc.capacity, 32);

	vectorDestroy_p(&vc);
}

TEST(VectorPTest, VectorInsertsAndSets) {
	struct vector_p vc;
	initVector_p(&vc, sizeof(int), 2);

	ASSERT_EQ(vc.size, 0);
	ASSERT_EQ(vc.capacity, 2);
	ASSERT_EQ(vc.elsz, sizeof(int));

	int a = 1;
	int b = 2;
	int c = 3;
	int d = 4;
	int e = 5;

	size_t i = vectorInsertEl_p(&vc, (char *)&a);
	ASSERT_EQ(vc.size, 1);
	ASSERT_EQ(vc.capacity, 2);
	ASSERT_EQ(i, 0);
	ASSERT_EQ(*(int *)vectorGetEl_p(&vc, 0), a);

	i = vectorInsertEl_p(&vc, (char *)&b);
	ASSERT_EQ(vc.size, 2);
	ASSERT_EQ(vc.capacity, 2);
	ASSERT_EQ(i, 1);
	ASSERT_EQ(*(int *)vectorGetEl_p(&vc, 0), a);
	ASSERT_EQ(*(int *)vectorGetEl_p(&vc, 1), b);

	i = vectorInsertEl_p(&vc, (char *)&c);
	ASSERT_EQ(vc.size, 3);
	ASSERT_EQ(vc.capacity, 4);
	ASSERT_EQ(i, 2);
	ASSERT_EQ(*(int *)vectorGetEl_p(&vc, 0), a);
	ASSERT_EQ(*(int *)vectorGetEl_p(&vc, 1), b);
	ASSERT_EQ(*(int *)vectorGetEl_p(&vc, 2), c);

	vectorSetEl_p(&vc, 1, (char *)&c);
	ASSERT_EQ(*(int *)vectorGetEl_p(&vc, 1), c);

	i = vectorInsertEl_p(&vc, (char *)&d);
	ASSERT_EQ(vc.size, 4);
	ASSERT_EQ(vc.capacity, 4);
	ASSERT_EQ(i, 3);
	ASSERT_EQ(*(int *)vectorGetEl_p(&vc, 0), a);
	ASSERT_EQ(*(int *)vectorGetEl_p(&vc, 1), c);
	ASSERT_EQ(*(int *)vectorGetEl_p(&vc, 2), c);
	ASSERT_EQ(*(int *)vectorGetEl_p(&vc, 3), d);

	i = vectorInsertEl_p(&vc, (char *)&e);
	ASSERT_EQ(vc.size, 5);
	ASSERT_EQ(vc.capacity, 8);
	ASSERT_EQ(i, 4);
	ASSERT_EQ(*(int *)vectorGetEl_p(&vc, 0), a);
	ASSERT_EQ(*(int *)vectorGetEl_p(&vc, 1), c);
	ASSERT_EQ(*(int *)vectorGetEl_p(&vc, 2), c);
	ASSERT_EQ(*(int *)vectorGetEl_p(&vc, 3), d);
	ASSERT_EQ(*(int *)vectorGetEl_p(&vc, 4), e);

	vectorDestroy_p(&vc);
}
