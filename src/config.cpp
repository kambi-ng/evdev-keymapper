
#include "config.hpp"
#include "vendor/toml.hpp"
#include <memory>
#include <optional>

#include <cstdio>
#include <string>
#include <string_view>
#include <utility>

#include "keycodes.hpp"

int keystrtoi(std::string key) {
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
  auto layermap = std::make_shared<std::map<int, map_t_ptr>>();

  for (const auto &[key, value] : *keys.as_table()) {
    auto orig = std::string(key.str());

    if (value.type() == toml::node_type::table) {
      printf("Parsing Layer %s\n", orig.c_str());

      auto submap = std::make_shared<map_t>();
      for (const auto &[subkey, subvalue] : *value.as_table()) {

        auto sub_orig = std::string(subkey.str());
        auto sub_dest = subvalue.value<std::string>();

        int orig_key = keystrtoi(sub_orig);
        if (orig_key == -1){
          printf("Error parsing key %s\n", sub_orig.c_str());
          return {};
        }
        int dest_key = keystrtoi(sub_dest.value_or(sub_orig));
        if (dest_key == -1){
          printf("Error parsing key %s\n", sub_dest.value_or(sub_orig).c_str());
          return {};
        }
        printf("    Adding %s -> %s in layer %s\n", sub_orig.c_str(), sub_dest.value_or(orig).c_str(), orig.c_str());
        submap->insert(std::make_pair(orig_key, dest_key));
      }

      layermap->insert(std::make_pair(keystrtoi(orig), submap));
      continue;
    }

    auto dest = value.value<std::string>();
    int orig_key = keystrtoi(orig);
    if (orig_key == -1){
      printf("Error parsing key %s\n", orig.c_str());
      return {};
    }
    int dest_key = keystrtoi(dest.value_or(orig));
    if (dest_key == -1){
      printf("Error parsing key %s\n", dest.value_or(orig).c_str());
      return {};
    }

    printf("Adding %s -> %s\n", orig.c_str(), dest.value_or(orig).c_str());
    keymap->insert(std::make_pair(orig_key, dest_key));
  }

  auto toggle = conf_file[CONFIG_TABLE_NAME][TOGGLE_CONF_NAME].value_or(false);

  auto device =
      conf_file[CONFIG_TABLE_NAME][DEVICE_CONF_NAME].value<std::string>();
  if (!device.has_value()){
    printf("Error parsing device\n");
    return {};
  }

  return config{.toggle = toggle  ,
                .device = device.value(),
                .keymap = keymap,
                .layermap = layermap};
}
