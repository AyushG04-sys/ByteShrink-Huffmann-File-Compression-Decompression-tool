#ifndef COMPRESSION_STATS_H
#define COMPRESSION_STATS_H

#include <cstdint>
#include <string>

/**
 * @brief CompressionStats computes and formats performance and sizing statistics.
 */
struct CompressionStats {
    uint64_t originalSize;        ///< Size of input file in bytes
    uint64_t compressedSize;      ///< Size of archive in bytes
    int64_t spaceSaved;           ///< originalSize - compressedSize (can be negative if enlarged)
    double compressionRatio;      ///< originalSize / compressedSize (0.0 if compressedSize is 0)
    double spaceSavedPercent;     ///< ((original - compressed) / original) * 100.0
    double durationSeconds;       ///< Processing time in seconds
    double throughputMBps;        ///< Processing speed in megabytes per second
    bool wasEnlarged;             ///< true if compressedSize > originalSize
    bool isZeroByteFile;          ///< true if originalSize == 0
    std::string statusMessage;    ///< User-friendly diagnostic message

    CompressionStats();

    /**
     * @brief Computes all metrics from raw sizes and elapsed duration.
     */
    void compute(uint64_t origSize, uint64_t compSize, double durationSec);

    /**
     * @brief Formats raw byte count into human-readable string (B, KB, MB, GB).
     */
    static std::string formatBytes(uint64_t bytes);

    /**
     * @brief Formats signed byte count (+/- B, KB, MB, GB).
     */
    static std::string formatSignedBytes(int64_t bytes);

    /**
     * @brief Formats duration in milliseconds or seconds.
     */
    static std::string formatDuration(double seconds);
};

#endif // COMPRESSION_STATS_H
