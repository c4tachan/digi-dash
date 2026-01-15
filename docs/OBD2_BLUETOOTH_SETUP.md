# OBD-II Bluetooth Connection Setup

This guide explains how to connect your digi-dash simulator to a real OBD-II Bluetooth adapter (ELM327 or compatible).

## Hardware Requirements

- ELM327 Bluetooth OBD-II adapter (v1.5 or newer recommended)
- Vehicle with OBD-II port (1996+ for US vehicles)
- Linux computer with Bluetooth support

## Software Setup

### 1. Install Required Packages

```bash
sudo apt-get update
sudo apt-get install bluetooth bluez bluez-tools rfcomm
```

### 2. Find Your OBD-II Adapter

1. Turn on your vehicle's ignition (engine doesn't need to be running)
2. The ELM327 adapter should power on and enter pairing mode (LED blinking)
3. Scan for Bluetooth devices:

```bash
bluetoothctl
# Inside bluetoothctl:
power on
agent on
default-agent
scan on
```

Look for a device named something like "OBDII", "ELM327", "OBD2", "CHX", etc. Note the MAC address (format: XX:XX:XX:XX:XX:XX).

### 3. Pair and Connect

```bash
# Still in bluetoothctl:
pair XX:XX:XX:XX:XX:XX
# Enter PIN if prompted (common PINs: 1234, 0000, 6789)
trust XX:XX:XX:XX:XX:XX
exit
```

### 4. Create RFCOMM Serial Connection

```bash
# Bind the Bluetooth device to a serial port
sudo rfcomm bind /dev/rfcomm0 XX:XX:XX:XX:XX:XX 1

# Verify connection
ls -l /dev/rfcomm0
```

### 5. Set Permissions (Optional)

To avoid needing sudo every time:

```bash
sudo usermod -a -G dialout $USER
# Log out and back in for group changes to take effect
```

## Running the Simulator with OBD-II

### With Mock Data (Default)

```bash
cd /home/c4tachan/projects/digi-dash/simulator/build
./digi-dash-simulator --mock
```

### With Real OBD-II Data

```bash
cd /home/c4tachan/projects/digi-dash/simulator/build
./digi-dash-simulator --obd2 /dev/rfcomm0
```

### View Help

```bash
./digi-dash-simulator --help
```

## Troubleshooting

### Connection Failed

**Symptom**: "Failed to open /dev/rfcomm0: No such file or directory"

**Solution**: 
- Verify adapter is powered on and paired
- Run `sudo rfcomm bind /dev/rfcomm0 XX:XX:XX:XX:XX:XX 1` again
- Check `hcitool con` to verify Bluetooth connection

### Permission Denied

**Symptom**: "Failed to open /dev/rfcomm0: Permission denied"

**Solution**:
```bash
# Temporary fix:
sudo chmod 666 /dev/rfcomm0

# Permanent fix:
sudo usermod -a -G dialout $USER
# Then log out and back in
```

### No Response from Adapter

**Symptom**: "OBD2: No response received"

**Solutions**:
- Ensure vehicle ignition is ON
- Try different baud rate (ELM327 adapters use 38400 or 9600)
- Some adapters need the engine running to respond
- Verify adapter is ELM327 compatible (cheap clones may have issues)

### Disconnect After Setup

To disconnect and free up the serial port:

```bash
sudo rfcomm release /dev/rfcomm0
```

## Alternative: USB OBD-II Adapters

If using a USB OBD-II adapter instead:

```bash
# Find the device (usually /dev/ttyUSB0)
ls /dev/ttyUSB*

# Run simulator with USB adapter
./digi-dash-simulator --obd2 /dev/ttyUSB0
```

## Supported OBD-II PIDs

The simulator currently queries these standard PIDs:

| Parameter | PID | Description |
|-----------|-----|-------------|
| RPM | 010C | Engine RPM (revolutions per minute) |
| Speed | 010D | Vehicle speed (km/h) |
| Coolant Temp | 0105 | Engine coolant temperature (°C) |

## Auto-Reconnect on Startup

Create a systemd service or add to your shell rc file:

```bash
# Add to ~/.bashrc or create a script
alias digi-dash-obd="sudo rfcomm bind /dev/rfcomm0 XX:XX:XX:XX:XX:XX 1 && cd ~/projects/digi-dash/simulator/build && ./digi-dash-simulator --obd2 /dev/rfcomm0"
```

## Testing Without a Vehicle

If you want to test the OBD-II code without a real vehicle:

1. Use the mock data mode: `./digi-dash-simulator --mock`
2. Or use an OBD-II simulator software with a virtual serial port

## Notes

- First connection may take longer (up to 5 seconds) for ELM327 initialization
- Query rate is limited to ~50ms between commands to avoid overloading the adapter
- Connection will auto-reconnect if lost
- RFCOMM binding persists until reboot or manual release
