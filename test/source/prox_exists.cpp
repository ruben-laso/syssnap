#include <gtest/gtest.h>

#include <prox/prox.hpp>

TEST(prox, exists)
{
	EXPECT_TRUE(true);
}

auto main() -> int
{
	::testing::InitGoogleTest();
	return RUN_ALL_TESTS();
}