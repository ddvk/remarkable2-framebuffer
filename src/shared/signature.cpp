// ./a.out < xochitl
// ./a.out < remarkable-shutdown

#include <string>
#include <fstream>
#include <sstream>


namespace swtfb {
struct Signature {
  bool indirect;
  std::string bytes;
  int offset;
};

std::string read_file(const std::string& path) {
  std::ostringstream output;
  std::ifstream input_file{path, std::ifstream::binary};
  output << input_file.rdbuf();
  return std::string(output.str());
}

std::int32_t decode_jump_offset(std::string instruction) {
  // Assuming A1 or A2 (4 bytes) instruction encoding
  // See section A8.8.25 of the ARMv7 Architecture Reference Manual
  char byte_0 = instruction[0];
  char byte_1 = instruction[1];
  char byte_2 = instruction[2];
  char byte_3 = instruction[3];

  if (((byte_3 >> 1) & 0b111) != 0b101) {
    // Not a jump instruction
    return 0;
  }

  std::int32_t res = (
    (byte_2 << 16)
    | (byte_1 << 8)
    | byte_0
  );

  bool is_negative = byte_2 >> 7;

  if (is_negative) {
    res |= (0xff << 24);
  }

  return (res << 2) + 8;
}

void *locate_signature(const std::string& data, const Signature& signature) {
  const size_t base_offset = 0x10000;
  const size_t position = data.find(signature.bytes);

  if (position == std::string::npos) {
    return nullptr;
  }

  if (!signature.indirect) {
    return (char *) position + base_offset - 4 + signature.offset;
  } else {
    auto jump_offset = decode_jump_offset(data.substr(position + signature.offset, 4));

    if (jump_offset == 0) {
      return nullptr;
    }

    return (char*) position + signature.offset + base_offset + jump_offset;
  }
}

void *locate_signature(const std::string& data, const std::vector<Signature>& signatures) {
  for (const Signature& signature : signatures) {
    void *result = locate_signature(data, signature);

    if (result != nullptr) {
      return result;
    }
  }

  return nullptr;
}
} // namespace swtfb
