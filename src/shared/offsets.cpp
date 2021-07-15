#include "offsets.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

std::map<std::string, void*> read_offsets() {
  // Read current version number
  std::string version;
  std::ifstream version_file_buf{system_version_file};

  if (!version_file_buf) {
    std::cerr << "Read current OS release: " << std::strerror(errno) << "\n";
    std::exit(-1);
  }

  std::getline(version_file_buf, version);

  // Iterate on the offsets file to find offsets for the current version
  std::map<std::string, void*> result;
  std::string line;
  bool version_matches = false;
  std::ifstream offsets_file_buf{main_offsets_file};

  if (!offsets_file_buf) {
    std::cerr << "Read offsets file: " << std::strerror(errno) << "\n";
    std::exit(-1);
  }

  while (std::getline(offsets_file_buf, line)) {
    if (line.empty() && line[0] == '#') {
      // Comment or empty line, ignore
    } else if (line[0] == '!') {
      // Start version block
      std::istringstream line_stream{line.substr(1)};
      std::string file_version;
      line_stream >> file_version;

      if (file_version == version) {
        version_matches = true;
      } else {
        version_matches = false;
      }
    } else if (version_matches) {
      std::istringstream line_stream{line};
      line_stream.unsetf(std::ios::basefield);

      std::string function_name;
      std::uint32_t offset_value;

      line_stream >> function_name >> offset_value;
      result[function_name] = (char*) offset_value;
    }
  }

  return result;
}
