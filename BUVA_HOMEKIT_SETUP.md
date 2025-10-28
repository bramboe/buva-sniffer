# BUVA Ventilation HomeKit Control via Shelly Dimmer

This setup allows you to control your BUVA ventilation system using a Shelly Gen 3 Dimmer and expose it to HomeKit as a proper **Fan** entity (not a light).

## Hardware Setup

- **Shelly Plus Dimmer PM Gen3** (or similar Shelly dimmer)
- Connected to BUVA ventilation control input
- Integrated into Home Assistant

## How It Works

### Brightness to Speed Mapping

The Shelly dimmer brightness controls ventilation speed:

| Brightness | Fan Speed | Fan Percentage |
|------------|-----------|----------------|
| 0%         | Off       | 0%             |
| 25%        | Low       | 33%            |
| 50%        | Medium    | 67%            |
| 75%        | High      | 100%           |

### Why 75% Maximum?

Using only 0-75% brightness range:
- ✅ Prevents accidentally maxing out the dimmer
- ✅ Provides headroom for the dimmer electronics
- ✅ Matches typical ventilation 3-speed control
- ✅ Cleaner percentage mapping

## Configuration

### Template Fan Entity

Located in `configuration.yaml`:

```yaml
template:
  - fan:
      - name: "BUVA Ventilation"
        unique_id: buva_ventilation_control
```

**Key Features:**
- ✅ Exposed as `fan.buva_ventilation` (not light)
- ✅ 3 preset modes: Low, Medium, High
- ✅ Smooth percentage control (0-100%)
- ✅ Auto-maps to 0-75% brightness on Shelly
- ✅ `speed_count: 3` for proper HomeKit display

### HomeKit Configuration

```yaml
homekit:
  - name: Home Assistant Bridge
    filter:
      include_entities:
        - fan.buva_ventilation
      exclude_entities:
        - light.paradise_buva  # Hide raw Shelly from HomeKit
```

**What This Does:**
- ✅ Exposes fan entity to HomeKit
- ✅ Hides the underlying Shelly light
- ✅ Shows as **Fan** in HomeKit (with fan icon)
- ✅ Supports on/off and 3 speed levels

## Key Improvements Made

### 1. Fixed Percentage Calculation ✅

**Before (broken):**
```yaml
percentage: >
  {{ ((state_attr('light.paradise_buva', 'brightness') | float / 255 * 100) / 0.75) | int }}
```

**After (correct):**
```yaml
percentage: >
  {% set brightness_pct = state_attr('light.paradise_buva', 'brightness') | float / 255 * 100 %}
  {{ (brightness_pct / 0.75) | round(0) | int }}
```

### 2. Added Speed Count ✅

```yaml
speed_count: 3
```

This tells HomeKit the fan has 3 discrete speeds, improving the UI.

### 3. Fixed Preset Mode Percentages ✅

Now correctly maps to your defined speeds:
- **Low**: 25% brightness
- **Medium**: 50% brightness  
- **High**: 75% brightness

### 4. Added Preset Mode Getter ✅

Shows current fan mode in Home Assistant UI and automations.

### 5. Excluded Underlying Light from HomeKit ✅

Prevents duplicate controls in HomeKit (one fan, not a fan AND a light).

### 6. Better Entity Naming ✅

- Entity ID: `fan.buva_ventilation`
- HomeKit name: "BUVA Ventilation"

## Using the Fan

### In Home Assistant

**Lovelace Card:**
```yaml
type: fan
entity: fan.buva_ventilation
name: BUVA Ventilation
```

**Set Speed:**
```yaml
service: fan.set_percentage
target:
  entity_id: fan.buva_ventilation
data:
  percentage: 67  # Medium speed
```

**Set Preset:**
```yaml
service: fan.set_preset_mode
target:
  entity_id: fan.buva_ventilation
data:
  preset_mode: High
```

### In HomeKit

1. Open **Home** app on iPhone/iPad
2. Find **"BUVA Ventilation"**
3. Tap to toggle on/off
4. Long-press for speed slider
5. Choose from 3 speeds: Low, Medium, High

### In Automations

