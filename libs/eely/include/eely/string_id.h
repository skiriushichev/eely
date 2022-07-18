#pragma once

#include "eely/bit_reader.h"
#include "eely/bit_writer.h"

#include <string>

namespace eely {
// TODO: hashed string
using string_id = std::string;

// Serialize string id into memory buffer.
void string_id_serialize(const string_id& id, bit_writer& writer);

// Deserialize string id from memory buffer.
string_id string_id_deserialize(bit_reader& reader);
}  // namespace eely