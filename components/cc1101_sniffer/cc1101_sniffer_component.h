#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/core/log.h"
#include <RadioLib.h>

namespace esphome {
namespace cc1101_sniffer {

// CC1101 sniffer component for ESPHome using RadioLib
// Constructor: CC1101SnifferComponent(cs_pin, gdo0_pin, gdo2_pin, freq_mhz)

class CC1101SnifferComponent : public PollingComponent {
 public:

  // Default constructor for ESPHome
  CC1101SnifferComponent() : PollingComponent(200) {}

  // Legacy constructor for custom_component usage
  CC1101SnifferComponent(int cs_pin, int gdo0_pin, int gdo2_pin, float freq_mhz)
      : PollingComponent(200), cs_pin_(cs_pin), gdo0_pin_(gdo0_pin),
        gdo2_pin_(gdo2_pin), freq_mhz_(freq_mhz) {}

  void setup() override;
  void update() override;

  // Setter methods for ESPHome component configuration
  void set_cs_pin(InternalGPIOPin *pin) { cs_pin_ = pin->get_pin(); }
  void set_gdo0_pin(InternalGPIOPin *pin) { gdo0_pin_ = pin->get_pin(); }
  void set_gdo2_pin(InternalGPIOPin *pin) { gdo2_pin_ = pin->get_pin(); }
  
  // Helper to change frequency (both at config time and runtime)
  void set_frequency(float freq) {
    freq_mhz_ = freq;
    if (radio_) {
      radio_->setFrequency(freq_mhz_);
    }
  }

 protected:
  int cs_pin_{-1};
  int gdo0_pin_{-1};
  int gdo2_pin_{-1};
  float freq_mhz_{868.3};

  // RadioLib CC1101 instance
  CC1101 *radio_{nullptr};

  // helper to convert bytes to hex string
  std::string bytes_to_hex(const uint8_t *buf, size_t len);
};

}  // namespace cc1101_sniffer
}  // namespace esphome
