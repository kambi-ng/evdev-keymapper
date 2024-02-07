#ifndef CONFIG_H_
#define CONFIG_H_

#include <map>
#include <memory>
#include <optional>
#include <string>

#define CONFIG_TABLE_NAME "Config"
#define KEYMAP_TABLE_NAME "Keymap"

#define LAYER_KEY_CONF_NAME "layer_key"
#define TOGGLE_CONF_NAME "toggle"
#define DEVICE_CONF_NAME "device"

struct config {
  int layerkey;
  bool toggle;
  std::string device;
  std::shared_ptr<std::map<int, int>> keymap;
};

std::optional<config> read_config(std::string filename);

#endif // CONFIG_H_