**Example: Turn on at medium speed when CO2 is high**
```yaml
automation:
  - alias: "Ventilation: High CO2"
    trigger:
      - platform: numeric_state
        entity_id: sensor.living_room_co2
        above: 1000
    action:
      - service: fan.set_preset_mode
        target:
          entity_id: fan.buva_ventilation
        data:
          preset_mode: High
```

## Troubleshooting

### Fan Not Showing in HomeKit

1. **Check HomeKit Integration:**
   - Settings → Devices & Services → HomeKit
   - Verify bridge is running

2. **Restart HomeKit Bridge:**
   ```yaml
   service: homekit.reload
   ```

3. **Check Entity Name:**
   - Entity must be `fan.buva_ventilation`
   - If you changed the name, update `homekit` config

4. **Verify Filter:**
   ```yaml
   include_entities:
     - fan.buva_ventilation  # Must match exact entity_id
   ```

### Fan Shows as Light in HomeKit

- Ensure you're exposing `fan.buva_ventilation`, NOT `light.paradise_buva`
- Exclude the light entity explicitly
- Reload HomeKit configuration

### Speed Control Not Working

1. **Check Template:**
   - Developer Tools → Template
   - Test percentage calculation manually

2. **Verify Shelly Connection:**
   - Check `light.paradise_buva` is available
   - Test brightness control directly

3. **Check Logs:**
   ```
   Settings → System → Logs
   Filter: homekit
   ```

### Percentage Mapping Issues

**Current mapping:**
```
Fan 0%   → Shelly 0%   → Off
Fan 33%  → Shelly 25%  → Low
Fan 67%  → Shelly 50%  → Medium  
Fan 100% → Shelly 75%  → High
```

If speeds don't match your BUVA system, adjust in `set_percentage`:
```yaml
brightness_pct: >
  {{ (stepped * 0.75) | int }}
```

## Advanced: Customization

### Adding a 4th Speed (Max)

```yaml
preset_modes:
  - Low
  - Medium
  - High
  - Max

# Add to set_preset_mode:
- conditions: "{{ preset_mode == 'Max' }}"
  sequence:
    - service: light.turn_on
      target:
        entity_id: light.paradise_buva
      data:
        brightness_pct: 100  # Use full dimmer range
```

### Adding Boost Mode (Temporary High Speed)

```yaml
script:
  ventilation_boost:
    alias: "Ventilation Boost (15 min)"
    sequence:
      - service: fan.set_preset_mode
        target:
          entity_id: fan.buva_ventilation
        data:
          preset_mode: High
      - delay:
          minutes: 15
      - service: fan.set_preset_mode
        target:
          entity_id: fan.buva_ventilation
        data:
          preset_mode: Medium
```

### Integration with Bathroom Humidity

```yaml
automation:
  - alias: "Ventilation: Bathroom Humidity"
    trigger:
      - platform: numeric_state
        entity_id: sensor.bathroom_humidity
        above: 70
    action:
      - service: fan.set_preset_mode
        target:
          entity_id: fan.buva_ventilation
        data:
          preset_mode: High
      - wait_template: "{{ states('sensor.bathroom_humidity') | float < 60 }}"
        timeout: '01:00:00'
      - service: fan.set_preset_mode
        target:
          entity_id: fan.buva_ventilation
        data:
          preset_mode: Low
```

## Files

- `configuration.yaml` - Main Home Assistant configuration
- `BUVA_HOMEKIT_SETUP.md` - This documentation
- `buva_sniffer.yaml` - ESPHome CC1101 sniffer (alternative RF approach)

## Why This Approach Works Better Than RF

**Advantages over RF sniffing:**
- ✅ No need to reverse-engineer RF protocol
- ✅ Works immediately with standard hardware
- ✅ Reliable (wired connection)
- ✅ Bidirectional feedback (know current state)
- ✅ Easy to set up and maintain
- ✅ No battery concerns
- ✅ Perfect HomeKit integration

**Use Cases:**
- Smart home automation
- Voice control (Siri)
- Scheduling
- Humidity/CO2 triggered ventilation
- Integration with other HomeKit devices

---

**Enjoy your smart BUVA ventilation control!** 🎯

