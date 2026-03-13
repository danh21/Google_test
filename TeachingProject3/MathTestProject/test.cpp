#include "pch.h"
#include "../MyMathLib/MyMathLib.hpp"

TEST(MathTestCase, MathTestSqrt9)
{
	ASSERT_EQ(3, mySqrt(9));
}

TEST(MathTestCase, MathTestSqrt1)
{
	ASSERT_EQ(1, mySqrt(1));
}