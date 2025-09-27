# TCPSER with Pseudo Terminal (PTY) Support

## Overview

This implementation adds pseudo terminal (PTY) support to tcpser, allowing you to use Hayes-compatible modem functionality through a PTY device instead of requiring a physical serial port.

## New Command Line Option

The `-P` option has been added to create or use a pseudo terminal:

```
-P   pseudo terminal (PTY) - auto-create or specify device (e.g. /dev/pts/15)
```

## Usage Examples

### Auto-Create PTY (Original Behavior)
```bash
# Create a PTY modem listening on default port 6400
./tcpser -P mymodem -s 38400

# Create a PTY modem with specific TCP port
./tcpser -P mymodem -s 38400 -p 2323
```

### Specify Existing PTY Device
```bash
# Use a specific PTY device
./tcpser -P /dev/pts/15 -s 38400

# Use a specific PTY device with custom TCP port
./tcpser -P /dev/pts/20 -s 38400 -p 2323
```

### Multiple Modems (PTY + Serial)
```bash
# First modem on PTY, second on serial port
./tcpser -P modem1 -s 38400 -d /dev/ttyS0 -s 9600
```

## How It Works

1. **Auto-Create Mode**: When using `-P` with a name hint (e.g., `-P mymodem`), tcpser creates a pseudo terminal pair automatically
2. **Specify Mode**: When using `-P` with a device path (e.g., `-P /dev/pts/15`), tcpser opens the specified PTY device
3. **Device Output**: The PTY device name is displayed (e.g., `Pseudo terminal device: /dev/pts/13`)
4. **Hayes Commands**: Connect to the PTY device to send AT commands
5. **Network Bridge**: Commands like `ATDT hostname:port` establish TCP connections

## Testing Hayes Functionality

### Method 1: Using minicom
```bash
# Connect to the PTY device (replace /dev/pts/13 with your device)
minicom -D /dev/pts/13 -b 38400
```

### Method 2: Using screen
```bash
screen /dev/pts/13 38400
```

### Method 3: Direct echo/cat test
```bash
# For auto-created PTY, use the device shown in tcpser output
# For specified PTY, use the device you specified

# In one terminal, read responses
cat /dev/pts/15 &

# In another terminal, send commands
echo -e "AT\r" > /dev/pts/15
echo -e "ATDT example.com:23\r" > /dev/pts/15
```

### Method 4: Using socat to create PTY pairs
```bash
# Create a PTY pair with socat for testing
socat -d -d pty,raw,echo=0 pty,raw,echo=0

# Use one of the PTY devices with tcpser
./tcpser -P /dev/pts/16 -s 38400
```

## Common Hayes Commands

- `AT` - Basic attention command (should respond "OK")
- `ATZ` - Reset modem to default settings
- `ATDT hostname:port` - Dial/connect to a TCP host and port
- `ATDT hostname` - Connect to host on default port 23 (telnet)
- `ATH` - Hang up connection
- `ATA` - Answer incoming call
- `+++` - Escape sequence to return to command mode

## Example Session

```
AT
OK

ATDT towel.blinkenlights.nl:23
CONNECT 38400

[Star Wars ASCII animation plays]

+++
OK

ATH
NO CARRIER
```

## Implementation Details

### Files Added/Modified

1. **New Files**:
   - `src/pty.h` - PTY interface definitions
   - `src/pty.c` - PTY implementation

2. **Modified Files**:
   - `src/dce.h` - Added PTY support to DCE configuration
   - `src/dce.c` - Integrated PTY functions into DCE layer
   - `src/init.c` - Added `-P` command line option
   - `Makefile` - Added PTY source files and -lutil library

### Key Features

- **Automatic PTY Creation**: Uses `openpty()` to create master/slave pair
- **Control Line Simulation**: Simulates DTR/DCD for compatibility
- **Hayes Protocol**: Full compatibility with existing Hayes command set
- **Flow Control**: Supports software flow control (XON/XOFF)
- **Multi-modem**: Can be combined with serial and IP232 modems

### Benefits

1. **No Hardware Required**: Works without physical serial ports
2. **Container Friendly**: Perfect for Docker/containerized environments
3. **Testing**: Ideal for development and testing scenarios
4. **Retro Computing**: Enables vintage systems to connect via modern networks
5. **BBS Systems**: Allows old BBS software to accept network connections

## Troubleshooting

### PTY Device Not Found
- Ensure you have permissions to create PTY devices
- Check that `/dev/pts` is mounted and accessible

### Hayes Commands Not Working
- Verify the PTY device name in the tcpser output
- Ensure line endings are correct (use `\r` not `\n`)
- Check that the terminal application supports the baud rate

### Connection Issues
- Verify the target host and port are accessible
- Check firewall settings
- Ensure tcpser is listening on the correct port

## Building

The PTY support requires the `util` library:

```bash
make clean
make
```

On some systems, you may need to install development packages:
```bash
# Ubuntu/Debian
sudo apt-get install libutil-dev

# CentOS/RHEL
sudo yum install libutempter-devel
```