#pragma once

#include <map>
#include <string>

/* Path to the file containing the current OS release identifier. */
constexpr auto system_version_file = "/etc/version";

/* Path to the file containing per-release Xochitl function offsets. */
constexpr auto main_offsets_file = "/opt/share/rm2fb/offsets";

/**
 * Read the Xochitl function offsets for the current OS version.
 *
 * @return Mapping of the function names to their pointer.
 */
std::map<std::string, void*> read_offsets();
