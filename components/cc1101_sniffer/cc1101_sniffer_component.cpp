#include "cc1101_sniffer_component.h"
#include "esphome/core/log.h"
#include <SPI.h>
#include <sstream>
#include <iomanip>

namespace esphome {
namespace cc1101_sniffer {

static const char *const TAG = "cc1101_sniffer";

void CC1101SnifferComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "CC1101 Sniffer:");
  ESP_LOGCONFIG(TAG, "  CS Pin: GPIO%d", cs_pin_);
  ESP_LOGCONFIG(TAG, "  GDO0 Pin: GPIO%d", gdo0_pin_);
  ESP_LOGCONFIG(TAG, "  GDO2 Pin: GPIO%d", gdo2_pin_);
  ESP_LOGCONFIG(TAG, "  Base Frequency: %.3f MHz", freq_mhz_);
  
  if (scan_mode_) {
    ESP_LOGCONFIG(TAG, "  Scan Mode: ENABLED (scanning %d frequencies)", scan_frequencies_.size());
  } else {
    ESP_LOGCONFIG(TAG, "  Scan Mode: DISABLED (fixed frequency)");
  }
  
  // Show initialization status
  if (!init_attempted_) {
    ESP_LOGW(TAG, "  Status: Initialization NOT attempted!");
  } else if (init_success_) {
    ESP_LOGCONFIG(TAG, "  Status: ✅ Initialized OK");
    ESP_LOGCONFIG(TAG, "  Chip Version: 0x%02X", chip_version_);
  } else {
    ESP_LOGE(TAG, "  Status: ❌ Initialization FAILED!");
    ESP_LOGE(TAG, "  Error Code: %d", init_error_);
    ESP_LOGE(TAG, "  Chip Version: 0x%02X (should be 0x14)", chip_version_);
  }
  
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Component marked as FAILED");
  }
}

void CC1101SnifferComponent::setup() {
  ESP_LOGI(TAG, "");
  ESP_LOGI(TAG, "==============================================");
  ESP_LOGI(TAG, "=== CC1101 Sniffer Setup Starting ===");
  ESP_LOGI(TAG, "==============================================");
  ESP_LOGI(TAG, "Pins: CS=%d, GDO0=%d, GDO2=%d", cs_pin_, gdo0_pin_, gdo2_pin_);
  ESP_LOGI(TAG, "Frequency: %.3f MHz", freq_mhz_);

  init_attempted_ = true;

  // Create Module wrapper for RadioLib (GDO2 optional)
  Module *mod = new Module(cs_pin_, gdo0_pin_, gdo2_pin_);

  radio_ = new CC1101(mod);

  // Try to initialize CC1101
  int16_t state = radio_->begin();
  init_error_ = state;
  
  if (state != RADIOLIB_ERR_NONE) {
    ESP_LOGE(TAG, "❌ CC1101 begin() FAILED! Error code=%d", state);
    ESP_LOGE(TAG, "Check wiring: VCC->3.3V, GND->GND, CS->GPIO%d, MOSI->GPIO23, MISO->GPIO19, SCK->GPIO18", cs_pin_);
    init_success_ = false;
    this->mark_failed();
    return;
  }
  ESP_LOGI(TAG, "✅ CC1101 initialized successfully!");
  init_success_ = true;

  // Read and verify chip version (diagnostic)
  uint8_t version = radio_->getChipVersion();
  chip_version_ = version;
  ESP_LOGI(TAG, "CC1101 chip version: 0x%02X (should be 0x14)", version);
  if (version != 0x14) {
    ESP_LOGW(TAG, "⚠️  Unexpected chip version! Expected 0x14, got 0x%02X", version);
    ESP_LOGW(TAG, "This might indicate wiring issues or a faulty CC1101 module");
  }

  // Configure for BUVA/Q-Stream - try multiple modulation schemes
  ESP_LOGI(TAG, "Configuring radio parameters...");
  radio_->setFrequency(freq_mhz_);
  
  // Try OOK modulation first (common for simple remotes)
  int16_t ret = radio_->setOOK(true);
  ESP_LOGI(TAG, "OOK modulation set: %d", ret);
  
  radio_->setBitRate(4.8);           // kbps - adjust if needed
  radio_->setRxBandwidth(200.0);     // kHz - wider = more noise but catches more
  
  ESP_LOGI(TAG, "Current RSSI: %.1f dBm", radio_->getRSSI());

  // Start listening
  int16_t r = radio_->startReceive();
  if (r != RADIOLIB_ERR_NONE) {
    ESP_LOGE(TAG, "❌ startReceive() FAILED: %d", r);
  } else {
    if (scan_mode_) {
      ESP_LOGI(TAG, "✅ Starting FREQUENCY SCANNER");
      ESP_LOGI(TAG, "   Will scan %d frequencies automatically", scan_frequencies_.size());
      ESP_LOGI(TAG, "   Press buttons on your remote NOW!");
      ESP_LOGI(TAG, "");
      ESP_LOGI(TAG, "📡 Frequencies to scan:");
      for (size_t i = 0; i < scan_frequencies_.size(); i++) {
        ESP_LOGI(TAG, "   %d. %.2f MHz", i+1, scan_frequencies_[i]);
      }
      ESP_LOGI(TAG, "==============================================");
    } else {
      ESP_LOGI(TAG, "✅ Listening on %.3f MHz (OOK mode)", freq_mhz_);
      ESP_LOGI(TAG, "Press buttons on your remote to test...");
    }
  }
}

