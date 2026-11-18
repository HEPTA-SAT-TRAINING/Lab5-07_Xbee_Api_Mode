#ifdef HEPTA_BOARD_LITE
#include "src/HeptaSatLite.h"
HeptaLiteCdh cdh;
HeptaLiteCom com;
HeptaLiteEps eps;
#else
#include "src/HeptaSat.h"
HeptaCdh cdh;
HeptaCom com;
HeptaEps eps;
#endif
#include "src/common/hepta_lab_usb.h"

HeptaLabUsb lab;
bool ready = false;
uint32_t last_count = 0;
uint32_t count = 0;
char ground_line[24];
size_t ground_used = 0;
bool ground_overflow = false;
uint32_t ground_last_byte = 0;

void echo_command(char c) {
  com.printf("command = %c\r\n", c);
  cdh.printf("command = %c\r\n", c);
}

void ground_byte(char c) {
  ground_last_byte = millis();
  if (c == '\r' || c == '\n') {
    ground_line[ground_used] = 0;
    if (!ground_overflow && ground_used == 18 && !strncmp(ground_line, "H?", 2) &&
        strspn(ground_line + 2, "0123456789abcdefABCDEF") == 16) {
      ground_line[1] = '!';
      com.send_to(com.zigbee()->paired(), (const uint8_t*)ground_line, ground_used);
    } else if (!ground_overflow && ground_used == 19 && !strncmp(ground_line, "TO ", 3) &&
               strspn(ground_line + 3, "0123456789abcdefABCDEF") == 16) {
      uint64_t peer = strtoull(ground_line + 3, nullptr, 16);
      com.send_to(peer, (const uint8_t*)"HELLO", 5);
    } else if (ground_used == 1) {
      echo_command(ground_line[0]);
    }
    ground_used = 0;
    ground_overflow = false;
  } else if (ground_used == 0 && c != 'H' && c != 'T') {
    echo_command(c);
  } else if (ground_used < sizeof(ground_line) - 1) {
    ground_line[ground_used++] = c;
  } else {
    ground_overflow = true;
  }
}

void setup() {
  cdh.begin();
  eps.init();
  ready = com.begin(XBEE_MODE_API, XbeeReceive::Packets);
  if (!ready) cdh.println(com.last_error());
}

void loop() {
  lab.poll(Serial, com);
  if (!ready) { delay(1); return; }
  XbeePacket packet;
  if (com.tx_result().state != XbeeTxResult::Pending && com.receive_packet(packet)) {
    if (packet.length == 5 && memcmp(packet.data, "HELLO", 5) == 0) {
      com.send_to(packet.source, (const uint8_t*)"ACK", 3);
    } else if (packet.source == com.zigbee()->paired()) {
      for (size_t i = 0; i < packet.length; ++i) ground_byte((char)packet.data[i]);
    }
    if (packet.length == 3 && !memcmp(packet.data, "ACK", 3)) {
      com.printf("ACK from %016llX\n", (unsigned long long)packet.source);
    }
  }
  // A single H or T without an EOL is still a legacy echo command.
  if (ground_used == 1 && millis() - ground_last_byte > 500) {
    echo_command(ground_line[0]);
    ground_used = 0;
  }
  if (millis() - last_count >= 1000 &&
      com.zigbee()->paired_result().state != XbeeTxResult::Pending) {
    last_count = millis();
    com.printf("count from COM = %lu\n", (unsigned long)++count);
  }
}
