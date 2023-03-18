#include <eely/base/bit_reader.h>
#include <eely/base/bit_writer.h>

#include <gtest/gtest.h>

#include <array>

TEST(bit_reader_writer, write_and_read)
{
  using namespace eely;
  using namespace eely::internal;

  std::array<std::byte, 4> buffer;

  bit_writer writer{buffer};
  bit_reader reader{buffer};

  EXPECT_EQ(writer.get_bit_position(), 0);
  EXPECT_EQ(reader.get_position_bits(), 0);

  writer.write({.value = 0b01110110'1, .size_bits = 9});
  EXPECT_EQ(writer.get_bit_position(), 9);

  EXPECT_EQ(reader.read(1), 1);
  EXPECT_EQ(reader.read(1), 0);
  EXPECT_EQ(reader.read(1), 1);
  EXPECT_EQ(reader.read(1), 1);
  EXPECT_EQ(reader.read(1), 0);
  EXPECT_EQ(reader.read(1), 1);
  EXPECT_EQ(reader.read(1), 1);
  EXPECT_EQ(reader.read(1), 1);
  EXPECT_EQ(reader.read(1), 0);
  EXPECT_EQ(reader.get_position_bits(), 9);

  writer.write({.value = 0b11100111'00, .size_bits = 10});
  EXPECT_EQ(writer.get_bit_position(), 19);

  EXPECT_EQ(reader.read(10), 0b11100111'00);
  EXPECT_EQ(reader.get_position_bits(), 19);

  writer.patch({.value = 0b10001001'0, .size_bits = 9, .offset_bits = 0});
  EXPECT_EQ(writer.get_bit_position(), 19);

  reader = bit_reader(buffer);
  EXPECT_EQ(reader.read(9), 0b10001001'0);

  reader.read(10);
  EXPECT_EQ(reader.get_position_bits(), 19);

  writer.align();
  EXPECT_EQ(writer.get_bit_position(), 24);

  EXPECT_EQ(reader.read(5), 0);
  EXPECT_EQ(reader.get_position_bits(), 24);

  EXPECT_ANY_THROW(writer.write({.value = 0, .size_bits = 9}));
  EXPECT_ANY_THROW(reader.read(9));
}

TEST(bit_reader_writer, utils)
{
  using namespace eely;
  using namespace eely::internal;

  std::array<std::byte, 4> buffer;

  bit_writer writer(buffer);
  bit_reader reader(buffer);

  EXPECT_EQ(bit_reader_get_bytes_read(reader), 0);
  EXPECT_EQ(bit_writer_get_bytes_written(writer), 0);

  writer.write({.value = 0, .size_bits = 1});
  reader.read(1);
  EXPECT_EQ(bit_reader_get_bytes_read(reader), 1);
  EXPECT_EQ(bit_writer_get_bytes_written(writer), 1);

  writer.write({.value = 0, .size_bits = 7});
  reader.read(7);
  EXPECT_EQ(bit_reader_get_bytes_read(reader), 1);
  EXPECT_EQ(bit_writer_get_bytes_written(writer), 1);

  writer.write({.value = 0, .size_bits = 9});
  reader.read(9);
  EXPECT_EQ(bit_reader_get_bytes_read(reader), 3);
  EXPECT_EQ(bit_writer_get_bytes_written(writer), 3);
}