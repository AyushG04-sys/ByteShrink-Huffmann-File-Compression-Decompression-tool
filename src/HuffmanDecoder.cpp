#include "HuffmanDecoder.h"
#include "BitReader.h"
#include "Checksum.h"
#include "FileUtils.h"
#include <fstream>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <iomanip>

using namespace std;
namespace fs = filesystem;

ArchiveMetadata HuffmanDecoder::inspectArchive(const string& archivePath) {
    ArchiveMetadata meta;
    meta.isValid = false;

    if (!FileUtils::fileExists(archivePath)) {
        meta.errorMessage = "Archive file does not exist: " + archivePath;
        return meta;
    }

    uint64_t totalSize = FileUtils::getFileSize(archivePath);
    ifstream inFile(archivePath, ios::binary);
    if (!inFile) {
        meta.errorMessage = "Could not open archive file: " + archivePath;
        return meta;
    }

    return ArchiveFormat::readHeader(inFile, static_cast<size_t>(totalSize));
}

DecompressionResult HuffmanDecoder::decompressFile(const string& archivePath,
                                                  const string& outputFilePath,
                                                  const string& outputDirectory,
                                                  DecompressProgressCallback onProgress,
                                                  atomic<bool>* cancelFlag) {
    DecompressionResult result;
    result.success = false;

    auto startTime = chrono::high_resolution_clock::now();

    // 1. Validate and read header
    if (onProgress) onProgress(0.05f, "Validating archive format and reading metadata...");

    if (!FileUtils::fileExists(archivePath)) {
        result.errorMessage = "Archive file does not exist: " + archivePath;
        return result;
    }

    uint64_t totalArchiveSize = FileUtils::getFileSize(archivePath);
    ifstream inFile(archivePath, ios::binary);
    if (!inFile) {
        result.errorMessage = "Could not open archive file for reading: " + archivePath;
        return result;
    }

    ArchiveMetadata meta = ArchiveFormat::readHeader(inFile, static_cast<size_t>(totalArchiveSize));
    if (!meta.isValid) {
        result.errorMessage = meta.errorMessage;
        return result;
    }

    result.originalFilename = meta.originalFilename;
    result.expectedCrc = meta.header.originalCrc32;

    // 2. Determine final destination path
    string destinationPath = outputFilePath;
    if (destinationPath.empty()) {
        string targetDir = outputDirectory.empty() ? FileUtils::getDirectoryPath(archivePath) : outputDirectory;
        destinationPath = (fs::path(targetDir) / meta.originalFilename).string();
    }

    string destDir = FileUtils::getDirectoryPath(destinationPath);
    if (!destDir.empty() && !FileUtils::directoryExists(destDir)) {
        try {
            fs::create_directories(destDir);
        } catch (...) {
            result.errorMessage = "Failed to create destination directory: " + destDir;
            return result;
        }
    }

    string tempOutputPath = FileUtils::createTempFilePath(destinationPath);
    ofstream outFile(tempOutputPath, ios::binary);
    if (!outFile) {
        result.errorMessage = "Could not create temporary output file: " + tempOutputPath;
        return result;
    }

    // 3. Handle Special Case: Zero-byte file
    if (meta.header.originalSize == 0) {
        outFile.close();
        if (!FileUtils::atomicReplaceFile(tempOutputPath, destinationPath)) {
            result.errorMessage = "Failed to finalize zero-byte output file.";
            return result;
        }
        result.restoredSize = 0;
        result.outputPath = destinationPath;
        result.actualCrc = 0;
        result.success = true;
        if (onProgress) onProgress(1.0f, "Decompression complete (empty file restored).");
        return result;
    }

    // 4. Reconstruct Huffman Tree from frequency table
    if (onProgress) onProgress(0.12f, "Reconstructing Huffman tree from metadata...");

    HuffmanTree tree;
    if (!tree.buildFromFrequencies(meta.frequencyTable)) {
        outFile.close();
        fs::remove(tempOutputPath);
        result.errorMessage = "Failed to reconstruct Huffman tree from stored metadata.";
        return result;
    }

    // 5. Decode bitstream
    if (onProgress) onProgress(0.18f, "Decoding compressed bitstream and restoring original bytes...");

    CRC32 crc;
    uint64_t bytesDecoded = 0;
    const size_t outBufferSize = 65536;
    vector<uint8_t> outBuffer;
    outBuffer.reserve(outBufferSize);

    // Special Case: Single unique symbol in original file
    if (tree.getDistinctSymbolCount() == 1) {
        uint8_t singleByte = tree.getRoot()->byteValue;
        while (bytesDecoded < meta.header.originalSize) {
            if (cancelFlag && cancelFlag->load()) {
                outFile.close();
                fs::remove(tempOutputPath);
                result.errorMessage = "Decompression cancelled by user.";
                return result;
            }

            size_t toWrite = min(static_cast<uint64_t>(outBufferSize),
                                 meta.header.originalSize - bytesDecoded);
            outBuffer.assign(toWrite, singleByte);
            outFile.write(reinterpret_cast<const char*>(outBuffer.data()), toWrite);
            crc.update(outBuffer.data(), toWrite);
            bytesDecoded += toWrite;

            if (onProgress) {
                float prog = 0.18f + 0.75f * (static_cast<float>(bytesDecoded) / static_cast<float>(meta.header.originalSize));
                onProgress(prog, "Restoring symbols (" +
                           CompressionStats::formatBytes(bytesDecoded) + " / " +
                           CompressionStats::formatBytes(meta.header.originalSize) + ")...");
            }
        }
    } else {
        // General Case: Traverse tree from root for each bit
        BitReader bitReader(inFile, 65536);
        HuffmanNode* root = tree.getRoot();
        HuffmanNode* curr = root;

        while (bytesDecoded < meta.header.originalSize) {
            if (cancelFlag && cancelFlag->load()) {
                outFile.close();
                fs::remove(tempOutputPath);
                result.errorMessage = "Decompression cancelled by user.";
                return result;
            }

            int bit = 0;
            if (!bitReader.readBit(bit)) {
                outFile.close();
                fs::remove(tempOutputPath);
                result.errorMessage = "Corrupted archive: bitstream ended prematurely (" +
                                    to_string(bytesDecoded) + " of " +
                                    to_string(meta.header.originalSize) + " bytes decoded).";
                return result;
            }

            curr = (bit == 0) ? curr->left : curr->right;
            if (!curr) {
                outFile.close();
                fs::remove(tempOutputPath);
                result.errorMessage = "Corrupted bitstream: encountered null child node in Huffman tree.";
                return result;
            }

            if (curr->isLeaf()) {
                outBuffer.push_back(curr->byteValue);
                bytesDecoded++;
                curr = root;

                if (outBuffer.size() >= outBufferSize) {
                    outFile.write(reinterpret_cast<const char*>(outBuffer.data()), outBuffer.size());
                    crc.update(outBuffer.data(), outBuffer.size());
                    outBuffer.clear();

                    if (onProgress) {
                        float prog = 0.18f + 0.75f * (static_cast<float>(bytesDecoded) / static_cast<float>(meta.header.originalSize));
                        onProgress(prog, "Decoding bitstream (" +
                                   CompressionStats::formatBytes(bytesDecoded) + " / " +
                                   CompressionStats::formatBytes(meta.header.originalSize) + ")...");
                    }
                }
            }
        }

        // Flush remaining decoded bytes in buffer
        if (!outBuffer.empty()) {
            outFile.write(reinterpret_cast<const char*>(outBuffer.data()), outBuffer.size());
            crc.update(outBuffer.data(), outBuffer.size());
            outBuffer.clear();
        }
    }

    outFile.close();
    inFile.close();

    // 6. Verification
    if (onProgress) onProgress(0.95f, "Verifying integrity checksum...");

    result.actualCrc = crc.finalize();

    // Verify byte count
    if (bytesDecoded != meta.header.originalSize) {
        fs::remove(tempOutputPath);
        result.errorMessage = "Size mismatch: expected " + to_string(meta.header.originalSize) +
                              " bytes, but decoded " + to_string(bytesDecoded) + " bytes.";
        return result;
    }

    // Verify CRC32 checksum
    if ((meta.header.flags & ARCHIVE_FLAG_HAS_CRC32) && (result.actualCrc != meta.header.originalCrc32)) {
        fs::remove(tempOutputPath);
        ostringstream oss;
        oss << "Integrity check failed: Checksum mismatch! (Expected 0x"
            << hex << uppercase << meta.header.originalCrc32
            << ", Got 0x" << result.actualCrc << "). The archive is corrupted or has been altered.";
        result.errorMessage = oss.str();
        return result;
    }

    // 7. Atomic replace
    if (!FileUtils::atomicReplaceFile(tempOutputPath, destinationPath)) {
        result.errorMessage = "Failed to finalize destination file: " + destinationPath;
        return result;
    }

    auto endTime = chrono::high_resolution_clock::now();
    result.durationSeconds = chrono::duration<double>(endTime - startTime).count();
    if (result.durationSeconds > 0.0001) {
        result.throughputMBps = (static_cast<double>(bytesDecoded) / (1024.0 * 1024.0)) / result.durationSeconds;
    }

    result.restoredSize = bytesDecoded;
    result.outputPath = destinationPath;
    result.success = true;

    if (onProgress) onProgress(1.0f, "Decompression successful! Original file restored byte-for-byte.");
    return result;
}
