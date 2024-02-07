
#include "config.hpp"
#include "vendor/toml.hpp"
#include <memory>
#include <optional>

#include <cstdio>
#include <string>
#include <string_view>
#include <utility>

#include "keycodes.hpp"

int keystrtoi (std::string key) {
  if (keycodeMap.find(key) != keycodeMap.end()) {
    return keycodeMap[key];
  } else {
    // Return -1 if the key is not found
    return -1;
  }
}

std::optional<config> read_config(std::string filename) {
  auto conf_file = toml::parse_file(filename);

  auto keys = conf_file[KEYMAP_TABLE_NAME];
  auto keymap = std::make_shared<std::map<int, int>>();

  for (const auto &[key, value] : *keys.as_table()) {
    auto orig = std::string(key.str());
    auto dest = value.value<std::string>();

    int orig_key = keystrtoi(orig);
    if (orig_key == -1) return {};

    int dest_key = keystrtoi(dest.value_or(orig));
    if (dest_key == -1) return {};

    keymap->insert(std::make_pair(orig_key, dest_key));
  }

  auto layerkeystr = conf_file[CONFIG_TABLE_NAME][LAYER_KEY_CONF_NAME].value_or("");
  int layerkey = keystrtoi(layerkeystr);
  if (layerkey == -1)  return {};

  auto toggle = conf_file[CONFIG_TABLE_NAME][TOGGLE_CONF_NAME].value_or(false);

  auto device = conf_file[CONFIG_TABLE_NAME][DEVICE_CONF_NAME].value<std::string>();
  if (!device.has_value()) return {};

  return config{.layerkey = layerkey, .toggle = toggle, .device = device.value(), .keymap = keymap};
}

