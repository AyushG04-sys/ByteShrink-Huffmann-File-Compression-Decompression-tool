#ifndef BIT_WRITER_H
#define BIT_WRITER_H

#include <ostream>
#include <vector>
#include <string>
#include <cstdint>

/**
 * @brief BitWriter handles packing arbitrary bit sequences into binary bytes.
 *
 * It uses an internal memory buffer (default 64KB) to minimize disk I/O syscalls,
 * buffering bits into bytes (MSB first) and streaming to an std::ostream.
 */
class BitWriter {
private:
    std::ostream& outStream;
    uint8_t currentByte;
    int bitCount;                 ///< Bits accumulated in currentByte (0..7)
    uint64_t totalBitsWritten;
    std::vector<uint8_t> buffer;
    size_t bufferCapacity;

    void flushBuffer();

public:
    /**
     * @brief Constructs BitWriter associated with an output stream.
     * @param stream The destination binary output stream.
     * @param bufferSize Internal byte buffer capacity (default 64 KB).
     */
    explicit BitWriter(std::ostream& stream, size_t bufferSize = 65536);
    ~BitWriter();

    // Disable copy
    BitWriter(const BitWriter&) = delete;
    BitWriter& operator=(const BitWriter&) = delete;

    /**
     * @brief Writes a single bit (0 or 1).
     * @param bit Bit value (0 or 1).
     */
    void writeBit(int bit);

    /**
     * @brief Writes a sequence of bits from a '0'/'1' string.
     * @param bitString String containing only '0' and '1' characters.
     */
    void writeBits(const std::string& bitString);

    /**
     * @brief Writes an uncompressed raw byte directly.
     * Throws if BitWriter is not currently byte-aligned.
     */
    void writeByte(uint8_t byte);

    /**
     * @brief Flushes any remaining partial bits in currentByte and flushes buffer.
     * @return uint8_t Number of valid bits in the final byte (0..8).
     */
    uint8_t flush();

    /**
     * @brief Returns total number of individual bits written so far.
     */
    uint64_t getTotalBitsWritten() const { return totalBitsWritten; }

    /**
     * @brief Checks if writer is currently aligned on a byte boundary.
     */
    bool isByteAligned() const { return bitCount == 0; }
};

#endif // BIT_WRITER_H
