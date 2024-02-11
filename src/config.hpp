#ifndef CONFIG_H_
#define CONFIG_H_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#define CONFIG_TABLE_NAME "Config"
#define KEYMAP_TABLE_NAME "Keymap"

#define LAYER_KEY_CONF_NAME "layer_key"
#define TOGGLE_CONF_NAME "toggle"
#define DEVICE_CONF_NAME "device"


using map_t = std::map<int, std::vector<int>>;
using map_t_ptr = std::shared_ptr<map_t>;

struct config {
  bool toggle;
  std::string device;
  map_t_ptr keymap;
  std::shared_ptr<std::map<int, map_t_ptr>> layermap;
};

std::optional<config> read_config(std::string filename);

#endif // CONFIG_H_
