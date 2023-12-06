#include <gtest/gtest.h>

#include <syssnap/syssnap.hpp>

TEST(syssnap, exists)
{
	EXPECT_TRUE(true);
}

auto main(int argc, char ** argv) -> int
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}