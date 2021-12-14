#include "config.h"
#include "defines.h"
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {

constexpr auto default_config = R"CONF(
!20201016123042
version str 2.4.0.27
update addr 0x2b2534
updateType str int4
create addr 0x2b47c0
shutdown addr 0x2b4764
wait addr 0x2b3ffc
getInstance addr 0x2b2a54

!20201028163830
version str 2.4.1.30
update addr 0x2b255c
updateType str int4
create addr 0x2b47e8
shutdown addr 0x2b478c
wait addr 0x2b4024
getInstance addr 0x2b2a7c

!20201127104549
version str 2.5.0.27
update addr 0x2c050c
updateType str int4
create addr 0x2c2798
shutdown addr 0x2c273c
wait addr 0x2c1fd4
getInstance addr 0x2c0a2c

!20201216142449
version str 2.5.1.47
update addr 0x2c0664
updateType str int4
create addr 0x2c28f0
shutdown addr 0x2c2894
wait addr 0x2c212c
getInstance addr 0x2c0b84

!20210311194323
version str 2.6.1.71
update addr 0x311c10
updateType str int4
create addr 0x314178
shutdown addr 0x31411c
wait addr 0x31369c
getInstance addr 0x312128

!20210322075357
version str 2.6.2.75
update addr 0x311c00
updateType str int4
create addr 0x314168
shutdown addr 0x31410c
wait addr 0x31368c
getInstance addr 0x312118

!20210504114631
version str 2.7.0.51
update addr 0x32373c
updateType str int4
create addr 0x325ca8
shutdown addr 0x325c4c
wait addr 0x3251cc
getInstance addr 0x323c54

!20210511153632
version str 2.7.1.53
update addr 0x32373c
updateType str int4
create addr 0x325ca8
shutdown addr 0x325c4c
wait addr 0x3251cc
getInstance addr 0x323c54

!20210611153600
version str 2.8.0.98
update addr 0x3426ac
updateType str int4
create addr 0x344c18
shutdown addr 0x344bbc
wait addr 0x34413c
getInstance addr 0x342bc8

!20210709092503
version str 2.9.0.153
update addr 0x3aa5c4
updateType str QRect
create addr 0x3ac9d8
shutdown addr 0x3ac97c
wait addr 0x3abefc
getInstance addr 0x3a0e7c

!20210809135257
version str 2.9.0.210
update addr 0x3adc0c
updateType str QRect
create addr 0x3b0020
shutdown addr 0x3affc4
wait addr 0x3af544
getInstance addr 0x3a44c4

!20210812195523
version str 2.9.1.217
update addr 0x3afc04
updateType str QRect
create addr 0x3b2018
shutdown addr 0x3b1fbc
wait addr 0x3b153c
getInstance addr 0x3a64bc

!20210929140057
version str 2.10.1.332
update addr 0x397ff4
updateType str QRect
create addr 0x39a408
shutdown addr 0x39a3ac
wait addr 0x39992c
getInstance addr 0x38e8dc

!20211014151303
version str 2.10.2.356
update addr 0x398aac
updateType str QRect
create addr 0x39aec0
shutdown addr 0x39ae64
wait addr 0x39a3e4
getInstance addr 0x38f394

!20211102143141
version str 2.10.3.379
update addr 0x398acc
updateType str QRect
create addr 0x39aee0
shutdown addr 0x39ae84
wait addr 0x39a404
getInstance addr 0x38f3b4

!20211202111519
version str 2.11.0.433
update addr 0x3a9ccc
updateType str QRect
create addr 0x3ac124
shutdown addr 0x3ac0c8
wait addr 0x3ab604
getInstance addr 0x3a041c

!20211203054951
version str 2.11.0.435
update addr 0x3a9ccc
updateType str QRect
create addr 0x3ac124
shutdown addr 0x3ac0c8
wait addr 0x3ab604
getInstance addr 0x3a041c

!20211208075454
version str 2.11.0.442
update addr 0x3a9c28
updateType str QRect
create addr 0x3ac134
shutdown addr 0x3ac0d8
wait addr 0x3ab614
getInstance addr 0x3a042c
)CONF";

void read_config_file(
  const std::string& name,
  std::istream& file,
  const std::string& version,
  Config& accumulator
) {
  std::string line;
  unsigned int line_no = 1;
  bool version_matches = true;

  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#') {
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
      // Key/value for current version
      std::istringstream line_stream{line};
      std::string key, type;
      line_stream >> key >> type;

      if (type == "addr") {
        std::uint32_t value;
        line_stream.unsetf(std::ios::basefield);
        line_stream >> value;
        accumulator[key] = (char*) value;
      } else if (type == "str") {
        std::string value;
        line_stream >> value;
        accumulator[key] = value;
      } else {
        std::cerr << name << ":" << line_no << " - Ignored key of invalid "
          "type '" << type << "'\n";
      }
    }
    ++line_no;
  }
}

}

Config read_config(const std::string& version) {
  Config result;
  std::istringstream default_config_file{default_config};
  read_config_file("<default config>", default_config_file, version, result);

  constexpr std::array config_locations = {
    "/usr/share/rm2fb.conf",
    "/opt/share/rm2fb.conf",
    "/etc/rm2fb.conf",
    "/opt/etc/rm2fb.conf",
    "rm2fb.conf",
  };

  for (const auto& path : config_locations) {
    std::ifstream config_file{path};

    if (!config_file) {
#ifdef DEBUG
      std::cerr << path << " - " << std::strerror(errno) << " (skipped)\n";
#endif
      continue;
    }

#ifdef DEBUG
    std::cerr << path << " - Parsing contents\n";
#endif

    read_config_file(path, config_file, version, result);
  }

  return result;
}

Config read_config() {
  std::string version;
  std::ifstream version_file_buf{"/etc/version"};

  if (!version_file_buf) {
    std::cerr << "/etc/version - " << std::strerror(errno) << "\n";
    std::exit(-1);
  }

  std::getline(version_file_buf, version);
  return read_config(version);
}
