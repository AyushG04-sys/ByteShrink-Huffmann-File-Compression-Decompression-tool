#include "ArchiveFormat.h"
#include <cstring>
#include <fstream>
#include <algorithm>
#include <cctype>

using namespace std;

const char ArchiveFormat::MAGIC[4] = {'H', 'F', 'X', '1'};

string ArchiveFormat::sanitizeFilename(const string& rawName) {
    if (rawName.empty()) {
        return "restored_file.bin";
    }

    string clean;
    clean.reserve(rawName.size());

    // Extract basename only (strip directories)
    size_t lastSlash = rawName.find_last_of("/\\:");
    string base = (lastSlash == string::npos) ? rawName : rawName.substr(lastSlash + 1);

    for (char c : base) {
        // Disallow dangerous or path traversal characters
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
            c == '"' || c == '<' || c == '>' || c == '|' || static_cast<unsigned char>(c) < 32) {
            clean += '_';
        } else {
            clean += c;
        }
    }

    // Protect against names like ".." or "."
    if (clean == "." || clean == ".." || clean.empty()) {
        return "restored_file.bin";
    }

    // Limit length to avoid filesystem limits
    if (clean.size() > 255) {
        clean = clean.substr(0, 255);
    }

    return clean;
}

bool ArchiveFormat::writeHeader(ostream& stream,
                                uint64_t originalSize,
                                uint32_t crc32,
                                uint8_t validBitsInLastByte,
                                const string& filename,
                                const vector<uint64_t>& frequencies,
                                bool storeRaw) {
    ArchiveHeader header;
    memcpy(header.magic, MAGIC, 4);
    header.version = CURRENT_VERSION;
    header.flags = 0;

    string safeName = sanitizeFilename(filename);
    if (!safeName.empty()) {
        header.flags |= ARCHIVE_FLAG_HAS_FILENAME;
        header.filenameLength = static_cast<uint16_t>(safeName.size());
    } else {
        header.filenameLength = 0;
    }

    header.flags |= ARCHIVE_FLAG_HAS_CRC32;
    if (storeRaw) {
        header.flags |= ARCHIVE_FLAG_STORE_RAW;
    }

    header.originalSize = originalSize;
    header.originalCrc32 = crc32;
    header.validBitsInLastByte = validBitsInLastByte;

    // Count non-zero symbols
    uint16_t nonZeroSymbols = 0;
    if (!storeRaw) {
        for (int i = 0; i < 256; ++i) {
            if (i < static_cast<int>(frequencies.size()) && frequencies[i] > 0) {
                nonZeroSymbols++;
            }
        }
    }
    header.symbolCount = nonZeroSymbols;

    // Write fixed header
    stream.write(reinterpret_cast<const char*>(&header), sizeof(ArchiveHeader));
    if (!stream) return false;

    // Write filename if present
    if (header.filenameLength > 0) {
        stream.write(safeName.data(), safeName.size());
        if (!stream) return false;
    }

    // Write frequency table entries
    if (!storeRaw && header.symbolCount > 0) {
        for (int i = 0; i < 256; ++i) {
            if (i < static_cast<int>(frequencies.size()) && frequencies[i] > 0) {
                SerializedSymbolEntry entry;
                entry.symbol = static_cast<uint8_t>(i);
                entry.frequency = frequencies[i];
                stream.write(reinterpret_cast<const char*>(&entry), sizeof(SerializedSymbolEntry));
                if (!stream) return false;
            }
        }
    }

    return true;
}

ArchiveMetadata ArchiveFormat::readHeader(istream& stream, size_t totalArchiveSize) {
    ArchiveMetadata meta;
    meta.isValid = false;

    if (totalArchiveSize < sizeof(ArchiveHeader)) {
        meta.errorMessage = "File is too small to be a valid HuffmanX archive (truncated header).";
        return meta;
    }

    ArchiveHeader header;
    stream.read(reinterpret_cast<char*>(&header), sizeof(ArchiveHeader));
    if (!stream) {
        meta.errorMessage = "Failed to read archive header from stream.";
        return meta;
    }

    // Validate magic
    if (memcmp(header.magic, MAGIC, 4) != 0) {
        meta.errorMessage = "Invalid magic number. This file was not created by HuffmanX.";
        return meta;
    }

    // Validate version
    if (header.version != CURRENT_VERSION) {
        meta.errorMessage = "Unsupported archive format version (" + to_string(header.version) + ").";
        return meta;
    }

    // Validate symbol count (cannot exceed 256 for a byte alphabet)
    if (header.symbolCount > 256) {
        meta.errorMessage = "Corrupted archive: symbol count exceeds 256.";
        return meta;
    }

    // Validate filename length
    if (header.filenameLength > MAX_FILENAME_LENGTH) {
        meta.errorMessage = "Corrupted archive: filename length exceeds safe limits.";
        return meta;
    }

    // Validate validBitsInLastByte
    if (header.validBitsInLastByte > 8) {
        meta.errorMessage = "Corrupted archive: invalid bit count in final byte.";
        return meta;
    }

    size_t expectedMinSize = sizeof(ArchiveHeader) + header.filenameLength +
                             (header.symbolCount * sizeof(SerializedSymbolEntry));
    if (totalArchiveSize < expectedMinSize) {
        meta.errorMessage = "Corrupted archive: file is truncated before metadata finishes.";
        return meta;
    }

    // Read filename
    if (header.filenameLength > 0) {
        string rawName(header.filenameLength, '\0');
        stream.read(&rawName[0], header.filenameLength);
        if (!stream) {
            meta.errorMessage = "Failed to read original filename from archive.";
            return meta;
        }
        meta.originalFilename = sanitizeFilename(rawName);
    } else {
        meta.originalFilename = "restored_file.bin";
    }

    // Read frequency table
    meta.frequencyTable.assign(256, 0);
    uint64_t sumFreq = 0;

    for (uint16_t i = 0; i < header.symbolCount; ++i) {
        SerializedSymbolEntry entry;
        stream.read(reinterpret_cast<char*>(&entry), sizeof(SerializedSymbolEntry));
        if (!stream) {
            meta.errorMessage = "Truncated frequency table in archive.";
            return meta;
        }
        meta.frequencyTable[entry.symbol] = entry.frequency;
        sumFreq += entry.frequency;
    }

    // Verify frequency sum matches original size (for compressed mode)
    if (!(header.flags & ARCHIVE_FLAG_STORE_RAW)) {
        if (sumFreq != header.originalSize) {
            meta.errorMessage = "Corrupted archive: sum of symbol frequencies (" +
                                to_string(sumFreq) + ") does not match original file size (" +
                                to_string(header.originalSize) + ").";
            return meta;
        }
    }

    meta.header = header;
    meta.isValid = true;
    return meta;
}

bool ArchiveFormat::updateLastByteValidBits(const string& filePath, uint8_t validBits) {
    fstream fs(filePath, ios::in | ios::out | ios::binary);
    if (!fs) return false;

    // Offset of validBitsInLastByte in ArchiveHeader:
    // magic(4) + version(1) + flags(1) + originalSize(8) + originalCrc32(4) = 18 bytes
    size_t offset = 18;
    fs.seekp(static_cast<streamoff>(offset), ios::beg);
    if (!fs) return false;

    fs.write(reinterpret_cast<const char*>(&validBits), 1);
    return fs.good();
}
