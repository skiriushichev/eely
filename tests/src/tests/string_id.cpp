#include <eely/base/bit_reader.h>
#include <eely/base/bit_writer.h>
#include <eely/base/string_id.h>

#include <gtest/gtest.h>

TEST(string_id, utils)
{
  using namespace eely;
  using namespace eely::internal;

  // string_id_serialize & string_id_deserialize
  {
    std::array<std::byte, 32> buffer;
    bit_reader reader{buffer};
    bit_writer writer{buffer};

    string_id id{};
    bit_writer_write(writer, id);
    string_id id_deserialized{bit_reader_read<string_id>(reader)};
    EXPECT_EQ(id, id_deserialized);

    id = "Such a lovely string";
    bit_writer_write(writer, id);
    id_deserialized = bit_reader_read<string_id>(reader);
    EXPECT_EQ(id, id_deserialized);
  }
}