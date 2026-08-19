# Lab5-07: XBee API Mode

Firmware that downlinks a 1-second count over XBee in **API mode** (`AP=1`).
Uplink bytes from the ground XBee are echoed back, same pattern as Lab5-02.

The sketch calls `com.begin(XBEE_MODE_API)`. `printf` / `send` / `available`
stay payload-oriented; the library wraps Transmit Request (`0x10`) and unwraps
Receive Packet (`0x90`).

## XBee setup

Keep the **on-board XBee nonvolatile setting at `AP=0`** (the usual kit state).
`begin(XBEE_MODE_API)` sends `ATAP1` without `ATWR`, so a power cycle returns
the radio to Transparent mode.

The ground / XCTU XBee can stay in Transparent mode (`AP=0`). You should see
the count payload, not API frame bytes.

Optional unicast (XCTU `DH` / `DL` of the ground module):

```cpp
com.set_destination(0x0013A200, 0x41234567);
```

`com.set_destination_broadcast()` restores `000000000000FFFF`.

## Web Serial Monitor

Open in **Chrome or Edge**, then **Add Port** → **Connect** (38400 baud):

**https://hepta-sat-training.github.io/hepta-serial-viewer/**

You should see lines such as `count from COM = ...` every second. Sending a
character from the ground side prints `command = ...`.

## Firmware

Open `Lab5-07_Xbee_Api_Mode.ino` in the Arduino IDE and upload to your board.
For library and submodule setup, see [src/README.md](src/README.md).
