#pragma once

#include "src/common/hepta_cdh_base.h"
#include "src/common/hepta_image_tx.h"
#include "src/drv/zigbee.h"

namespace picture_downlink {
constexpr char filename[] = "/picture_dl.jpg";
constexpr size_t header_size = 12, block_size = 64;
constexpr uint32_t max_size = 1024 * 1024;

inline void put32(uint8_t* p, uint32_t n) {
  for (unsigned i = 0; i < 4; ++i) p[i] = n >> (8 * i);
}

inline uint32_t get32(const uint8_t* p) {
  uint32_t n = 0;
  for (unsigned i = 0; i < 4; ++i) n |= uint32_t(p[i]) << (8 * i);
  return n;
}

template <typename FileType>
bool checksum(FileType& file, uint32_t size, uint32_t& crc) {
  uint8_t buf[512];
  crc = 0xffffffffu;
  while (size) {
    const size_t count = min(size_t(size), sizeof(buf));
    if (file.read(buf, count) != int(count)) return false;
    for (size_t i = 0; i < count; ++i) {
      crc ^= buf[i];
      for (int bit = 0; bit < 8; ++bit)
        crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320u : 0);
    }
    size -= count;
  }
  crc ^= 0xffffffffu;
  return true;
}

// Reads are idempotent. Each response echoes the requesting session and byte
// offset. A second ground station can read the same file independently.
template <typename OpenPicture>
bool handle(const XbeePacket& packet, OpenPicture open_picture, HeptaComBase& com) {
  const uint8_t* request = packet.data;
  if (packet.length != header_size || memcmp(request, "\x89HI", 3) ||
      (request[3] != '?' && request[3] != 'R')) return false;
  if (packet.options & 2) return true;
  uint8_t reply[header_size + block_size + 2];
  memcpy(reply, request, header_size);
  uint8_t* body = reply + header_size;
  const uint32_t offset = get32(request + 8);
  auto file = open_picture();
  const size_t size = file ? file.size() : 0;
  reply[3] = 'E';
  body[0] = 1; // unavailable or empty
  size_t n = 1;
  if (size > max_size) body[0] = 2;
  else if (size && request[3] == '?' && offset == 0) {
    uint32_t crc;
    if (checksum(file, size, crc)) {
      reply[3] = 'I';
      put32(body, size);
      put32(body + 4, crc);
      n = 8;
    } else body[0] = 3;
  } else if (size && request[3] == 'R' && offset < size) {
    const size_t count = min(block_size, size - offset);
    if (file.seek(offset) && file.read(body, count) == int(count)) {
      reply[3] = 'D';
      n = count;
    } else body[0] = 3;
  } else if (size) body[0] = 3;
  file.close();
  const uint16_t crc = HeptaImageTx::crc16_ccitt_false(reply, header_size + n);
  body[n] = crc;
  body[n + 1] = crc >> 8;
  com.send_to(packet.source, reply, header_size + n + 2);
  return true;
}
} // namespace picture_downlink
