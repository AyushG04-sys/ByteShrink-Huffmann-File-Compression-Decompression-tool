#ifndef HUFFMAN_ENCODER_H
#define HUFFMAN_ENCODER_H

#include "HuffmanTree.h"
#include "CompressionStats.h"
#include <string>
#include <functional>
#include <atomic>
#include <vector>

/**
 * @brief Callback function type for reporting compression progress.
 * @param progress Value from 0.0f to 1.0f.
 * @param stageDescription Human-readable description of current phase.
 */
using ProgressCallback = std::function<void(float progress, const std::string& stageDescription)>;

/**
 * @brief Result struct returned upon compression completion.
 */
struct CompressionResult {
    bool success;
    std::string errorMessage;
    std::string outputPath;
    CompressionStats stats;
    std::vector<uint64_t> frequencies; // Kept for Algorithm Explorer

    CompressionResult() : success(false), frequencies(256, 0) {}
};

/**
 * @brief Core Huffman compression engine.
 */
class HuffmanEncoder {
private:
    std::vector<uint64_t> lastFrequencies;
    HuffmanTree lastTree;

public:
    HuffmanEncoder();
    ~HuffmanEncoder() = default;

    /**
     * @brief Compresses an input file to a .hfx archive.
     * @param inputFilePath Absolute or relative path to source file.
     * @param outputFilePath Desired destination .hfx archive path.
     * @param onProgress Optional progress callback.
     * @param cancelFlag Optional atomic flag to request cancellation.
     * @return CompressionResult Complete statistics and status.
     */
    CompressionResult compressFile(const std::string& inputFilePath,
                                  const std::string& outputFilePath,
                                  ProgressCallback onProgress = nullptr,
                                  std::atomic<bool>* cancelFlag = nullptr);

    /**
     * @brief Access to last generated Huffman Tree for Algorithm Explorer inspection.
     */
    const HuffmanTree& getLastTree() const { return lastTree; }

    /**
     * @brief Access to last calculated byte frequencies.
     */
    const std::vector<uint64_t>& getLastFrequencies() const { return lastFrequencies; }
};

#endif // HUFFMAN_ENCODER_H
