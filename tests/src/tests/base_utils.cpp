#include <eely/base_utils.h>

#include <gtest/gtest.h>

#include <limits>

TEST(base_utils, base_utils)
{
  using namespace eely;

  // bit_cast
  {
    float f{2.178281828F};
    uint32_t i{bit_cast<uint32_t>(f)};
    float f_returned{bit_cast<float>(i)};
    EXPECT_EQ(f, f_returned);
  }

  //
  {
    enum test_flags { flag_0 = 1 << 0, flag_1 = 1 << 1, flag_2 = 1 << 2 };

    EXPECT_FALSE(has_flag(0b000, test_flags::flag_0));
    EXPECT_FALSE(has_flag(0b000, test_flags::flag_1));
    EXPECT_FALSE(has_flag(0b000, test_flags::flag_2));

    EXPECT_TRUE(has_flag(0b001, test_flags::flag_0));
    EXPECT_FALSE(has_flag(0b001, test_flags::flag_1));
    EXPECT_FALSE(has_flag(0b001, test_flags::flag_2));

    EXPECT_TRUE(has_flag(0b011, test_flags::flag_0));
    EXPECT_TRUE(has_flag(0b011, test_flags::flag_1));
    EXPECT_FALSE(has_flag(0b011, test_flags::flag_2));

    EXPECT_TRUE(has_flag(0b111, test_flags::flag_0));
    EXPECT_TRUE(has_flag(0b111, test_flags::flag_1));
    EXPECT_TRUE(has_flag(0b111, test_flags::flag_2));
  }
}