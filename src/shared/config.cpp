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
update addr 0x3a9cdc
updateType str QRect
create addr 0x3ac134
shutdown addr 0x3ac0d8
wait addr 0x3ab614
getInstance addr 0x3a042c

!20220202133838
version str 2.12.1.527
update addr 0x3d72bc
updateType str QRect
create addr 0x3d9714
shutdown addr 0x3d96b8
wait addr 0x3d8bf4
getInstance addr 0x3cda0c

!20220303120824
version str 2.12.2.573
update addr 0x3edff4
updateType str QRect
create addr 0x3f0138
shutdown addr 0x3f00d0
wait addr 0x3ef704
getInstance addr 0x3e4ddc

!20220330134519
version str 2.12.3.606
update addr 0x3ee704
updateType str QRect
create addr 0x3f0848
shutdown addr 0x3f07e0
wait addr 0x3efe14
getInstance addr 0x3e54ec

!20220429114855
version str 2.13.0.689
update addr 0X45899c
updateType str QRect
create addr 0x45aae0
shutdown addr 0x45aa78
wait addr 0x45a0ac
getInstance addr 0x44f784

!20220519120030
version str 2.13.0.758
update addr 0x4589fc
updateType str QRect
create addr 0x45ab40
shutdown addr 0x45aad8
wait addr 0x45a10c
getInstance addr 0x44f7e4

!20220601111708
version str 2.14.0.830
update addr 0x4931c4
updateType str QRect
create addr 0x495308
shutdown addr 0x4952a0
wait addr 0x4948d4
getInstance addr 0x489fac

!20220615074909
version str 2.14.0.861
update addr 0x4931c4
updateType str QRect
create addr 0x495308
shutdown addr 0x4952a0
wait addr 0x4948d4
getInstance addr 0x489fac

!20220617143306
version str 2.14.1.866
update addr 0x4931c4
updateType str QRect
create addr 0x495308
shutdown addr 0x4952a0
wait addr 0x4948d4
getInstance addr 0x489fac

!20220725110358
version str 2.14.3.925
update addr 0x4bfb1c
updateType str QRect
create addr 0x4c2740
shutdown addr 0x4c26d8
wait addr 0x4c16e0
getInstance addr 0x4b66a4

!20220804123958
version str 2.14.3.940
update addr 0x4bfb1c
updateType str QRect
create addr 0x4c2740
shutdown addr 0x4c26d8
wait addr 0x4c16e0
getInstance addr 0x4b66a4

!20220805144937
version str 2.14.3.942
update addr 0x4bfb1c
updateType str QRect
create addr 0x04c2740
shutdown addr 0x4c26d8
wait addr 0x4c16e0
getInstance addr 0x4b66a4

!20220817135850
version str 2.14.3.958
update addr 0x4bfb2c
updateType str QRect
create addr 0x4c2750
shutdown addr 0x4c26e8
wait addr 0x4c16f0
getInstance addr 0x4b66b4

!20220825124750
version str 2.14.3.977
update addr 0x4bfb2c
updateType str QRect
create addr 0x4c2750
shutdown addr 0x4c26e8
wait addr 0x4c16f0
getInstance addr 0x4b66b4

!20220907143405
version str 2.14.3.1005
update addr 0x4bfb2c
updateType str QRect
create addr 0x4c2750
shutdown addr 0x4c26e8
wait addr 0x4c16f0
getInstance addr 0x4b66b4

!20220921101206
version str 2.14.3.1047
update addr 0x4bfb2c
updateType str QRect
create addr 0x4c2750
shutdown addr 0x4c26e8
wait addr 0x4c16f0
getInstance addr 0x4b66b4

!20220909155240
version str 2.15.0.1011
update addr 0x4e411c
updateType str QRect
create addr 0x4e6d40
shutdown addr 0x4e6cd8
wait addr 0x4e5ce0
getInstance addr 0x4daca4

!20220921071527
version str 2.15.0.1046
update addr 0x4e418c
updateType str QRect
create addr 0x4e6db0
shutdown addr 0x4e6d48
wait addr 0x4e5d50
getInstance addr 0x4dad14

!20220923094801
version str 2.15.0.1052
update addr 0x4e4254
updateType str QRect
create addr 0x4e6e78
shutdown addr 0x4e6e10
wait addr 0x4e5e18
getInstance addr 0x4daddc

!20221003075633
version str 2.15.0.1067
update addr 0x4e425c
updateType str QRect
create addr 0x4e6e80
shutdown addr 0x4e6e18
wait addr 0x4e5e20
getInstance addr 0x4dade4
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
