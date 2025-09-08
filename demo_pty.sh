#!/bin/bash

echo "=== TCPSER PTY Support Demonstration ==="
echo ""

# Start tcpser with PTY support
echo "1. Starting tcpser with PTY support..."
./tcpser -P demo_modem -s 38400 -l 2 -p 6400 &
TCPSER_PID=$!

# Give it time to start
sleep 2

# Find the PTY device that was created
PTY_DEVICE=$(ps aux | grep tcpser | grep -v grep | head -1)
echo "2. TCPSER started with PID: $TCPSER_PID"

# Note: In a real scenario, you would get the PTY device from tcpser output
# For demo purposes, let's assume it created /dev/pts/X
echo ""
echo "3. To use the Hayes modem functionality:"
echo "   - Connect to the PTY device shown in tcpser output"
echo "   - Example: minicom -D /dev/pts/XX -b 38400"
echo "   - Or: screen /dev/pts/XX 38400"
echo ""

echo "4. Example Hayes commands to try:"
echo "   AT           - Basic attention command"
echo "   ATZ          - Reset modem"
echo "   ATDT host:port - Dial/connect to host"
echo "   ATH          - Hang up"
echo ""

echo "5. Example connection:"
echo "   ATDT towel.blinkenlights.nl:23"
echo "   (This connects to the famous Star Wars ASCII animation)"
echo ""

echo "Press Enter to stop the demonstration..."
read

# Clean up
kill $TCPSER_PID 2>/dev/null
echo "Demonstration complete!"