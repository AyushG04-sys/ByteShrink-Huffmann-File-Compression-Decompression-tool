#include "HuffmanEncoder.h"
#include "HuffmanDecoder.h"
#include "FileUtils.h"
#include <iostream>
#include <string>
#include <iomanip>

using namespace std;

void printHelp() {
    cout << "========================================================\n"
         << "  ByteShrink — Smart File Compression Studio\n"
         << "  Privacy-First Lossless File Compression (C++ DSA)\n"
         << "========================================================\n\n"
         << "Usage:\n"
         << "  ByteShrink -c <input> <output> Compress file to .hfx archive\n"
         << "  ByteShrink -d <archive> <out>  Decompress .hfx archive to original file\n"
         << "  ByteShrink -i <archive>        Inspect archive metadata and header\n"
         << "  ByteShrink --test              Run automated verification test suite\n"
         << "  ByteShrink --help              Show this help message\n\n"
         << "Examples:\n"
         << "  ByteShrink -c document.pdf document.pdf.hfx\n"
         << "  ByteShrink -d document.pdf.hfx restored_doc.pdf\n"
         << "  ByteShrink -i archive.hfx\n";
}

int runCliCompress(const string& inPath, const string& outPath) {
    cout << "[ByteShrink] Compressing: " << inPath << " -> " << outPath << "\n";
    HuffmanEncoder encoder;
    CompressionResult res = encoder.compressFile(
        inPath, outPath,
        [](float prog, const string& stage) {
            cout << "\r[" << setw(3) << static_cast<int>(prog * 100) << "%] "
                 << left << setw(50) << stage << flush;
        }
    );
    cout << "\n";

    if (res.success) {
        cout << "\n[SUCCESS] Compression Complete!\n"
             << "  Original Size:    " << CompressionStats::formatBytes(res.stats.originalSize) << " (" << res.stats.originalSize << " bytes)\n"
             << "  Compressed Size:  " << CompressionStats::formatBytes(res.stats.compressedSize) << " (" << res.stats.compressedSize << " bytes)\n"
             << "  Space Saved:      " << CompressionStats::formatSignedBytes(res.stats.spaceSaved) << " (" << fixed << setprecision(1) << res.stats.spaceSavedPercent << "%)\n"
             << "  Compression Ratio: " << fixed << setprecision(2) << res.stats.compressionRatio << " : 1\n"
             << "  Duration:         " << CompressionStats::formatDuration(res.stats.durationSeconds) << "\n"
             << "  Throughput:       " << fixed << setprecision(2) << res.stats.throughputMBps << " MB/s\n"
             << "  Diagnostic:       " << res.stats.statusMessage << "\n";
        return 0;
    } else {
        cerr << "\n[ERROR] Compression failed: " << res.errorMessage << "\n";
        return 1;
    }
}

int runCliDecompress(const string& archPath, const string& outPath) {
    cout << "[ByteShrink] Decompressing: " << archPath << " -> " << outPath << "\n";
    HuffmanDecoder decoder;
    DecompressionResult res = decoder.decompressFile(
        archPath, outPath, "",
        [](float prog, const string& stage) {
            cout << "\r[" << setw(3) << static_cast<int>(prog * 100) << "%] "
                 << left << setw(50) << stage << flush;
        }
    );
    cout << "\n";

    if (res.success) {
        cout << "\n[SUCCESS] Decompression Complete!\n"
             << "  Restored File: " << res.outputPath << "\n"
             << "  Restored Size: " << CompressionStats::formatBytes(res.restoredSize) << " (" << res.restoredSize << " bytes)\n"
             << "  Integrity CRC: 0x" << hex << uppercase << res.actualCrc << " (VERIFIED MATCH)\n"
             << "  Duration:      " << dec << CompressionStats::formatDuration(res.durationSeconds) << "\n"
             << "  Throughput:    " << fixed << setprecision(2) << res.throughputMBps << " MB/s\n";
        return 0;
    } else {
        cerr << "\n[ERROR] Decompression failed: " << res.errorMessage << "\n";
        return 1;
    }
}

int runCliInfo(const string& archPath) {
    HuffmanDecoder decoder;
    ArchiveMetadata meta = decoder.inspectArchive(archPath);
    if (!meta.isValid) {
        cerr << "[ERROR] Invalid archive: " << meta.errorMessage << "\n";
        return 1;
    }

    cout << "========================================================\n"
         << "  ByteShrink Archive Header & Metadata: " << archPath << "\n"
         << "========================================================\n"
         << "  Magic Signature:    " << meta.header.magic[0] << meta.header.magic[1]
                                     << meta.header.magic[2] << meta.header.magic[3] << " (Valid)\n"
         << "  Format Version:     " << static_cast<int>(meta.header.version) << "\n"
         << "  Original Filename:  " << meta.originalFilename << "\n"
         << "  Original Size:      " << CompressionStats::formatBytes(meta.header.originalSize)
                                     << " (" << meta.header.originalSize << " bytes)\n"
         << "  Expected CRC-32:    0x" << hex << uppercase << meta.header.originalCrc32 << dec << "\n"
         << "  Distinct Symbols:   " << meta.header.symbolCount << " / 256\n"
         << "  Last Byte Bits:     " << static_cast<int>(meta.header.validBitsInLastByte) << " / 8 valid bits\n";
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc >= 2) {
        string arg1 = argv[1];
        if (arg1 == "--help" || arg1 == "-h") {
            printHelp();
            return 0;
        }
        if ((arg1 == "-c" || arg1 == "--compress") && argc >= 4) {
            return runCliCompress(argv[2], argv[3]);
        }
        if ((arg1 == "-d" || arg1 == "--decompress") && argc >= 4) {
            return runCliDecompress(argv[2], argv[3]);
        }
        if ((arg1 == "-i" || arg1 == "--info") && argc >= 3) {
            return runCliInfo(argv[2]);
        }
        if (arg1 == "--test") {
            cout << "To execute the full test suite, please run run_tests.exe\n";
            return 0;
        }
    }

    // Default: Display help and CLI syntax
    printHelp();
    return 0;
}
