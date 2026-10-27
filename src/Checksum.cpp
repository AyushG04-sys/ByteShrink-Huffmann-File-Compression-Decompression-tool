#include "Checksum.h"

CRC32::CRC32() {
    initTable();
    reset();
}

void CRC32::initTable() {
    const uint32_t polynomial = 0xEDB88320;
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int j = 0; j < 8; ++j) {
            if (c & 1) {
                c = polynomial ^ (c >> 1);
            } else {
                c = c >> 1;
            }
        }
        table[i] = c;
    }
}

void CRC32::reset() {
    currentCrc = 0xFFFFFFFF;
}

void CRC32::update(const uint8_t* data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        uint8_t index = static_cast<uint8_t>((currentCrc ^ data[i]) & 0xFF);
        currentCrc = table[index] ^ (currentCrc >> 8);
    }
}

uint32_t CRC32::finalize() const {
    return currentCrc ^ 0xFFFFFFFF;
}

uint32_t CRC32::calculate(const uint8_t* data, size_t length) {
    CRC32 crc;
    crc.update(data, length);
    return crc.finalize();
}
