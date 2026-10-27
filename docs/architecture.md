# ByteShrink — System Architecture & Modular Design

## 1. High-Level Architectural Overview

ByteShrink is structured using a strict separation of concerns across three core architectural tiers:
1. **Core DSA & File Engine Tier (`include/`, `src/`)**: Pure C++20 implementation of the Huffman algorithm, custom Min-Heap data structure, bitstream serialization, CRC-32 integrity calculation, and binary file I/O.
2. **Presentation & Studio UI Tier (`web/`, `gui/`)**: Web-based retro-terminal studio interface and native desktop interface with interactive 2D canvas rendering for the Algorithm Explorer.
3. **Verification & Quality Assurance Tier (`tests/`)**: Automated test runner executing 18 roundtrip and negative/corruption test cases with byte-for-byte binary validation.

```
+-------------------------------------------------------------------------------+
|                       Desktop User Interface (gui/)                           |
|   +-------------------+  +-------------------+  +-------------------------+   |
|   |   Dashboard View  |  |  Compress View    |  | Decompress View         |   |
|   +-------------------+  +-------------------+  +-------------------------+   |
|   +------------------------------------------+  +-------------------------+   |
|   |  Algorithm Explorer & 2D Tree Visualizer |  | Settings & Help View    |   |
|   +------------------------------------------+  +-------------------------+   |
+-------------------------------------------------------------------------------+
                                      |
                     [ Worker Thread / Decoupled Calls ]
                                      v
+-------------------------------------------------------------------------------+
|                      Core DSA & Compression Engine (src/)                     |
|                                                                               |
|   [ Frequency Counter ] ---> [ MinHeap <HuffmanNode*> ] ---> [ Huffman Tree ] |
|            |                           |                            |         |
|            v                           v                            v         |
|        [ CRC32 ]               [ Greedy Merges ]            [ Code Generator ]|
|            |                                                        |         |
|            v                                                        v         |
|   [ ArchiveFormat ] <--------------------------------------- [ BitWriter ]   |
|   Header (23B) + Filename + Serialized Frequency Table + Bitstream Payload    |
|                                                                               |
|                                      ^                                        |
|                                      | Decompression                          |
|                                      v                                        |
|   [ BitReader ] <------------------------------------------- [ Root Traverser]|
|         |                                                           |         |
|         +-----------------> [ CRC32 Verifier ] <--------------------+         |
|                                     |                                         |
|                                     v                                         |
|                         [ Exact Restored File ]                               |
+-------------------------------------------------------------------------------+
```

---

## 2. Component Directory Breakdown

```
HuffmanX/
├── CMakeLists.txt              # Cross-platform CMake build configuration
├── README.md                   # Comprehensive documentation and viva guide
├── docs/
│   ├── architecture.md         # System design and component roles (this file)
│   ├── compression_format.md   # Exact binary archive specification
│   └── algorithm_explanation.md# DSA theory, proofs, and viva Q&A
├── include/
│   ├── ArchiveFormat.h         # Archive header structures and security sanitization
│   ├── BitReader.h             # Bit-level unpacking from buffered input streams
│   ├── BitWriter.h             # Bit-level packing into buffered output streams
│   ├── Checksum.h              # CRC-32 (0xEDB88320) polynomial calculation
│   ├── CompressionStats.h      # Performance metrics and sizing calculations
│   ├── FileUtils.h             # Binary file signatures, path helpers, safe atomic rename
│   ├── HuffmanDecoder.h        # Decompression engine with integrity verification
│   ├── HuffmanEncoder.h        # Compression engine with streaming passes
│   ├── HuffmanNode.h           # Tree node and deterministic min-heap comparators
│   ├── HuffmanTree.h           # Binary tree construction and 2D visual layout
│   └── MinHeap.h               # Custom templated priority queue (siftUp, siftDown)
├── src/
│   ├── ArchiveFormat.cpp
│   ├── BitReader.cpp
│   ├── BitWriter.cpp
│   ├── Checksum.cpp
│   ├── CompressionStats.cpp
│   ├── FileUtils.cpp
│   ├── HuffmanDecoder.cpp
│   ├── HuffmanEncoder.cpp
│   ├── HuffmanNode.cpp
│   ├── HuffmanTree.cpp
│   └── main.cpp                # Hybrid entrypoint (Desktop GUI + CLI)
├── gui/
│   ├── AppWindow.h / .cpp      # Win32 lifecycle, Direct3D 11, background worker thread
│   ├── NativeFileDialog.h/.cpp # Native Windows file/folder picker integration
│   ├── Theme.h / .cpp          # Modern Slate Dark and Crisp Light themes
│   └── TreeRenderer.h / .cpp   # 2D interactive canvas renderer with pan/zoom/hover
├── tests/
│   └── test_suite.cpp          # 18 automated roundtrip and negative unit tests
└── third_party/
    └── imgui/                  # Dear ImGui library + Win32/DX11 backends
```

---

## 3. Class Relationships and Design Patterns

1. **Greedy Priority Queue Pattern (`MinHeap` + `HuffmanNodeComparator`)**:
   - Leaf nodes are inserted into a custom binary min-heap array.
   - The greedy choice extracts the two subtrees with the smallest frequencies at each iteration:
     $$\text{Combined Frequency} = \text{Freq}(L) + \text{Freq}(R)$$
   - Deterministic tie-breaking is enforced by comparing the minimum byte value in each subtree (`minByteInSubtree`), ensuring uniform tree generation across systems.

2. **Producer-Consumer Bitstream Pattern (`BitWriter` and `BitReader`)**:
   - `BitWriter` buffers bits MSB-first in a `uint8_t currentByte` and writes 64KB chunks to `std::ostream`.
   - On completion, `BitWriter::flush()` pads the last byte to 8 bits and writes the valid bit count into the archive header.
   - `BitReader` decodes bits one by one without overshooting past the valid bits in the final byte.

3. **Safe Two-Phase Commit for File I/O (`FileUtils::createTempFilePath` & `atomicReplaceFile`)**:
   - Output files are written to a temporary sibling file (`<target>.tmp_<timestamp>.<ext>`).
   - Only after full completion and checksum validation is the temporary file atomically renamed to the final destination, completely eliminating the risk of accidental overwriting or corrupt partial files.

4. **Thread-Safe Responsive GUI (`AppWindow`)**:
   - File compression and decompression execute on a background `std::thread`.
   - The GUI runs at a continuous 60 FPS without UI freezing.
   - Atomic flags (`cancelCompressFlag`, `cancelDecompressFlag`) allow the user to safely abort operations mid-stream.
