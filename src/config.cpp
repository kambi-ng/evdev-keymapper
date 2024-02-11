
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
std::vector<std::string> split_string(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::pair<int, std::vector<int>> parse_key(std::string key, std::string value ){
  int orig_key = keystrtoi(key);
  if (orig_key == -1){
    printf("Error parsing key %s\n", key.c_str());
    return std::make_pair(-1, std::vector<int>());
  }
  auto dest_keys = std::vector<int>();

  auto vals = split_string(value, '+');

  for (auto val : vals){
    int dest_key = keystrtoi(val);
    if (dest_key == -1){
      printf("Error parsing key %s\n", val.c_str());
      return std::make_pair(-1, std::vector<int>());
    }
    dest_keys.push_back(dest_key);
  }
  return std::make_pair(orig_key, dest_keys);
}


map_t_ptr parse_layer(const toml::table &table) {
  auto map = std::make_shared<map_t>();
  for (const auto &[key, value] : table) {
    auto orig = std::string(key.str());
    auto dest = value.value<std::string>();
    printf("   Mapping Key %s -> %s\n", orig.c_str(), dest.value_or("").c_str());
    auto keyval = parse_key(orig, dest.value_or(orig));
    if (keyval.first == -1){
      return {};
    }

    map->insert(keyval);
  }
  return map;
}


std::optional<config> read_config(std::string filename) {
  auto conf_file = toml::parse_file(filename);
  auto keys = conf_file[KEYMAP_TABLE_NAME];
  auto layermap = std::make_shared<std::map<int, map_t_ptr>>();
  auto keymap = std::make_shared<map_t>();

  for (const auto &[key, value] : *keys.as_table()) {
    auto orig = std::string(key.str());

    if (value.type() == toml::node_type::table) {
      printf("Parsing Layer %s\n", orig.c_str());
      auto map = parse_layer(*value.as_table());
      layermap->insert(std::make_pair(keystrtoi(orig), map));
      continue;
    }

    auto dest = value.value_or(orig);
    printf("Mapping Key %s -> %s\n", orig.c_str(), dest.c_str());
    auto keyval = parse_key(orig, dest);

    if (keyval.first == -1){
      return {};
    }

    keymap->insert(keyval);
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
