# CC1101 Sniffer Troubleshooting Guide

## Quick Diagnostic Checklist

After flashing the firmware, check your ESPHome logs for these indicators:

### ✅ Good Signs (Everything Working)
```
[cc1101_sniffer]: ✅ CC1101 initialized successfully!
[cc1101_sniffer]: CC1101 chip version: 0x14 (should be 0x14)
[cc1101_sniffer]: ✅ Listening on 868.300 MHz (OOK mode)
[cc1101_sniffer]: 🔍 Status check - RSSI: -95.2 dBm (waiting for packets...)
```

### ❌ Bad Signs (Problems)
```
[cc1101_sniffer]: ❌ CC1101 begin() FAILED! Error code=-2
[cc1101_sniffer]: ⚠️ Unexpected chip version! Expected 0x14, got 0x00
```

---

## Problem 1: CC1101 Not Initializing

### Symptoms:
```
❌ CC1101 begin() FAILED! Error code=-2
```
or
```
CC1101 chip version: 0x00 (should be 0x14)
```

### Cause:
SPI communication failure - CC1101 not responding

### Solutions:

#### Check Power Supply
- **VCC**: Connect to **3.3V** (NOT 5V! This will damage the CC1101)
- **GND**: Connect to ESP32 GND
- Use a multimeter to verify 3.3V on the VCC pin
- Some CC1101 modules need a separate 3.3V regulator if your ESP32 can't provide enough current

#### Verify SPI Wiring

**Standard ESP32 SPI Pins:**
| CC1101 Pin | ESP32 Pin | GPIO | Purpose |
|------------|-----------|------|---------|
| VCC        | 3.3V      | -    | Power (3.3V ONLY!) |
| GND        | GND       | -    | Ground |
| MOSI       | MOSI      | 23   | Data ESP32→CC1101 |
| MISO       | MISO      | 19   | Data CC1101→ESP32 |
| SCK/CLK    | SCK       | 18   | SPI Clock |
| CS/CSN     | GPIO5     | 5    | Chip Select |
| GDO0       | GPIO4     | 4    | Data/Interrupt |
| GDO2       | GPIO15    | 15   | Optional |

**Common Wiring Mistakes:**
- ❌ VCC connected to 5V instead of 3.3V
- ❌ Swapped MOSI and MISO
- ❌ Loose or poor quality jumper wires
- ❌ Wrong pin numbers in configuration

#### Check Your Configuration
Make sure your `buva_sniffer.yaml` matches your actual wiring:
```yaml
spi:
  clk_pin: GPIO18   # Must match physical connection
  mosi_pin: GPIO23
  miso_pin: GPIO19

cc1101_sniffer:
  cs_pin: GPIO5     # Chip select
  gdo0_pin: GPIO4   # Data pin
  gdo2_pin: GPIO15  # Optional
```

---

## Problem 2: No Packets Received

### Symptoms:
```
✅ CC1101 initialized successfully!
🔍 Status check - RSSI: -95.2 dBm (waiting for packets...)
```
(But no packets when pressing buttons)

### Step-by-Step Diagnosis:

#### A. Verify CC1101 is Working
If you see `✅ CC1101 initialized successfully!` and `CC1101 chip version: 0x14`, the hardware is working!

#### B. Check for RF Activity
When pressing buttons on your BUVA remote, look for:
```
📡 RF ACTIVITY DETECTED! RSSI jumped to -65.3 dBm
```

**If you see RF activity:**
- ✅ CC1101 is receiving something
- ❌ Wrong modulation or settings
- Try changing settings (see below)

**If you don't see RF activity:**
- Remote might not be transmitting
- Wrong frequency
- Antenna issue

#### C. Check Frequency

BUVA Q-Stream typically uses **868 MHz** in Europe. Check your configuration:
```yaml
cc1101_sniffer:
  frequency: 868.3  # Try these: 868.3, 868.95, 433.92
```

**Common frequencies to try:**
- `868.3` - Most common EU 868 band
- `868.95` - Alternative EU frequency
- `433.92` - If your remote uses 433 MHz band

#### D. Check Antenna

**CC1101 modules usually come with:**
- Spring antenna (curly wire) - 10-20m range
- Wire antenna (straight wire) - 5-10m range
- PCB antenna (on module) - 1-5m range

