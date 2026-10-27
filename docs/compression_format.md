# ByteShrink (.hfx) — Binary Archive Format Specification

**Format Name:** ByteShrink Compressed Archive  
**Standard Extension:** `.hfx`  
**Current Version:** 1 (0x01)  
**Byte Ordering:** Little-Endian (Standard x86_64 architecture)

---

## 1. Physical Byte Layout Overview

A ByteShrink compressed archive consists of four sequential sections:
```
+-------------------------------------------------------------------------------+
| SECTION 1: Fixed-Size Archive Header (23 Bytes)                              |
+-------------------------------------------------------------------------------+
| SECTION 2: Original Filename (Variable: 0 to 1024 Bytes, UTF-8)               |
+-------------------------------------------------------------------------------+
| SECTION 3: Serialized Symbol Frequency Table (Variable: symbolCount * 9 Bytes)|
+-------------------------------------------------------------------------------+
| SECTION 4: Compressed Variable-Length Bitstream Payload (Remaining Bytes)    |
+-------------------------------------------------------------------------------+
```

---

## 2. Section 1: Fixed-Size Header (23 Bytes)

All multi-byte numeric fields are encoded in standard unsigned little-endian format. The structure is packed with 1-byte alignment (`#pragma pack(push, 1)`):

| Byte Offset | Field Name | Data Type | Size (Bytes) | Description |
| :---: | :---: | :---: | :---: | :--- |
| **0x00 – 0x03** | `magic` | `char[4]` | 4 | Magic file signature: ASCII `'H'`, `'F'`, `'X'`, `'1'` (`0x48, 0x46, 0x58, 0x31`) |
| **0x04** | `version` | `uint8_t` | 1 | Archive format version (currently `1`) |
| **0x05** | `flags` | `uint8_t` | 1 | Feature flags bitmap (see Section 2.1) |
| **0x06 – 0x0D** | `originalSize` | `uint64_t` | 8 | Exact uncompressed file size in bytes |
| **0x0E – 0x11** | `originalCrc32` | `uint32_t` | 4 | IEEE 802.3 CRC-32 checksum of original bytes |
| **0x12** | `validBitsInLastByte` | `uint8_t` | 1 | Number of valid bits in the final payload byte (`0` to `8`) |
| **0x13 – 0x14** | `filenameLength` | `uint16_t` | 2 | Length of stored original filename in bytes (`0` to `1024`) |
| **0x15 – 0x16** | `symbolCount` | `uint16_t` | 2 | Number of distinct active symbols in table (`0` to `256`) |

### 2.1 Feature Flags Bitmap (`flags`)
- **Bit 0 (`0x01`) — `ARCHIVE_FLAG_HAS_FILENAME`**: When set, Section 2 contains the original source filename.
- **Bit 1 (`0x02`) — `ARCHIVE_FLAG_HAS_CRC32`**: When set, `originalCrc32` contains an integrity checksum.
- **Bit 2 (`0x04`) — `ARCHIVE_FLAG_STORE_RAW`**: Stored fallback mode (used when uncompressed payload is preferred).

---

## 3. Section 2: Original Filename (Variable Length)

Immediately follows the fixed header at offset `0x17`:
- **Length:** Specified by `filenameLength` in Section 1.
- **Encoding:** UTF-8 character string without a null terminator.
- **Security Rule:** Filenames are sanitized on both write and read. Directory path separators (`/`, `\`, `:`), wildcards, and path traversal tokens (`..`) are stripped to prevent arbitrary file overwrite attacks.

---

## 4. Section 3: Serialized Symbol Frequency Table

Immediately follows the filename string:
- **Length:** `symbolCount * 9` bytes.
- For each of the `symbolCount` active symbols, a 9-byte entry is serialized:

| Field Name | Type | Size | Description |
| :---: | :---: | :---: | :--- |
| `symbol` | `uint8_t` | 1 Byte | Byte value (`0x00` to `0xFF`) |
| `frequency` | `uint64_t` | 8 Bytes | Exact occurrence count of this symbol |

### Mathematical Validation:
On decompression, the decoder verifies:
$$\sum_{i=1}^{\text{symbolCount}} \text{entry}[i].\text{frequency} == \text{header}.\text{originalSize}$$
If this equality does not hold, the archive is immediately rejected as corrupted before any memory allocation or bitstream decoding begins.

---

## 5. Section 4: Compressed Bitstream Payload

The remainder of the `.hfx` file constitutes the compressed bitstream:
- Bits are packed into bytes **Most-Significant Bit (MSB) First**:
  - First bit of code is placed at bit 7 (`0x80`).
  - Eighth bit of code is placed at bit 0 (`0x01`).
- **Final Byte Handling:**
  - If total encoded bits is not a multiple of 8, remaining bits are shifted to the upper positions, and the lower positions are padded with zero bits.
  - The `validBitsInLastByte` field in the header records exactly how many upper bits in this final byte belong to the encoded stream (from 1 to 8).
  - If the original file was 0 bytes, `validBitsInLastByte` is set to 0 and no payload bytes are read.

---

## 6. Security & Tamper Defenses

1. **Header Boundary Verification**: The file size must be at least `sizeof(ArchiveHeader) + filenameLength + (symbolCount * 9)`.
2. **Alphabet Limit Protection**: `symbolCount` cannot exceed 256 for a byte alphabet. Any value `> 256` triggers an immediate corrupt archive error.
3. **Integer Overflow Prevention**: All size calculations use 64-bit unsigned integers (`uint64_t`).
4. **Path Traversal Sanitization**: Restored files can only be saved in the user-selected destination directory; absolute paths or directory climbing (`../../`) stored in malicious archives are neutralized.
5. **End-to-End CRC-32 Verification**: If even a single bit in the bitstream or metadata is altered, decompression aborts with a descriptive checksum mismatch error, preventing corrupted data from being mistaken for valid files.
