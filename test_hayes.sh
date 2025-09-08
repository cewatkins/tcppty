#!/bin/bash
# Test script for tcpser PTY Hayes modem functionality

PTY_DEVICE="/dev/pts/11"

echo "Testing Hayes AT commands on PTY device: $PTY_DEVICE"

# Test basic AT command
echo -e "AT\r" > $PTY_DEVICE &
sleep 1

# Test initialization command
echo -e "ATZ\r" > $PTY_DEVICE &
sleep 1

# Test dial command (to example.com:23 for telnet)
echo -e "ATDT example.com:23\r" > $PTY_DEVICE &
sleep 2

# Read any responses
timeout 2s cat $PTY_DEVICE

echo "Hayes command test completed"