**For 868 MHz:** Antenna should be ~8.2 cm (1/4 wavelength)
**For 433 MHz:** Antenna should be ~17.3 cm (1/4 wavelength)

**Test:**
1. Make sure antenna is connected to the ANT pin
2. Try holding the remote **very close** (< 1 meter) to the CC1101
3. Press button multiple times

#### E. Test with Known RF Source

Test if your CC1101 can receive anything:
1. Use a different 868MHz remote (garage door opener, weather station, etc.)
2. Press buttons close to the CC1101
3. If you see RF activity, your CC1101 works!

---

## Problem 3: Wrong Modulation

BUVA Q-Stream might use different modulation schemes. The firmware now uses **OOK (On-Off Keying)** by default.

### Try Different Settings

Edit the component code to try FSK instead:

In `cc1101_sniffer_component.cpp`, change:
```cpp
// Try OOK modulation first (common for simple remotes)
int16_t ret = radio_->setOOK(true);
```

To FSK/2-FSK:
```cpp
// Try FSK modulation
int16_t ret = radio_->setOOK(false);  // Use FSK instead
```

Or try different bandwidths:
```cpp
radio_->setRxBandwidth(58.0);   // Narrower (less noise)
radio_->setRxBandwidth(200.0);  // Current (default)
radio_->setRxBandwidth(812.0);  // Wider (catch more)
```

---

## Problem 4: BUVA Q-Stream Specific Issues

### What is BUVA Q-Stream?

BUVA Q-Stream is a ventilation control system. The RF remote might:
- Only transmit when button is pressed (not continuous)
- Use a proprietary protocol
- Transmit very short bursts
- Use encryption or rolling codes

### Known Information:
Based on similar systems, BUVA likely uses:
- **Frequency:** 868.3 MHz or 868.95 MHz (EU)
- **Modulation:** OOK or FSK
- **Short packets:** 10-50 bytes
- **Button press only:** No continuous transmission

### Testing Strategy:

1. **Press and HOLD** buttons on the remote (some remotes send multiple packets)
2. **Try different buttons** - some might use different codes
3. **Check distance** - start very close (<50cm) then move away
4. **Try different frequencies** - some remotes can use multiple channels

### Check Logs for:
```
📡 RF ACTIVITY DETECTED!  ← Any RF signal (even if can't decode)
📦 Packet available!      ← CC1101 detected a packet
✅ PACKET RECEIVED!       ← Successfully decoded packet
```

---

## Advanced Debugging

### Enable More Verbose Logging

In your YAML:
```yaml
logger:
  level: VERBOSE  # or VERY_VERBOSE
  logs:
    cc1101_sniffer: VERBOSE
```

### Monitor RSSI in Real-Time

The component now logs RSSI every 10 seconds:
```
🔍 Status check - RSSI: -95.2 dBm (waiting for packets...)
```

**Typical RSSI values:**
- `-95 to -100 dBm` - Noise floor (no signal)
- `-70 to -90 dBm` - Weak signal (might work)
- `-40 to -70 dBm` - Good signal (should work)
- `-20 to -40 dBm` - Strong signal (very close)

### Verify SPI Communication

If chip version shows `0x00` or `0xFF`, SPI is not working:
- Check all wiring again
- Try different GPIO pins for CS (edit YAML)
- Test with a multimeter for continuity
- Try a different CC1101 module (might be defective)

---

## Next Steps After Successful Reception

Once you see packets, you need to:
1. **Decode the protocol** - Analyze the hex data
2. **Identify button patterns** - Press same button multiple times
3. **Reverse engineer** - Figure out the command structure
4. **Implement transmit** - Send your own commands

---

## Still Not Working?

### Post Your Logs
Include this information:
1. Full boot log showing CC1101 initialization
2. CC1101 chip version
3. RSSI values
4. Any error messages
5. Your exact wiring
6. Your YAML configuration

### Test Hardware
- Try a different CC1101 module (might be defective)
- Test ESP32 with a simple SPI device
- Check 3.3V voltage with multimeter
- Verify ground connection

### Alternative Approach
If CC1101 doesn't work, consider:
- RTL-SDR dongle for initial RF analysis
- Different RF module (RFM69, nRF24L01)
- Logic analyzer to debug SPI communication

