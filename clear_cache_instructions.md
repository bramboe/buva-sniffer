# How to Clear ESPHome External Components Cache

## If running ESPHome in Home Assistant:

### Via SSH:
```bash
# Remove the cached external components
rm -rf /config/.esphome/external_components

# Or just remove this specific component:
rm -rf /config/.esphome/external_components/github.com/bramboe/buva-sniffer
```

### Via File Browser Add-on:
1. Enable "Show Hidden Files" in File Browser
2. Navigate to `/config/.esphome/external_components/`
3. Delete the `github.com` folder or the specific `bramboe` subfolder

## If running ESPHome standalone:
```bash
# In your ESPHome directory:
rm -rf .esphome/external_components
```

## Then:
1. Restart ESPHome (if running as add-on)
2. Try compiling your configuration again
3. ESPHome will download fresh from GitHub

## Verification:
After these steps, when you compile, you should see:
- "Cloning into..." message for your component
- No "fatal: ambiguous argument 'HEAD'" error
- The component should compile successfully

