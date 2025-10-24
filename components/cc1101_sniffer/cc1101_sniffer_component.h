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
  void dump_config() override;

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
  
  // Enable/disable frequency scanning
  void set_scan_mode(bool enable) { scan_mode_ = enable; }

 protected:
  int cs_pin_{-1};
  int gdo0_pin_{-1};
  int gdo2_pin_{-1};
  float freq_mhz_{868.3};

  // RadioLib CC1101 instance
  CC1101 *radio_{nullptr};
  
  // Track initialization status
  bool init_attempted_{false};
  bool init_success_{false};
  int16_t init_error_{0};
  uint8_t chip_version_{0};
  
  // Frequency scanning
  bool scan_mode_{true};  // Enable by default
  size_t scan_index_{0};
  int scan_dwell_counter_{0};
  float detected_frequency_{0};
  
  // Comprehensive frequency list for scanning (in MHz)
  std::vector<float> scan_frequencies_ = {
    // 433 MHz ISM band (very common for remotes)
    433.05f,  // Lower 433 band
    433.42f,  // EU 433 alternative
    433.92f,  // EU 433 primary (MOST COMMON!)
    434.42f,  // Upper 433 band
    434.79f,  // Upper edge 433 band
    
    // 868 MHz SRD band (Smart Home, sensors)
    863.00f,  // Lower edge
    865.00f,  // Mid-low
    867.00f,  // Lower 868
    868.00f,  // 868 band start
    868.30f,  // EU 868 primary
    868.35f,  // EU 868 common
    868.50f,  // Mid 868
    868.70f,  // Upper-mid 868
    868.95f,  // EU 868 LoRa/SRD
    869.20f,  // 869 band
    869.50f,  // Mid 869
    869.85f,  // Upper 869
    870.00f,  // Upper edge
    
    // 315 MHz (US/Asia - less common in EU)
    315.00f,
    
    // 915 MHz (US ISM band - if hardware supports)
    915.00f,
  };

  // helper to convert bytes to hex string
  std::string bytes_to_hex(const uint8_t *buf, size_t len);
};

}  // namespace cc1101_sniffer
}  // namespace esphome
