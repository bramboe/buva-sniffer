# BUVA RF Sniffer - CC1101 Component for ESPHome

This is a custom ESPHome component for using a CC1101 radio module to sniff RF packets, specifically designed for monitoring BUVA/Zehnder ventilation systems.

## Hardware Requirements

- ESP32 development board
- CC1101 radio module (868MHz or 433MHz depending on your region)
- Wiring between ESP32 and CC1101 (SPI connection)

## Wiring

Default pin configuration (can be changed in your YAML):

| CC1101 Pin | ESP32 Pin |
|------------|-----------|
| SCK        | GPIO18    |
| MOSI       | GPIO23    |
| MISO       | GPIO19    |
| CS         | GPIO5     |
| GDO0       | GPIO4     |
| GDO2       | GPIO15    |
| VCC        | 3.3V      |
| GND        | GND       |

## Installation

### Option 1: Use in ESPHome Configuration (Recommended)

Add this to your ESPHome YAML configuration:

```yaml
# SPI bus configuration
spi:
  clk_pin: GPIO18
  mosi_pin: GPIO23
  miso_pin: GPIO19

# Include the external component
external_components:
  - source: github://bramboe/buva-sniffer
    components: [ cc1101_sniffer ]

# Configure the CC1101 sniffer
cc1101_sniffer:
  cs_pin: GPIO5
  gdo0_pin: GPIO4
  gdo2_pin: GPIO15
  frequency: 868.3  # MHz
  update_interval: 200ms
```

See `example.yaml` for a complete configuration example.

## Configuration Options

- `cs_pin` (Required): Chip select pin for SPI
- `gdo0_pin` (Required): GDO0 pin from CC1101 (data input)
- `gdo2_pin` (Optional): GDO2 pin from CC1101
- `frequency` (Optional, default: 868.3): Frequency in MHz (300-928 MHz range)
- `update_interval` (Optional, default: 200ms): How often to poll for new packets

## Component Structure

```
components/
  cc1101_sniffer/
    __init__.py                      # ESPHome component configuration
    cc1101_sniffer_component.h       # C++ header
    cc1101_sniffer_component.cpp     # C++ implementation
```

## Dependencies

This component uses [RadioLib](https://github.com/jgromes/RadioLib) for CC1101 communication. The library is automatically downloaded by PlatformIO when you add it to your ESPHome configuration:

```yaml
esphome:
  libraries:
    - "jgromes/RadioLib"
```

## Features

- Continuous RF packet sniffing on CC1101
- Configurable frequency (868 MHz for EU, 433 MHz for some regions)
- RSSI (signal strength) reporting
- Hex dump of received packets
- Integration with Home Assistant via ESPHome

## Troubleshooting

### "ambiguous argument 'HEAD'" error

If you get this error, make sure:
1. The `__init__.py` file exists in `components/cc1101_sniffer/`
2. Your changes are committed and pushed to GitHub
3. Try adding `refresh: 0s` to force refresh the external component

### No packets received

- Check your wiring (especially GDO0 and CS pins)
- Verify the frequency matches your target device
- Check if there's actually RF traffic on that frequency
- Try adjusting the modulation settings in the .cpp file

## License

Open source - feel free to use and modify.