void CC1101SnifferComponent::update() {
  if (!radio_ || !init_success_) {
    static int error_counter = 0;
    if (++error_counter >= 50) {
      if (!init_attempted_) {
        ESP_LOGE(TAG, "❌ Setup was never called!");
      } else if (!init_success_) {
        ESP_LOGE(TAG, "❌ Init failed! Error: %d, Chip: 0x%02X", init_error_, chip_version_);
      }
      error_counter = 0;
    }
    return;
  }

  // FREQUENCY SCANNING MODE
  if (scan_mode_ && detected_frequency_ == 0) {
    // Increment dwell counter
    scan_dwell_counter_++;
    
    // Switch frequency every 10 updates (2 seconds at 200ms update)
    if (scan_dwell_counter_ >= 10) {
      scan_dwell_counter_ = 0;
      
      // Move to next frequency
      float new_freq = scan_frequencies_[scan_index_];
      radio_->setFrequency(new_freq);
      radio_->startReceive();
      
      ESP_LOGI(TAG, "📡 Scanning %.2f MHz [%d/%d]", 
               new_freq, scan_index_ + 1, scan_frequencies_.size());
      
      // Move to next frequency for next cycle
      scan_index_ = (scan_index_ + 1) % scan_frequencies_.size();
    }
  }

  // Read current RSSI to detect any activity
  float current_rssi = radio_->getRSSI();
  
  // Detect any RF activity (signal strength spike)
  static float baseline_rssi = -100.0;
  if (baseline_rssi == -100.0) baseline_rssi = current_rssi;
  
  // Detect significant RSSI spike (RF activity!)
  if (current_rssi > baseline_rssi + 10.0) {
    float detected_freq = scan_frequencies_[scan_index_ > 0 ? scan_index_ - 1 : scan_frequencies_.size() - 1];
    ESP_LOGW(TAG, "");
    ESP_LOGW(TAG, "🎯🎯🎯 RF ACTIVITY DETECTED! 🎯🎯🎯");
    ESP_LOGW(TAG, "   Frequency: %.2f MHz", detected_freq);
    ESP_LOGW(TAG, "   RSSI: %.1f dBm (was %.1f dBm)", current_rssi, baseline_rssi);
    ESP_LOGW(TAG, "");
    
    // If in scan mode, lock onto this frequency
    if (scan_mode_ && detected_frequency_ == 0) {
      detected_frequency_ = detected_freq;
      radio_->setFrequency(detected_frequency_);
      radio_->startReceive();
      ESP_LOGW(TAG, "🔒 LOCKED onto %.2f MHz - will monitor this frequency", detected_frequency_);
      ESP_LOGW(TAG, "Press buttons again to capture packets!");
      ESP_LOGW(TAG, "");
    }
  }
  baseline_rssi = baseline_rssi * 0.99 + current_rssi * 0.01; // slow moving average

  // check for incoming packet
  int16_t available = radio_->available();
  if (available > 0) {
    ESP_LOGI(TAG, "📦 Packet available! Size: %d bytes", available);
    
    // allocate a reasonable buffer
    uint8_t buffer[128];
    int16_t err = radio_->readData(buffer, sizeof(buffer));
    if (err == RADIOLIB_ERR_NONE) {
      int len = radio_->getPacketLength();
      float rssi = radio_->getRSSI();

      std::string hex = bytes_to_hex(buffer, len);

      // Log the received packet (visible in Home Assistant logs)
      ESP_LOGI(TAG, "✅ PACKET RECEIVED!");
      ESP_LOGI(TAG, "   Length: %d bytes", len);
      ESP_LOGI(TAG, "   RSSI: %.1f dBm", rssi);
      ESP_LOGI(TAG, "   Data: %s", hex.c_str());
    } else if (err != RADIOLIB_ERR_RX_TIMEOUT) {
      ESP_LOGW(TAG, "readData error: %d", err);
    }
  }

  // restart receive for next packet
  radio_->startReceive();
}

std::string CC1101SnifferComponent::bytes_to_hex(const uint8_t *buf, size_t len) {
  std::ostringstream oss;
  oss << std::hex << std::uppercase << std::setfill('0');
  for (size_t i = 0; i < len; ++i) {
    oss << std::setw(2) << static_cast<int>(buf[i]);
    if (i + 1 < len) oss << " ";
  }
  return oss.str();
}

}  // namespace cc1101_sniffer
}  // namespace esphome
