#include "BitWriter.h"
#include <stdexcept>

using namespace std;

BitWriter::BitWriter(ostream& stream, size_t bufferSize)
    : outStream(stream),
      currentByte(0),
      bitCount(0),
      totalBitsWritten(0),
      bufferCapacity(bufferSize) {
    buffer.reserve(bufferCapacity);
}

BitWriter::~BitWriter() {
    // Note: flush() should be called explicitly by caller to capture validBits
    try {
        flush();
    } catch (...) {}
}

void BitWriter::flushBuffer() {
    if (!buffer.empty()) {
        outStream.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        buffer.clear();
    }
}

void BitWriter::writeBit(int bit) {
    // MSB first: shift left and place new bit at LSB
    currentByte = static_cast<uint8_t>((currentByte << 1) | (bit & 1));
    bitCount++;
    totalBitsWritten++;

    if (bitCount == 8) {
        buffer.push_back(currentByte);
        currentByte = 0;
        bitCount = 0;

        if (buffer.size() >= bufferCapacity) {
            flushBuffer();
        }
    }
}

void BitWriter::writeBits(const string& bitString) {
    for (char c : bitString) {
        if (c == '0') {
            writeBit(0);
        } else if (c == '1') {
            writeBit(1);
        }
    }
}

void BitWriter::writeByte(uint8_t byte) {
    if (bitCount != 0) {
        throw runtime_error("Attempted to write raw byte while BitWriter is not byte-aligned");
    }
    buffer.push_back(byte);
    totalBitsWritten += 8;
    if (buffer.size() >= bufferCapacity) {
        flushBuffer();
    }
}

uint8_t BitWriter::flush() {
    uint8_t validBitsInLast = 0;

    if (totalBitsWritten == 0) {
        flushBuffer();
        outStream.flush();
        return 0;
    }

    if (bitCount > 0) {
        validBitsInLast = static_cast<uint8_t>(bitCount);
        // Shift remaining bits to the most significant positions
        currentByte = static_cast<uint8_t>(currentByte << (8 - bitCount));
        buffer.push_back(currentByte);
        currentByte = 0;
        bitCount = 0;
    } else {
        validBitsInLast = 8;
    }

    flushBuffer();
    outStream.flush();
    return validBitsInLast;
}
