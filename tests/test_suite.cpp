#include "HuffmanEncoder.h"
#include "HuffmanDecoder.h"
#include "FileUtils.h"
#include "Checksum.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <random>
#include <iomanip>

namespace fs = std::filesystem;

// ANSI Terminal Colors
#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"

struct TestCaseResult {
    std::string name;
    bool passed;
    std::string details;
    uint64_t origSize;
    uint64_t compSize;
    double ratio;
};

std::vector<TestCaseResult> testResults;

void logTest(const std::string& name, bool passed, const std::string& details = "",
             uint64_t orig = 0, uint64_t comp = 0, double ratio = 0.0) {
    TestCaseResult r;
    r.name = name;
    r.passed = passed;
    r.details = details;
    r.origSize = orig;
    r.compSize = comp;
    r.ratio = ratio;
    testResults.push_back(r);

    std::cout << (passed ? ANSI_GREEN "[PASS] " : ANSI_RED "[FAIL] ") << ANSI_RESET
              << std::left << std::setw(32) << name;
    if (!details.empty()) {
        std::cout << " (" << details << ")";
    }
    std::cout << std::endl;
}

// Helper to write raw bytes
void writeBinaryFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (!data.empty()) {
        out.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
}

// Helper to test a valid file roundtrip
bool testRoundtrip(const std::string& testName, const std::string& filePath,
                   HuffmanEncoder& encoder, HuffmanDecoder& decoder, const std::string& tempDir) {
    std::string base = FileUtils::getBaseFilename(filePath);
    std::string archivePath = tempDir + "/" + base + ".hfx";
    std::string restoredPath = tempDir + "/restored_" + base;

    // 1. Compress
    CompressionResult compRes = encoder.compressFile(filePath, archivePath);
    if (!compRes.success) {
        logTest(testName, false, "Compression failed: " + compRes.errorMessage);
        return false;
    }

    // 2. Decompress
    DecompressionResult decompRes = decoder.decompressFile(archivePath, restoredPath, tempDir);
    if (!decompRes.success) {
        logTest(testName, false, "Decompression failed: " + decompRes.errorMessage);
        return false;
    }

    // 3. Binary comparison
    bool identical = FileUtils::binaryCompareFiles(filePath, restoredPath);
    if (!identical) {
        logTest(testName, false, "Byte-for-byte mismatch between original and restored file!");
        return false;
    }

    std::ostringstream details;
    details << "Orig: " << compRes.stats.originalSize << "B -> Comp: " << compRes.stats.compressedSize
            << "B, Ratio: " << std::fixed << std::setprecision(2) << compRes.stats.compressionRatio
            << ", Saved: " << std::fixed << std::setprecision(1) << compRes.stats.spaceSavedPercent << "%";

    logTest(testName, true, details.str(), compRes.stats.originalSize,
            compRes.stats.compressedSize, compRes.stats.compressionRatio);
    return true;
}

