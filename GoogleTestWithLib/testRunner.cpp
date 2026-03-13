#include <iostream>
#include <gtest/gtest.h>
#include "LibraryCode.hpp"

TEST(TestSample, TestAddition)
{
    ASSERT_EQ(2, add(1,1));
}
