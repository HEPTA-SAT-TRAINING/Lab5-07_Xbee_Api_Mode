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
#include "picture_downlink.h"

HeptaLabUsb lab;
bool ready = false;
uint32_t last_count = 0;
uint32_t count = 0;
constexpr uint32_t counter_period_ms = 1000;
constexpr uint32_t counter_max_period_ms = 16000;
uint32_t counter_interval_ms = counter_period_ms;
uint8_t counter_frame = 0;
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
      if (com.send_to(peer, (const uint8_t*)"HELLO", 5))
        cdh.printf("HELLO to %016llX\n", (unsigned long long)peer);
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

void update_counter() {
  auto* radio = com.zigbee();
  const auto result = radio->paired_result();
  if (result.state == XbeeTxResult::Pending) return;
  if (counter_frame) {
    // Command replies can also use the paired transmit slot.
    if (result.frame_id == counter_frame) {
      counter_interval_ms = result.state == XbeeTxResult::Delivered
          ? counter_period_ms : min(counter_interval_ms * 2, counter_max_period_ms);
    }
    counter_frame = 0;
  }
  const uint32_t now = millis();
  if (now - last_count < counter_interval_ms) return;
  last_count = now;
  if (com.printf("count from COM = %lu\n", (unsigned long)++count))
    counter_frame = radio->paired_result().frame_id;
}

void loop() {
  lab.poll(Serial, com);
  if (!ready) { delay(1); return; }
  XbeePacket packet;
  if (com.tx_result().state != XbeeTxResult::Pending && com.receive_packet(packet)) {
    if (picture_downlink::handle(packet, [] { return cdh.open_file(picture_downlink::filename); }, com)) {
      update_counter();
      return;
    }
    if (packet.length == 5 && memcmp(packet.data, "HELLO", 5) == 0) {
      com.send_to(packet.source, (const uint8_t*)"ACK", 3);
      cdh.printf("HELLO from %016llX\n", (unsigned long long)packet.source);
    } else if (packet.source == com.zigbee()->paired()) {
      for (size_t i = 0; i < packet.length; ++i) ground_byte((char)packet.data[i]);
    } else {
      // An API ground station sends each command as a complete RF packet.
      size_t length = packet.length;
      while (length && (packet.data[length - 1] == '\r' || packet.data[length - 1] == '\n')) --length;
      if (length == 1 && packet.data[0] >= ' ' && packet.data[0] <= '~') {
        char reply[] = "command = ?\r\n";
        reply[10] = (char)packet.data[0];
        com.send_to(packet.source, (const uint8_t*)reply, sizeof(reply) - 1);
        cdh.printf("command = %c\r\n", packet.data[0]);
      }
    }
    if (packet.length == 3 && !memcmp(packet.data, "ACK", 3)) {
      cdh.printf("ACK from %016llX\n", (unsigned long long)packet.source);
      com.printf("ACK from %016llX\n", (unsigned long long)packet.source);
    }
  }
  // A single H or T without an EOL is still a legacy echo command.
  if (ground_used == 1 && millis() - ground_last_byte > 500) {
    echo_command(ground_line[0]);
    ground_used = 0;
  }
  update_counter();
}
