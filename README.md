# Lab5-07 · XBee networking

For HEPTA-SAT and HEPTA-SAT Lite with S2C TH Zigbee radios. Build the sketch as supplied for Full. For Lite, add `-DHEPTA_BOARD_LITE`. The ground station runs in Transparent mode at 38400 baud. The satellite uses API mode while this sketch is running.

## Use the paired ground station

Open the ground station in a serial monitor at 38400 baud. The paired satellite sends a count every second.

## Join another network

1. Open **XBee network setup** (`lab.html`) in desktop Chrome or Edge. Select **Read host ground station via USB** and choose the station running the network you want to join. Leave it powered. If you already know the network ID and channel, use **Enter settings manually**.
2. Power the joining satellite and connect its OBC by USB. Choose **Satellite → Connect device via USB → Review network change → Join network**. To move an additional ground station, repeat with **Ground station**. Joining devices take the router role. The host must allow new devices to join.
3. In the paired ground station’s serial monitor, send `TO 0013A20012345678` with LF or CR, substituting the destination satellite’s radio address. The satellite sends `HELLO`; the peer replies with `ACK`. The originating ground station displays the responding address. Routine telemetry continues to use the paired ground station.
4. In the same browser, reconnect each moved device and select **Restore original network**. Restore the ground station first so that it can form its original network. Select **Test radio link** to verify the return path.

The radio address identifies a physical XBee. The network ID (extended PAN ID) identifies a Zigbee network. The channel is the radio channel on which that network operates. The setup page reads both network values from the active host. Power loss clears temporary radio configuration.

## Hardware verification · 2026-11-18

Two Full satellites and two USB ground stations were tested with S2C TH radios (HV `2E4E`, VR `4061`). The two original networks used IDs `0x20` and `0x21`, both on channel 24. The tested host allowed joining (NJ `FF`).

| Devices on the shared network | Verified result |
| --- | --- |
| Ground station A and satellites A and B | Both satellite loopbacks and the application ACK from B reached their destinations while ground station B stayed on its original network. |
| Both ground stations and both satellites | With ground station A as coordinator and B as router, directed requests in each direction returned the addressed ACK in 10 of 10 trials. |

Chrome Web Serial tests covered manual entry, both directions of satellite loopback, each ground station’s paired echo, and restoration after a page reload. A wrong-channel attempt on channel 23 reported a join failure. Restoring channel 24 re-established the original pair. The 16-bit operating PAN ID can change when a coordinator reforms its network. Lite verification covered compilation.
