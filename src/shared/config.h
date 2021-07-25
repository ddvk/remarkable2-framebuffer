#pragma once

#include <map>
#include <string>
#include <variant>

using ConfigKey = std::string;
using ConfigValue = std::variant<std::string, void*>;
using Config = std::map<ConfigKey, ConfigValue>;

/**
 * Load the rm2fb configuration for the given OS version.
 *
 * @param version OS version timestamp (e.g. 20210611153600).
 * @return Mapping of the config keys to their value.
 */
Config read_config(const std::string& version);

/**
 * Load the rm2fb configuration for the current OS version (in `/etc/version`).
 *
 * @return Mapping of the config keys to their value.
 */
Config read_config();
