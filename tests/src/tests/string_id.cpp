#include <eely/bit_reader.h>
#include <eely/bit_writer.h>
#include <eely/string_id.h>

#include <gtest/gtest.h>

TEST(string_id, utils)
{
  using namespace eely;

  // string_id_serialize & string_id_deserialize
  {
    std::array<std::byte, 32> buffer;
    bit_reader reader{buffer};
    bit_writer writer{buffer};

    string_id id{};
    string_id_serialize(id, writer);
    string_id id_deserialized{string_id_deserialize(reader)};
    EXPECT_EQ(id, id_deserialized);

    id = "Such a lovely string";
    string_id_serialize(id, writer);
    id_deserialized = string_id_deserialize(reader);
    EXPECT_EQ(id, id_deserialized);
  }
}