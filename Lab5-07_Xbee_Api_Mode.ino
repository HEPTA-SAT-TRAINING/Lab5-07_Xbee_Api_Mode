#include "src/HeptaSat.h"

HeptaCdh cdh;
HeptaCom com;

uint32_t count = 0;

void setup() {
  cdh.begin();

  /* Temporarily set AP=1 (not written with ATWR). Payload send/receive
     uses API frames internally; the sketch API is the same as Lab5-01/02. */
  com.begin(XBEE_MODE_API);

  /* Default destination is broadcast (000000000000FFFF).
     To unicast, pass the peer DH/DL from XCTU:
     com.set_destination(0x0013A200, 0x41234567); */
  com.set_destination_broadcast();

  cdh.println("XBee API mode started (1 second interval)");
}

void loop() {
  if (com.available()) {
    char cmd = com.get_char();
    com.printf("command = %c\r\n", cmd);
    cdh.printf("command = %c\r\n", cmd);
  } else {
    count++;

    com.printf("count from COM = %lu\r\n", (unsigned long)count);
    cdh.printf("count from CDH = %lu\r\n", (unsigned long)count);
  }

  delay(1000);
}
