#include "tests/test_utils.h"

#include "eely/math_utils.h"
#include "eely/quaternion.h"
#include <eely/clip_utils.h>

#include <gtest/gtest.h>

#include <cmath>
#include <random>

static float quantize_and_dequantize(const eely::float_quantize_params& params)
{
  using namespace eely;

  uint16_t quantized{float_quantize(params)};

  float_dequantize_params dequantize_params{.data = quantized,
                                            .bits_count = params.bits_count,
                                            .range_from = params.range_from,
                                            .range_length = params.range_length};
  return float_dequantize(dequantize_params);
}

static eely::quaternion quantize_and_dequantize(const eely::quaternion& q)
{
  using namespace eely;

  const std::array<uint16_t, 4> quantized{quaternion_quantize(q)};
  return quaternion_dequantize(quantized);
}

TEST(quantization, floats)
{
  using namespace eely;

  // Quantize bunch of values and check that dequantized values are within accepted error
  // Also, check that interval ends are represented exactly

  auto check_float_quantize = [](const float value) {
    float_quantize_params params{.value = value};

    for (gsl::index bits_count{1}; bits_count <= 16; ++bits_count) {
      params.bits_count = bits_count;

      params.range_from = value - 1.0F;
      params.range_length = 2.0F;
      EXPECT_NEAR(value, quantize_and_dequantize(params),
                  calculate_acceptable_quantize_error(params));

      params.range_from = value - 0.5F;
      params.range_length = 1.0F;
      EXPECT_NEAR(value, quantize_and_dequantize(params),
                  calculate_acceptable_quantize_error(params));

      params.range_from = value;
      params.range_length = 1.0F;
      EXPECT_NEAR(value, quantize_and_dequantize(params),
                  calculate_acceptable_quantize_error(params));

      params.range_from = value - 1.0F;
      params.range_length = 1.0F;
      EXPECT_NEAR(value, quantize_and_dequantize(params),
                  calculate_acceptable_quantize_error(params));

      params.range_from = value - 3.0F;
      params.range_length = 3.2F;
      EXPECT_NEAR(value, quantize_and_dequantize(params),
                  calculate_acceptable_quantize_error(params));

      params.range_from = value - 8.0F;
      params.range_length = 13.0F;
      EXPECT_NEAR(value, quantize_and_dequantize(params),
                  calculate_acceptable_quantize_error(params));

      params.range_from = value - 0.10F;
      params.range_length = 9.8F;
      EXPECT_NEAR(value, quantize_and_dequantize(params),
                  calculate_acceptable_quantize_error(params));
    }
  };

  // Generate not very high numbers to avoid addiitional floating point accuracy errors
  static constexpr int random_samples = 200;
  std::mt19937 gen(seed);
  std::uniform_real_distribution<float> distr(-10.0F, 10.0F);
  for (int i{0}; i < random_samples; ++i) {
    const float value{distr(gen)};
    check_float_quantize(value);
  }
}

TEST(quantization, quaternions)
{
  using namespace eely;

  const float acceptible_error{calculate_acceptable_quantize_error(
      {.bits_count = 16, .range_from = -1.0F, .range_length = 2.0F})};

  auto check_quaternion_quantize = [acceptible_error](quaternion q) {
    const quaternion q_dequantized{quantize_and_dequantize(q)};

    for (gsl::index i{0}; i < 4; ++i) {
      const float diff{std::abs(quaternion_get_at(q, i) - quaternion_get_at(q_dequantized, i))};
      EXPECT_NEAR(quaternion_get_at(q, i), quaternion_get_at(q_dequantized, i), acceptible_error);
    }
  };

  static constexpr int random_samples = 200;
  std::mt19937 gen(seed);
  std::uniform_real_distribution<float> distr(-1.0F, 1.0F);
  for (int i{0}; i < random_samples; ++i) {
    const quaternion value = [&distr, &gen]() {
      quaternion generated{distr(gen), distr(gen), distr(gen), distr(gen)};
      return quaternion_normalized(generated);
    }();

    check_quaternion_quantize(value);
  }
}