int main() {
    std::cout << ANSI_CYAN << ANSI_BOLD
              << "========================================================\n"
              << "  HuffmanX — Automated Comprehensive Test Suite\n"
              << "  Verification of DSA Core Engine & Edge Cases\n"
              << "========================================================\n"
              << ANSI_RESET << std::endl;

    std::string testDir = "test_workspace";
    fs::create_directories(testDir);

    HuffmanEncoder encoder;
    HuffmanDecoder decoder;

    // 1. Empty file (0 bytes)
    {
        std::string path = testDir + "/empty.bin";
        writeBinaryFile(path, {});
        testRoundtrip("1. Empty File (0 Bytes)", path, encoder, decoder, testDir);
    }

    // 2. Single-character text file
    {
        std::string path = testDir + "/single_char.txt";
        writeBinaryFile(path, {'Z'});
        testRoundtrip("2. Single Character File", path, encoder, decoder, testDir);
    }

    // 3. Repeated-character file (5000 identical 'A's)
    {
        std::string path = testDir + "/repeated.txt";
        std::vector<uint8_t> data(5000, 'A');
        writeBinaryFile(path, data);
        testRoundtrip("3. Repeated-Character (5KB 'A')", path, encoder, decoder, testDir);
    }

    // 4. Ordinary text file
    {
        std::string path = testDir + "/prose.txt";
        std::string prose = "Huffman coding is a particular type of optimal prefix code that is "
                            "commonly used for lossless data compression. The process of finding or "
                            "using such a code proceeds by means of Huffman's algorithm, developed by "
                            "David A. Huffman while he was a Sc.D. student at MIT, and published in the "
                            "1952 paper 'A Method for the Construction of Minimum-Redundancy Codes'.\n"
                            "The output from Huffman's algorithm can be viewed as a variable-length code table "
                            "for encoding a source symbol. The algorithm derives this table from the estimated "
                            "probability or frequency of occurrence for each possible value of the source symbol.";
        std::vector<uint8_t> data(prose.begin(), prose.end());
        writeBinaryFile(path, data);
        testRoundtrip("4. Ordinary Text File", path, encoder, decoder, testDir);
    }

    // 5. Unicode UTF-8 multilingual text
    {
        std::string path = testDir + "/unicode.txt";
        std::string utf8Text = "Hello World! नमस्ते दुनिया! こんにちは世界! مرحبا بالعالم! 🚀🎉🔥💻\n"
                               "Multilingual text testing byte-level lossless preservation of UTF-8 encoding.";
        std::vector<uint8_t> data(utf8Text.begin(), utf8Text.end());
        writeBinaryFile(path, data);
        testRoundtrip("5. Unicode UTF-8 Multilingual", path, encoder, decoder, testDir);
    }

    // 6. File with all 256 byte values
    {
        std::string path = testDir + "/all_256_bytes.bin";
        std::vector<uint8_t> data;
        for (int rep = 0; rep < 16; ++rep) {
            for (int b = 0; b < 256; ++b) {
                data.push_back(static_cast<uint8_t>(b));
            }
        }
        writeBinaryFile(path, data);
        testRoundtrip("6. All 256 Byte Values (4KB)", path, encoder, decoder, testDir);
    }

    // 7. Random binary file (high entropy)
    {
        std::string path = testDir + "/random.bin";
        std::vector<uint8_t> data(32768);
        std::mt19937 rng(42);
        for (auto& b : data) {
            b = static_cast<uint8_t>(rng() & 0xFF);
        }
        writeBinaryFile(path, data);
        testRoundtrip("7. High-Entropy Random Binary", path, encoder, decoder, testDir);
    }

    // 8. PDF file sample
    {
        std::string path = testDir + "/sample.pdf";
        std::string pdfContent = "%PDF-1.4\n"
                                 "1 0 obj << /Type /Catalog /Pages 2 0 R >> endobj\n"
                                 "2 0 obj << /Type /Pages /Kids [3 0 R] /Count 1 >> endobj\n"
                                 "3 0 obj << /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] >> endobj\n"
                                 "xref\n0 4\n0000000000 65535 f \n0000000009 00000 n \n"
                                 "0000000058 00000 n \n0000000115 00000 n \n"
                                 "trailer << /Size 4 /Root 1 0 R >>\nstartxref\n190\n%%EOF\n";
        std::vector<uint8_t> data(pdfContent.begin(), pdfContent.end());
        writeBinaryFile(path, data);
        testRoundtrip("8. PDF Document Format", path, encoder, decoder, testDir);
    }

    // 9. DOCX / ZIP container sample
    {
        std::string path = testDir + "/sample.docx";
        // PK\x03\x04 signature + mock header
        std::vector<uint8_t> data = {'P', 'K', 0x03, 0x04, 0x14, 0x00, 0x00, 0x00, 0x08, 0x00};
        data.resize(512, 0x55);
        writeBinaryFile(path, data);
        testRoundtrip("9. DOCX (ZIP Container) Format", path, encoder, decoder, testDir);
    }

    // 10. JPEG image sample
    {
        std::string path = testDir + "/sample.jpg";
        std::vector<uint8_t> data = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 'J', 'F', 'I', 'F', 0x00, 0x01, 0x01};
        data.resize(1024, 0x33);
        data[data.size() - 2] = 0xFF;
        data[data.size() - 1] = 0xD9;
        writeBinaryFile(path, data);
        testRoundtrip("10. JPEG Image Format", path, encoder, decoder, testDir);
    }

    // 11. PNG image sample
    {
        std::string path = testDir + "/sample.png";
        std::vector<uint8_t> data = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
        data.resize(1024, 0x42);
        writeBinaryFile(path, data);
        testRoundtrip("11. PNG Image Format", path, encoder, decoder, testDir);
    }

    // 12. MP4 video sample
    {
        std::string path = testDir + "/sample.mp4";
        std::vector<uint8_t> data = {0x00, 0x00, 0x00, 0x20, 'f', 't', 'y', 'p', 'i', 's', 'o', 'm'};
        data.resize(2048, 0x77);
        writeBinaryFile(path, data);
        testRoundtrip("12. MP4 Video Format", path, encoder, decoder, testDir);
    }

    // 13. MP3 audio sample
    {
        std::string path = testDir + "/sample.mp3";
        std::vector<uint8_t> data = {'I', 'D', '3', 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        data.resize(1536, 0x88);
        writeBinaryFile(path, data);
        testRoundtrip("13. MP3 Audio Format", path, encoder, decoder, testDir);
    }

    // 14. Large file (1 MB mixed patterns)
    {
        std::string path = testDir + "/large_file.bin";
        std::vector<uint8_t> data(1048576); // 1 MB
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = static_cast<uint8_t>((i % 64) ^ ((i >> 8) & 0x1F));
        }
        writeBinaryFile(path, data);
        testRoundtrip("14. Large 1MB File Streaming", path, encoder, decoder, testDir);
    }

    // 15. Corrupted Archive: Invalid Magic Signature
    {
        std::string path = testDir + "/bad_magic.hfx";
        std::vector<uint8_t> data = {'B', 'A', 'D', '!', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        writeBinaryFile(path, data);
        DecompressionResult res = decoder.decompressFile(path, testDir + "/bad_out.bin", testDir);
        bool correctlyRejected = (!res.success && res.errorMessage.find("Invalid magic") != std::string::npos);
        logTest("15. Rejection of Invalid Magic", correctlyRejected, res.errorMessage);
    }

    // 16. Corrupted Archive: Truncated Archive
    {
        std::string path = testDir + "/truncated.hfx";
        std::vector<uint8_t> data = {'H', 'F', 'X', '1', 1}; // Only 5 bytes
        writeBinaryFile(path, data);
        DecompressionResult res = decoder.decompressFile(path, testDir + "/trunc_out.bin", testDir);
        bool correctlyRejected = (!res.success && res.errorMessage.find("truncated") != std::string::npos);
        logTest("16. Rejection of Truncated Archive", correctlyRejected, res.errorMessage);
    }

    // 17. Corrupted Archive: Tampered Bitstream / Checksum Mismatch
    {
        std::string srcPath = testDir + "/prose.txt";
        std::string archPath = testDir + "/tampered.hfx";
        encoder.compressFile(srcPath, archPath);

        // Tamper with the last byte
        std::fstream fs(archPath, std::ios::in | std::ios::out | std::ios::binary);
        fs.seekp(-1, std::ios::end);
        char b = 0;
        fs.read(&b, 1);
        b ^= 0xFF; // Flip all bits
        fs.seekp(-1, std::ios::end);
        fs.write(&b, 1);
        fs.close();

        DecompressionResult res = decoder.decompressFile(archPath, testDir + "/tampered_out.txt", testDir);
        bool caught = (!res.success);
        logTest("17. Detection of Checksum Mismatch", caught, res.errorMessage);
    }

    // 18. Non-Existent Input File Handling
    {
        CompressionResult res = encoder.compressFile("non_existent_file_xyz.bin", testDir + "/out.hfx");
        bool handled = (!res.success && res.errorMessage.find("does not exist") != std::string::npos);
        logTest("18. Graceful Non-Existent File Handling", handled, res.errorMessage);
    }

    // Summary Table
    std::cout << "\n" << ANSI_CYAN << ANSI_BOLD
              << "========================================================\n"
              << "  TEST EXECUTION SUMMARY\n"
              << "========================================================\n"
              << ANSI_RESET;

    int passedCount = 0;
    for (const auto& r : testResults) {
        if (r.passed) passedCount++;
    }

    std::cout << ANSI_BOLD << "Total Tests: " << testResults.size() << " | "
              << ANSI_GREEN << "Passed: " << passedCount << " | "
              << (passedCount == (int)testResults.size() ? ANSI_GREEN : ANSI_RED)
              << "Failed: " << (testResults.size() - passedCount) << ANSI_RESET << "\n\n";

    if (passedCount == (int)testResults.size()) {
        std::cout << ANSI_GREEN << ANSI_BOLD << "ALL " << passedCount
                  << " AUTOMATED UNIT & ROUNDTRIP TESTS PASSED WITH 100% SUCCESS!" << ANSI_RESET << "\n";
    }

    return (passedCount == (int)testResults.size()) ? 0 : 1;
}
