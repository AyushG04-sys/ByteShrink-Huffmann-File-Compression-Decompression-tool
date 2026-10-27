#ifndef BIT_READER_H
#define BIT_READER_H

#include <istream>
#include <vector>
#include <cstdint>
#include <cstddef>

/**
 * @brief BitReader handles unpacking binary bytes into individual bits.
 *
 * Employs buffered I/O streaming (default 64KB) for maximum throughput,
 * extracting bits MSB-first and respecting the exact bitstream length.
 */
class BitReader {
private:
    std::istream& inStream;
    uint8_t currentByte;
    int bitIndex;                 ///< Current bit position within currentByte (0..7, where 0 is MSB)
    uint64_t totalBitsRead;
    uint64_t maxBitsToRead;       ///< Total valid bits in the bitstream
    bool hasMaxBits;
    std::vector<uint8_t> buffer;
    size_t bufferPos;
    size_t bufferSize;

    bool refillBuffer();
    bool fetchNextByte();

public:
    /**
     * @brief Constructs BitReader associated with an input stream.
     * @param stream The source binary input stream.
     * @param bufferCapacity Internal buffer size (default 64 KB).
     */
    explicit BitReader(std::istream& stream, size_t bufferCapacity = 65536);
    ~BitReader() = default;

    // Disable copy
    BitReader(const BitReader&) = delete;
    BitReader& operator=(const BitReader&) = delete;

    /**
     * @brief Sets total expected valid bits to read from this bitstream.
     * Prevents reading into the zero-padding bits of the final byte.
     * @param totalBits The exact number of bits in the payload.
     */
    void setTotalBitsToRead(uint64_t totalBits);

    /**
     * @brief Reads a single bit from the stream.
     * @param[out] outBit Assigned 0 or 1.
     * @return true if a bit was successfully read; false if EOF or max bits reached.
     */
    bool readBit(int& outBit);

    /**
     * @brief Total number of individual bits read so far.
     */
    uint64_t getTotalBitsRead() const { return totalBitsRead; }

    /**
     * @brief Checks if reader has reached the end of valid bits or EOF.
     */
    bool isEof() const;
};

#endif // BIT_READER_H
