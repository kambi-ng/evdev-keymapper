#ifndef CONFIG_H_
#define CONFIG_H_

#include <map>
#include <memory>
#include <optional>
#include <string>

struct config {
  int layerkey;
  bool toggle;
  std::shared_ptr<std::map<int, int>> keymap;
};

std::optional<config> readconfig(std::string filename);

#endif // CONFIG_H_
