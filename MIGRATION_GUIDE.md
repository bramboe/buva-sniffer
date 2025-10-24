# Migration Guide: ESPHome 2025 Syntax

## What Changed

ESPHome 2025 removed the old `platform` key from the `esphome` block. Here's what you need to update:

### ❌ Old Syntax (Deprecated)
```yaml
esphome:
  name: my-device
  platform: ESP32
  board: esp32dev
  libraries:
    - "jgromes/RadioLib"
```

### ✅ New Syntax (ESPHome 2025+)
```yaml
esphome:
  name: my-device
  platformio_options:
    lib_deps:
      - "jgromes/RadioLib"

esp32:
  board: esp32dev
  framework:
    type: arduino
```

## Key Changes Summary

1. **Removed**: `platform: ESP32` from `esphome` block
2. **Added**: Separate `esp32` block with `board` and `framework`
3. **Changed**: `libraries` → `platformio_options.lib_deps`
4. **Updated**: `external_components` source to use `@main` branch explicitly

## For Your Configuration

Copy the updated `buva_sniffer.yaml` to your Home Assistant ESPHome folder at:
```
/config/esphome/zehnder-comfofan-2.yaml
```

Or use the simplified `example.yaml` as a starting point.

## Complete Working Example

See `example.yaml` in this repository for a fully working configuration using:
- ✅ ESPHome 2025 syntax
- ✅ Proper external_components with branch specification
- ✅ Simplified cc1101_sniffer component configuration (no custom_component needed!)

## If You're Using ESP8266 Instead

For ESP8266 boards, use:
```yaml
esp8266:
  board: nodemcuv2  # or your specific board
  framework:
    version: recommended
```

## Troubleshooting

### "Please remove the `platform` key"
- Update to the new syntax shown above
- Use the `esp32` or `esp8266` block instead

### "fatal: ambiguous argument 'HEAD'"
- Make sure you're using `github://bramboe/buva-sniffer@main`
- Clear cache: `rm -rf /config/.esphome/external_components`

### Component Not Found
- Ensure your GitHub repository has the `__init__.py` file (it's there now!)
- Use `refresh: 1s` in external_components to force update

