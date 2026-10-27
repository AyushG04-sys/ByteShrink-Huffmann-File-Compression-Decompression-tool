#ifndef ARCHIVE_FORMAT_H
#define ARCHIVE_FORMAT_H

#include <cstdint>
#include <string>
#include <vector>
#include <istream>
#include <ostream>

constexpr uint8_t ARCHIVE_FLAG_HAS_FILENAME = 0x01;
constexpr uint8_t ARCHIVE_FLAG_HAS_CRC32    = 0x02;
constexpr uint8_t ARCHIVE_FLAG_STORE_RAW    = 0x04;

#pragma pack(push, 1)
/**
 * @brief 23-byte binary archive header specification.
 */
struct ArchiveHeader {
    char magic[4];                ///< Must be "HFX1" (0x48, 0x46, 0x58, 0x31)
    uint8_t version;              ///< Format version (currently 1)
    uint8_t flags;                ///< Feature flags (CRC32, Filename, etc.)
    uint64_t originalSize;        ///< Original file size in bytes
    uint32_t originalCrc32;       ///< CRC-32 checksum of original file bytes
    uint8_t validBitsInLastByte;  ///< Number of valid bits in last byte (0..8)
    uint16_t filenameLength;      ///< Length of stored original filename (0 if omitted)
    uint16_t symbolCount;         ///< Number of distinct symbols in table (0..256)
};

/**
 * @brief Entry in the serialized frequency table.
 */
struct SerializedSymbolEntry {
    uint8_t symbol;               ///< Byte value (0..255)
    uint64_t frequency;           ///< Count of occurrences
};
#pragma pack(pop)

/**
 * @brief Metadata parsed from an archive.
 */
struct ArchiveMetadata {
    ArchiveHeader header;
    std::string originalFilename;
    std::vector<uint64_t> frequencyTable; // 256 elements
    bool isValid;
    std::string errorMessage;

    ArchiveMetadata() : frequencyTable(256, 0), isValid(false) {}
};

class ArchiveFormat {
public:
    static const char MAGIC[4];
    static const uint8_t CURRENT_VERSION = 1;
    static const size_t MAX_FILENAME_LENGTH = 1024;

    /**
     * @brief Writes the complete archive header, filename, and symbol table.
     */
    static bool writeHeader(std::ostream& stream,
                            uint64_t originalSize,
                            uint32_t crc32,
                            uint8_t validBitsInLastByte,
                            const std::string& filename,
                            const std::vector<uint64_t>& frequencies,
                            bool storeRaw = false);

    /**
     * @brief Reads and validates the archive header, filename, and symbol table.
     */
    static ArchiveMetadata readHeader(std::istream& stream, size_t totalArchiveSize);

    /**
     * @brief Sanitizes a filename to protect against path traversal attacks.
     * Strips directory components, illegal characters, and ".." sequences.
     */
    static std::string sanitizeFilename(const std::string& rawName);

    /**
     * @brief Updates the validBitsInLastByte field in an already written archive file.
     */
    static bool updateLastByteValidBits(const std::string& filePath, uint8_t validBits);
};

#endif // ARCHIVE_FORMAT_H
