#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <cstdint>
#include <cstddef>
#include <vector>

/**
 * @brief CRC32 Checksum calculator (Polynomial 0xEDB88320).
 *
 * Used to verify the exact byte-for-byte integrity of files during
 * compression and decompression, ensuring zero data corruption.
 */
class CRC32 {
private:
    uint32_t table[256];
    uint32_t currentCrc;

    void initTable();

public:
    CRC32();

    /**
     * @brief Resets current CRC accumulator to initial state (0xFFFFFFFF).
     */
    void reset();

    /**
     * @brief Updates CRC with a chunk of data.
     * @param data Pointer to input byte buffer.
     * @param length Number of bytes.
     */
    void update(const uint8_t* data, size_t length);

    /**
     * @brief Finalizes and returns the 32-bit CRC.
     * @return uint32_t Calculated CRC32 value.
     */
    uint32_t finalize() const;

    /**
     * @brief One-shot calculation of CRC32 for an entire buffer.
     */
    static uint32_t calculate(const uint8_t* data, size_t length);
};

#endif // CHECKSUM_H
