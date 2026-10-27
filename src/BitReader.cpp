#include "BitReader.h"

using namespace std;

BitReader::BitReader(istream& stream, size_t bufferCapacity)
    : inStream(stream),
      currentByte(0),
      bitIndex(8), // Force fetching a byte on first read
      totalBitsRead(0),
      maxBitsToRead(0),
      hasMaxBits(false),
      bufferPos(0),
      bufferSize(0) {
    buffer.resize(bufferCapacity);
}

void BitReader::setTotalBitsToRead(uint64_t totalBits) {
    maxBitsToRead = totalBits;
    hasMaxBits = true;
}

bool BitReader::refillBuffer() {
    inStream.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    bufferSize = static_cast<size_t>(inStream.gcount());
    bufferPos = 0;
    return bufferSize > 0;
}

bool BitReader::fetchNextByte() {
    if (bufferPos >= bufferSize) {
        if (!refillBuffer()) {
            return false;
        }
    }
    currentByte = buffer[bufferPos++];
    bitIndex = 0;
    return true;
}

bool BitReader::readBit(int& outBit) {
    if (hasMaxBits && totalBitsRead >= maxBitsToRead) {
        return false;
    }

    if (bitIndex >= 8) {
        if (!fetchNextByte()) {
            return false;
        }
    }

    // Extract bit at bitIndex (MSB first: bit 0 is 0x80, bit 7 is 0x01)
    outBit = (currentByte >> (7 - bitIndex)) & 1;
    bitIndex++;
    totalBitsRead++;
    return true;
}

bool BitReader::isEof() const {
    if (hasMaxBits && totalBitsRead >= maxBitsToRead) {
        return true;
    }
    return inStream.eof() && bufferPos >= bufferSize && bitIndex >= 8;
}
