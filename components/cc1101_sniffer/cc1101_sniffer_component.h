#pragma once

#include "esphome.h"
#include <RadioLib.h>

// CC1101 sniffer component for ESPHome using RadioLib
// Constructor: CC1101SnifferComponent(cs_pin, gdo0_pin, gdo2_pin, freq_mhz)

class CC1101SnifferComponent : public PollingComponent {
 public:
  // public text_sensor pointer so YAML / other components can read state
  text_sensor::TextSensor *packet_text_sensor = new text_sensor::TextSensor();

  CC1101SnifferComponent(int cs_pin, int gdo0_pin, int gdo2_pin, float freq_mhz)
      : PollingComponent(200), cs_pin_(cs_pin), gdo0_pin_(gdo0_pin),
        gdo2_pin_(gdo2_pin), freq_mhz_(freq_mhz) {}

  void setup() override;
  void update() override;

  // optional helper to change frequency at runtime
  void set_frequency(float freq) {
    freq_mhz_ = freq;
    if (radio_) {
      radio_->setFrequency(freq_mhz_);
    }
  }

 protected:
  int cs_pin_;
  int gdo0_pin_;
  int gdo2_pin_;
  float freq_mhz_;

  // RadioLib CC1101 instance
  CC1101 *radio_{nullptr};

  // helper to convert bytes to hex string
  std::string bytes_to_hex(const uint8_t *buf, size_t len);
};
