#include "cc1101_sniffer_component.h"
#include "esphome/core/log.h"
#include <SPI.h>
#include <sstream>
#include <iomanip>

namespace esphome {
namespace cc1101_sniffer {

static const char *const TAG = "cc1101_sniffer";

void CC1101SnifferComponent::setup() {
  ESP_LOGI(TAG, "Setup CC1101 sniffer (CS=%d GDO0=%d GDO2=%d Freq=%.3f)",
           cs_pin_, gdo0_pin_, gdo2_pin_, freq_mhz_);

  // Initialize text sensor for packet data
  if (!packet_text_sensor) {
    packet_text_sensor = new esphome::text_sensor::TextSensor();
  }

  // Create Module wrapper for RadioLib (GDO2 optional)
  Module *mod = new Module(cs_pin_, gdo0_pin_, gdo2_pin_);

  radio_ = new CC1101(mod);

  int16_t state = radio_->begin();
  if (state != RADIOLIB_ERR_NONE) {
    ESP_LOGE(TAG, "CC1101 begin() failed, code=%d", state);
    this->mark_failed();
    return;
  }

  // Set some sensible defaults — tweak for BUVA/Q-Stream
  radio_->setFrequency(freq_mhz_);   // MHz
  // Note: RadioLib uses begin() with parameters for modulation
  // For now, using defaults. To change modulation, use:
  // radio_->setOOK(true); // for OOK
  radio_->setBitRate(4.8);           // kbps (4.8 kbps)
  radio_->setRxBandwidth(200.0);     // kHz
  radio_->setFrequencyDeviation(5.0); // kHz (used for FSK)

  // Start listening
  int16_t r = radio_->startReceive();
  if (r != RADIOLIB_ERR_NONE) {
    ESP_LOGW(TAG, "startReceive() returned %d", r);
  } else {
    ESP_LOGI(TAG, "Started receive at %.3f MHz", freq_mhz_);
  }
}

void CC1101SnifferComponent::update() {
  if (!radio_) return;

  // check for incoming packet
  int16_t available = radio_->available();
  if (available <= 0) {
    // nothing
    return;
  }

  // allocate a reasonable buffer
  uint8_t buffer[128];
  int16_t err = radio_->readData(buffer, sizeof(buffer));
  if (err == RADIOLIB_ERR_NONE) {
    int len = radio_->getPacketLength();
    float rssi = radio_->getRSSI();

    std::string hex = bytes_to_hex(buffer, len);

    ESP_LOGI(TAG, "RX len=%d RSSI=%.1f dBm: %s", len, rssi, hex.c_str());

    // publish to the text_sensor for HA visibility
    if (packet_text_sensor) {
      packet_text_sensor->publish_state(hex);
    }
  } else if (err == RADIOLIB_ERR_RX_TIMEOUT) {
    // ignore timeouts
  } else {
    ESP_LOGW(TAG, "readData error: %d", err);
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
