#include "HuffmanEncoder.h"
#include "BitWriter.h"
#include "Checksum.h"
#include "ArchiveFormat.h"
#include "FileUtils.h"
#include <fstream>
#include <chrono>
#include <filesystem>
#include <iostream>

using namespace std;
namespace fs = filesystem;

HuffmanEncoder::HuffmanEncoder()
    : lastFrequencies(256, 0) {}

CompressionResult HuffmanEncoder::compressFile(const string& inputFilePath,
                                              const string& outputFilePath,
                                              ProgressCallback onProgress,
                                              atomic<bool>* cancelFlag) {
    CompressionResult result;
    result.success = false;

    auto startTime = chrono::high_resolution_clock::now();

    // 1. Validate input file
    if (!FileUtils::fileExists(inputFilePath)) {
        result.errorMessage = "Input file does not exist: " + inputFilePath;
        return result;
    }

    uint64_t fileSize = FileUtils::getFileSize(inputFilePath);
    string baseFilename = FileUtils::getBaseFilename(inputFilePath);

    // Ensure output directory exists
    string outDir = FileUtils::getDirectoryPath(outputFilePath);
    if (!outDir.empty() && !FileUtils::directoryExists(outDir)) {
        try {
            fs::create_directories(outDir);
        } catch (...) {
            result.errorMessage = "Failed to create output directory: " + outDir;
            return result;
        }
    }

    string tempOutputPath = FileUtils::createTempFilePath(outputFilePath);

    // Pass 1: Frequency Counting & Checksum Calculation
    if (onProgress) onProgress(0.05f, "Analyzing byte frequencies & calculating checksum...");

    ifstream inFile(inputFilePath, ios::binary);
    if (!inFile) {
        result.errorMessage = "Could not open input file for reading: " + inputFilePath;
        return result;
    }

    vector<uint64_t> freqs(256, 0);
    CRC32 crc;
    const size_t bufferCapacity = 65536;
    vector<uint8_t> buffer(bufferCapacity);
    uint64_t bytesProcessed = 0;

    while (inFile) {
        if (cancelFlag && cancelFlag->load()) {
            inFile.close();
            result.errorMessage = "Compression cancelled by user.";
            return result;
        }

        inFile.read(reinterpret_cast<char*>(buffer.data()), bufferCapacity);
        streamsize count = inFile.gcount();
        if (count > 0) {
            crc.update(buffer.data(), static_cast<size_t>(count));
            for (streamsize i = 0; i < count; ++i) {
                freqs[buffer[i]]++;
            }
            bytesProcessed += static_cast<uint64_t>(count);

            if (onProgress && fileSize > 0) {
                float progress = 0.05f + 0.40f * (static_cast<float>(bytesProcessed) / static_cast<float>(fileSize));
                onProgress(progress, "Analyzing frequencies (" +
                           CompressionStats::formatBytes(bytesProcessed) + " / " +
                           CompressionStats::formatBytes(fileSize) + ")...");
            }
        }
    }

    uint32_t finalCrc = crc.finalize();
    lastFrequencies = freqs;
    result.frequencies = freqs;

    // 2. Build Huffman Tree using Greedy MinHeap
    if (onProgress) onProgress(0.48f, "Building Huffman tree with greedy min-heap...");

    lastTree.clear();
    if (!lastTree.buildFromFrequencies(freqs)) {
        result.errorMessage = "Failed to construct Huffman tree.";
        return result;
    }

    const string* codeTable = lastTree.getCodeTable();

    // 3. Open temporary output archive
    ofstream outFile(tempOutputPath, ios::binary);
    if (!outFile) {
        result.errorMessage = "Could not create temporary archive file: " + tempOutputPath;
        return result;
    }

    // Write initial header with validBitsInLastByte = 0 (will be updated after encoding)
    if (!ArchiveFormat::writeHeader(outFile, fileSize, finalCrc, 0, baseFilename, freqs)) {
        outFile.close();
        fs::remove(tempOutputPath);
        result.errorMessage = "Failed to write archive header and frequency table.";
        return result;
    }

    // 4. Pass 2: Stream input file and pack variable-length Huffman codes
    if (onProgress) onProgress(0.52f, "Encoding bitstream with prefix-free Huffman codes...");

    // Rewind input file to beginning
    inFile.clear();
    inFile.seekg(0, ios::beg);

    BitWriter bitWriter(outFile, bufferCapacity);
    bytesProcessed = 0;

    // Special case: Single symbol file
    bool isSingleSymbol = (lastTree.getDistinctSymbolCount() == 1);

    while (inFile && fileSize > 0) {
        if (cancelFlag && cancelFlag->load()) {
            inFile.close();
            outFile.close();
            fs::remove(tempOutputPath);
            result.errorMessage = "Compression cancelled by user.";
            return result;
        }

        inFile.read(reinterpret_cast<char*>(buffer.data()), bufferCapacity);
        streamsize count = inFile.gcount();
        if (count > 0) {
            for (streamsize i = 0; i < count; ++i) {
                uint8_t byteVal = buffer[i];
                if (isSingleSymbol) {
                    bitWriter.writeBit(0);
                } else {
                    const string& code = codeTable[byteVal];
                    bitWriter.writeBits(code);
                }
            }
            bytesProcessed += static_cast<uint64_t>(count);

            if (onProgress && fileSize > 0) {
                float progress = 0.52f + 0.44f * (static_cast<float>(bytesProcessed) / static_cast<float>(fileSize));
                onProgress(progress, "Encoding bitstream (" +
                           CompressionStats::formatBytes(bytesProcessed) + " / " +
                           CompressionStats::formatBytes(fileSize) + ")...");
            }
        }
    }

    inFile.close();

    // Flush any remaining partial bits and capture valid count
    uint8_t validBitsInLastByte = bitWriter.flush();
    outFile.close();

    // 5. Update the header with the exact valid bit count of the final byte
    if (!ArchiveFormat::updateLastByteValidBits(tempOutputPath, validBitsInLastByte)) {
        fs::remove(tempOutputPath);
        result.errorMessage = "Failed to finalize archive header with bitstream boundaries.";
        return result;
    }

    // 6. Atomically replace target output file
    if (onProgress) onProgress(0.98f, "Finalizing archive on disk...");

    if (!FileUtils::atomicReplaceFile(tempOutputPath, outputFilePath)) {
        result.errorMessage = "Failed to finalize output file: " + outputFilePath;
        return result;
    }

    auto endTime = chrono::high_resolution_clock::now();
    double durationSec = chrono::duration<double>(endTime - startTime).count();

    uint64_t compressedSize = FileUtils::getFileSize(outputFilePath);

    result.stats.compute(fileSize, compressedSize, durationSec);
    result.outputPath = outputFilePath;
    result.success = true;

    if (onProgress) onProgress(1.0f, "Compression complete!");
    return result;
}
