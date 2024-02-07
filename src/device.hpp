#ifndef DEVICE_H_
#define DEVICE_H_

#include <memory>
#include "config.hpp"

struct device_state;

using device_state_ptr = std::shared_ptr<device_state>;

device_state_ptr setup_devices(config conf);

void listen_and_remap(device_state_ptr state);

#endif // DEVICE_H_
