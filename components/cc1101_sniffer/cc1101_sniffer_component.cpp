#include "cc1101_sniffer_component.h"
#include "esphome/core/log.h"
#include <SPI.h>
#include <sstream>
#include <iomanip>

namespace esphome {
namespace cc1101_sniffer {

static const char *const TAG = "cc1101_sniffer";

void CC1101SnifferComponent::setup() {
  ESP_LOGI(TAG, "=== CC1101 Sniffer Setup ===");
  ESP_LOGI(TAG, "Pins: CS=%d, GDO0=%d, GDO2=%d", cs_pin_, gdo0_pin_, gdo2_pin_);
  ESP_LOGI(TAG, "Frequency: %.3f MHz", freq_mhz_);

  // Create Module wrapper for RadioLib (GDO2 optional)
  Module *mod = new Module(cs_pin_, gdo0_pin_, gdo2_pin_);

  radio_ = new CC1101(mod);

  // Try to initialize CC1101
  int16_t state = radio_->begin();
  if (state != RADIOLIB_ERR_NONE) {
    ESP_LOGE(TAG, "❌ CC1101 begin() FAILED! Error code=%d", state);
    ESP_LOGE(TAG, "Check wiring: VCC->3.3V, GND->GND, CS->GPIO%d, MOSI->GPIO23, MISO->GPIO19, SCK->GPIO18", cs_pin_);
    this->mark_failed();
    return;
  }
  ESP_LOGI(TAG, "✅ CC1101 initialized successfully!");

  // Read and verify chip version (diagnostic)
  uint8_t version = radio_->getChipVersion();
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
    ESP_LOGI(TAG, "✅ Listening on %.3f MHz (OOK mode)", freq_mhz_);
    ESP_LOGI(TAG, "Press buttons on your remote to test...");
  }
}

void CC1101SnifferComponent::update() {
  if (!radio_) return;

  // Read current RSSI to detect any activity
  float current_rssi = radio_->getRSSI();
  
  // Log RSSI periodically to show we're alive (every ~50 updates = 10 seconds)
  static int counter = 0;
  if (++counter >= 50) {
    ESP_LOGI(TAG, "🔍 Status check - RSSI: %.1f dBm (waiting for packets...)", current_rssi);
    counter = 0;
  }
  
  // Detect any RF activity (signal strength spike)
  static float baseline_rssi = -100.0;
  if (baseline_rssi == -100.0) baseline_rssi = current_rssi;
  
  if (current_rssi > baseline_rssi + 10.0) {
    ESP_LOGI(TAG, "📡 RF ACTIVITY DETECTED! RSSI jumped to %.1f dBm", current_rssi);
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
