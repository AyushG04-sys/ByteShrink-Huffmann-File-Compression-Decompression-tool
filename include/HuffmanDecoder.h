#ifndef HUFFMAN_DECODER_H
#define HUFFMAN_DECODER_H

#include "HuffmanTree.h"
#include "ArchiveFormat.h"
#include "CompressionStats.h"
#include <string>
#include <functional>
#include <atomic>

using DecompressProgressCallback = std::function<void(float progress, const std::string& stageDescription)>;

struct DecompressionResult {
    bool success;
    std::string errorMessage;
    std::string outputPath;
    std::string originalFilename;
    uint64_t restoredSize;
    uint32_t expectedCrc;
    uint32_t actualCrc;
    double durationSeconds;
    double throughputMBps;

    DecompressionResult()
        : success(false), restoredSize(0), expectedCrc(0), actualCrc(0),
          durationSeconds(0.0), throughputMBps(0.0) {}
};

class HuffmanDecoder {
public:
    HuffmanDecoder() = default;
    ~HuffmanDecoder() = default;

    /**
     * @brief Inspects archive header without decompressing.
     * Useful for UI pre-validation before user clicks Start Decompression.
     */
    ArchiveMetadata inspectArchive(const std::string& archivePath);

    /**
     * @brief Decompresses a .hfx archive into the exact original file.
     * @param archivePath Path to .hfx archive.
     * @param outputFilePath Destination file path (or empty to use stored filename in outputDirectory).
     * @param outputDirectory Directory where restored file should be placed.
     * @param onProgress Optional progress callback.
     * @param cancelFlag Optional cancellation flag.
     * @return DecompressionResult Complete status and verification details.
     */
    DecompressionResult decompressFile(const std::string& archivePath,
                                      const std::string& outputFilePath,
                                      const std::string& outputDirectory,
                                      DecompressProgressCallback onProgress = nullptr,
                                      std::atomic<bool>* cancelFlag = nullptr);
};

#endif // HUFFMAN_DECODER_H
