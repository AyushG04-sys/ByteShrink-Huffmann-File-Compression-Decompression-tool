# ByteShrink — Smart File Compression Studio

**Privacy-First, Offline Lossless File Compression Powered by Huffman Coding, Min-Heaps, and Binary Trees**

*Developed for College-Level Computer Science & Engineering Data Structures and Algorithms (DSA) Coursework, Laboratory Evaluations, and Viva Voce Presentations.*

---

## 1. Project Overview

**ByteShrink** is a modular, high-performance local-first file compression and decompression application and educational studio powered by the **Huffman Coding algorithm**, **Min-Heaps (Priority Queues)**, **Binary Trees**, and **Greedy Algorithms**.

The application operates directly on raw binary byte streams (preserving all 256 byte values from `0x00` to `0xFF`). It supports documents (`.pdf`, `.docx`, `.txt`), images (`.jpg`, `.png`, `.bmp`), video (`.mp4`), audio (`.mp3`), and general binary files, restoring the original data **byte-for-byte** with IEEE 802.3 CRC-32 integrity verification.

ByteShrink provides two unified interfaces:
1. **ByteShrink Studio (Web / Local-First UI)**: A retro-terminal industrial interface featuring a dark palette (`#0B0B0E`), vivid orange (`#FF5A0A`) and purple (`#7844F9`) accents, hazard diagonal stripes, notch borders, an interactive 2D HTML5 canvas Huffman Tree visualizer with drag-to-pan and scroll-to-zoom, Welcome onboarding flow, direct `.hfx` and restored file downloads, and a zero-dependency local bridge.
2. **ByteShrink Native Engine (C++ CLI & GUI)**: A native C++ compiled binary (`build/ByteShrink.exe`) for headless scripting, automated evaluation, and hardware-accelerated desktop rendering via Dear ImGui and Direct3D 11.

---

## 2. Key Features

- **Privacy-First & 100% Offline**: All compression, decompression, and algorithm visualizations run locally on your device. No cloud uploads, telemetry, or external internet connections are required.
- **Student-Friendly Clean C++ Engine**: All C++ implementation files (`src/*.cpp`) use `using namespace std;` instead of redundant `std::` prefixes, making the codebase clean, legible, and directly aligned with college DSA instruction standards and viva presentation expectations.
- **Custom Binary Compression Engine**: Pure C++ implementation of Huffman coding with bit packing (`BitWriter`) and unpacking (`BitReader`) using MSB-first bitwise stream manipulation and 64KB I/O block buffering.
- **Complete DSA Demonstration**: Custom templated Min-Heap (`include/MinHeap.h`), greedy binary tree generation, deterministic tie-breaking, and recursive prefix-free code assignment.
- **Welcome / Introduction Onboarding Screen**: New sessions greet users with an introduction to Huffman coding, supported formats, privacy guarantees, and a prominent **⚡ GET STARTED** button leading to the Dashboard.
- **Algorithm Explorer & 2D Tree Visualizer**:
  - Live 2D graphical representation of the Huffman Tree with drag-to-pan, scroll-to-zoom, branch bit labels (`0`/`1`), and node inspection tooltips.
  - Sortable 256-symbol byte frequency table and variable-length prefix code inspector.
  - Priority queue timeline detailing every greedy merge step.
  - Interactive playground allowing real-time tree generation from custom user text or file byte streams.
- **End-to-End Data Integrity**: IEEE 802.3 CRC-32 checksum calculation guarantees exact bit-for-bit file reconstruction.
- **Security & Path Traversal Protection**: Sanitizes metadata and file paths to prevent directory climbing attacks (`../`).
- **Safe Two-Phase Commit**: Writes to temporary files and atomically replaces destinations only upon successful validation.
- **Dual Interface Modes**: Runs as an interactive web/local studio or as a native command-line utility.

---

## 3. Technology Stack

| Component | Technology | Rationale |
| :--- | :--- | :--- |
| **Core Algorithm** | C++ (C++17/C++20 compliant) | High-performance compiled code with direct pointer control and bitwise I/O. |
| **Coding Style** | `using namespace std;` in `src/*.cpp` | Clean, student-friendly code structure ideal for college faculty evaluation. |
| **DSA Engine** | Custom Min-Heap, Binary Tree | Educational clarity and complete control over greedy heap operations. |
| **Web Studio UI** | HTML5, CSS3, Vanilla JavaScript | Responsive, industrial retro-terminal aesthetic with 2D HTML5 Canvas. |
| **Local Bridge** | Node.js (Built-in `http`, `fs`, `child_process`) | Zero external npm dependencies; binds to `0.0.0.0:3000` for IPv4/IPv6 support. |
| **Native GUI** | Dear ImGui + Direct3D 11 + Win32 | Lightweight, native desktop execution with zero external DLL runtime dependencies. |
| **Integrity** | CRC-32 (Polynomial `0xEDB88320`) | Fast, collision-resistant checksum verification. |
| **Build System** | CMake 3.20+ / MinGW Makefiles | Standardized, cross-platform build automation. |
| **Compiler** | GCC 14.2 (UCRT64 MinGW-w64) / Clang / MSVC | High-optimization C++ compiler. |

---

## 4. DSA Concepts Demonstrated

1. **Greedy Algorithm**: Combines the two lowest-frequency candidate subtrees at each step without backtracking, provably constructing the optimal prefix-free tree with minimum weighted external path length.
2. **Min-Heap / Priority Queue**: Implements complete binary tree array indexing (`(i-1)/2`, `2i+1`, `2i+2`) with `siftUp` and `siftDown` in $O(\log k)$ time. Total construction runs in $O(k \log k)$.
3. **Binary Tree**: Organizes symbols at leaf nodes with left edges assigned bit `0` and right edges bit `1`.
4. **Tree Traversal (DFS)**: Recursive tree traversal generates variable-length prefix-free codes; bitstream traversal from root to leaf decodes original symbols.
5. **Bit Manipulation & Packing**: Accumulates bits MSB-first in bytes and buffers 64KB I/O blocks for high throughput.
6. **Information Theory & Shannon Entropy**: Calculates the theoretical entropy limit $H(X) = -\sum P(x) \log_2 P(x)$ to explain why high-entropy files (e.g. JPEG, MP4) cannot be further compressed losslessly.

---

## 5. System Architecture & Directory Layout

```
ByteShrink/
├── CMakeLists.txt              # CMake build configuration for C++ engine
├── package.json                # Project scripts (npm start)
├── server.js                   # Zero-dependency local offline bridge server (port 3000)
├── README.md                   # This project manual
│
├── src/                        # === CORE C++ HUFFMAN ENGINE (SHOW TO FACULTY) ===
│   ├── main.cpp                # Hybrid GUI & CLI entrypoint with clean 'using namespace std;'
│   ├── HuffmanEncoder.cpp      # Two-pass streaming compression & frequency analysis
│   ├── HuffmanDecoder.cpp      # Bitstream decompression & tree traversal
│   ├── HuffmanTree.cpp         # Binary tree construction, min-heap merges & code generation
│   ├── HuffmanNode.cpp         # Tree node struct & deterministic tie-breaking logic
│   ├── BitWriter.cpp           # Bit-level packing (MSB first) & flush with valid bit count
│   ├── BitReader.cpp           # Bit-level unpacking & streaming buffer reader
│   ├── ArchiveFormat.cpp       # .hfx 23-byte header read/write & serialization
│   ├── CompressionStats.cpp    # DSA metrics (space saved %, ratio, throughput, Shannon insight)
│   ├── FileUtils.cpp           # Binary file signatures, atomic replacement & directory handling
│   └── Checksum.cpp            # IEEE 802.3 CRC-32 integrity calculator
│
├── include/                    # C++ Header Files & Data Structures
│   ├── ArchiveFormat.h         # .hfx header binary layout (#pragma pack(push, 1))
│   ├── BitReader.h             # BitReader class definition
│   ├── BitWriter.h             # BitWriter class definition
│   ├── Checksum.h              # CRC-32 class definition
│   ├── CompressionStats.h      # Compression metrics structures
│   ├── FileUtils.h             # Cross-platform file utility signatures
│   ├── HuffmanDecoder.h        # HuffmanDecoder class definition
│   ├── HuffmanEncoder.h        # HuffmanEncoder class definition
│   ├── HuffmanNode.h           # HuffmanNode struct & comparator functors
│   ├── HuffmanTree.h           # HuffmanTree class definition & VisualNode layout
│   └── MinHeap.h               # Custom templated Min-Heap priority queue implementation
│
├── web/                        # ByteShrink Studio (Web Frontend)
│   ├── index.html              # Modern retro-terminal studio interface
│   ├── css/
│   │   └── style.css           # Industrial dark orange-and-purple theme
│   └── js/
│       ├── app.js              # State controller, API bridge, dropzones & activity log
│       └── tree_canvas.js      # 2D interactive canvas Huffman tree visualizer
│
├── docs/                       # Comprehensive Academic Documentation
│   ├── architecture.md         # Architectural tiers & design patterns
│   ├── compression_format.md   # Exact binary specification of .hfx
│   └── algorithm_explanation.md# Theory, mathematical proofs, and viva Q&A
│
├── gui/                        # Native Desktop GUI (Dear ImGui)
│   ├── AppWindow.h / .cpp      # Desktop window lifecycle & background worker thread
│   ├── NativeFileDialog.h/.cpp # Native Windows file/folder pickers
│   ├── Theme.h / .cpp          # Industrial dark orange/purple themes
│   └── TreeRenderer.h / .cpp   # ImGui 2D canvas tree renderer
│
├── tests/                      # Automated Test Suite
│   └── test_suite.cpp          # 18 automated unit and roundtrip tests
└── samples/                    # Sample documents, images, and text files
```

---

## 6. Binary Archive Format (`.hfx`)

The custom `.hfx` archive format utilizes a strict 23-byte fixed header followed by dynamic metadata and the compressed bitstream:

```
+--------------------------------------------------------------------------------+
| MAGIC (4B) | VER (1B) | FLAGS (1B) | ORIG_SIZE (8B) | CRC32 (4B) | LAST_BITS(1B)|
| "HFX1"     | 0x01     | Bitfield   | uint64_t       | uint32_t   | 0..8 valid   |
+--------------------------------------------------------------------------------+
| FILENAME_LEN (2B) | SYMBOL_COUNT (2B) | FILENAME (UTF-8, sanitized)            |
+--------------------------------------------------------------------------------+
| SYMBOL TABLE (SYMBOL: 1B + FREQUENCY: 8B per active symbol)                   |
+--------------------------------------------------------------------------------+
| COMPRESSED BITSTREAM (Packed bits MSB-first)                                   |
+--------------------------------------------------------------------------------+
```

### Header Fields Specification

| Field | Type | Size | Description |
| :--- | :---: | :---: | :--- |
| `magic` | `char[4]` | 4 Bytes | Magic signature identifier: ASCII `'H'`, `'F'`, `'X'`, `'1'` |
| `version` | `uint8_t` | 1 Byte | Format version (currently `1`) |
| `flags` | `uint8_t` | 1 Byte | Feature bitfield (`0x01` has filename, `0x02` has CRC, `0x04` store raw) |
| `originalSize` | `uint64_t` | 8 Bytes | Exact uncompressed size in bytes |
| `originalCrc32` | `uint32_t` | 4 Bytes | IEEE 802.3 CRC-32 integrity checksum |
| `validBitsInLastByte` | `uint8_t` | 1 Byte | Number of valid payload bits in the final byte (`0` to `8`) |
| `filenameLength` | `uint16_t` | 2 Bytes | Length of stored original filename string |
| `symbolCount` | `uint16_t` | 2 Bytes | Number of distinct non-zero symbols in frequency table |

---

## 7. Time and Space Complexity

| Operation | Time Complexity | Auxiliary Space | Description |
| :--- | :---: | :---: | :--- |
| **Byte Frequency Counting** | $O(n)$ | $O(1)$ | Single streaming pass over $n$ bytes; fixed 256-counter array. |
| **Min-Heap Construction** | $O(k \log k)$ | $O(k)$ | Inserting $k$ unique symbols into min-heap ($k \le 256$). |
| **Tree Merging** | $O(k \log k)$ | $O(k)$ | $k - 1$ greedy heap extractions and parent insertions. |
| **Code Generation** | $O(k)$ | $O(k)$ | Recursive DFS traversal of $2k - 1$ binary tree nodes. |
| **Bitstream Encoding** | $O(n)$ | $O(1)$ buffer | Translating source bytes to bits with 64KB I/O buffering. |
| **Bitstream Decoding** | $O(b)$ | $O(1)$ buffer | Traversing tree edges for $b$ encoded bits ($b \le 8n$). |

---

## 8. Quick Start Instructions

### Mode A: Launching ByteShrink Studio (Recommended)

1. Open PowerShell or Terminal in the `dsa` folder:
```powershell
npm start
# Or alternatively:
node server.js
```

2. Open your browser to:
```
http://localhost:3000
```

3. You will be greeted by the **Welcome Screen** onboarding page. Click **⚡ GET STARTED** to open the Dashboard!
4. **Compress Files**: Switch to the **Compress** tab, drag and drop any file (or browse), verify the detected file type signature, and click **START COMPRESSION**. Once complete, click **DOWNLOAD .HFX ARCHIVE** or click **DECOMPRESS THIS ARCHIVE** to test decompression immediately!
5. **Decompress Files**: Switch to the **Decompress** tab, drop your `.hfx` file, inspect the archive header, and click **START DECOMPRESSION** to restore the original file with verified CRC-32 integrity.
6. **Inspect Tree**: Switch to the **Tree Visualizer** tab to pan, zoom, and inspect the Huffman binary tree, symbol codes, and priority queue merges.

---

### Mode B: Building & Running the Native C++ Engine

```powershell
# 1. Configure build directory with CMake
& "C:\Program Files\CMake\bin\cmake.exe" -B build -G "MinGW Makefiles"

# 2. Compile binaries
& "C:\Program Files\CMake\bin\cmake.exe" --build build -j 4
```

This compiles:
- `build/ByteShrink.exe` (Main application CLI & Desktop GUI)
- `build/run_tests.exe` (Automated verification test runner)

#### Running the Native Executable:
```powershell
# Interactive Desktop GUI (Dear ImGui)
.\build\ByteShrink.exe

# CLI: Compress a document to .hfx archive
.\build\ByteShrink.exe -c samples/sample_document.txt samples/sample_document.txt.hfx

# CLI: Decompress a .hfx archive to original file
.\build\ByteShrink.exe -d samples/sample_document.txt.hfx samples/restored_doc.txt

# CLI: Inspect archive metadata and header fields
.\build\ByteShrink.exe -i samples/sample_document.txt.hfx

# CLI: Display Help
.\build\ByteShrink.exe --help
```

---

## 9. Automated Test Results (100% Pass Rate)

Execute the comprehensive automated test suite:
```powershell
.\build\run_tests.exe
```

Output:
```
========================================================
  HuffmanX / ByteShrink — Automated Test Suite
  Verification of DSA Core Engine & Edge Cases
========================================================

[PASS] 1. Empty File (0 Bytes)          (Orig: 0B -> Comp: 32B, Ratio: 0.00, Saved: 0.0%)
[PASS] 2. Single Character File         (Orig: 1B -> Comp: 48B, Ratio: 0.02, Saved: -4700.0%)
[PASS] 3. Repeated-Character (5KB 'A')  (Orig: 5000B -> Comp: 669B, Ratio: 7.47, Saved: 86.6%)
[PASS] 4. Ordinary Text File            (Orig: 606B -> Comp: 753B, Ratio: 0.80, Saved: -24.3%)
[PASS] 5. Unicode UTF-8 Multilingual    (Orig: 196B -> Comp: 810B, Ratio: 0.24, Saved: -313.3%)
[PASS] 6. All 256 Byte Values (4KB)     (Orig: 4096B -> Comp: 6440B, Ratio: 0.64, Saved: -57.2%)
[PASS] 7. High-Entropy Random Binary    (Orig: 32768B -> Comp: 35105B, Ratio: 0.93, Saved: -7.1%)
[PASS] 8. PDF Document Format           (Orig: 329B -> Comp: 683B, Ratio: 0.48, Saved: -107.6%)
[PASS] 9. DOCX (ZIP Container) Format   (Orig: 512B -> Comp: 174B, Ratio: 2.94, Saved: 66.0%)
[PASS] 10. JPEG Image Format            (Orig: 1024B -> Comp: 266B, Ratio: 3.85, Saved: 74.0%)
[PASS] 11. PNG Image Format             (Orig: 1024B -> Comp: 236B, Ratio: 4.34, Saved: 77.0%)
[PASS] 12. MP4 Video Format             (Orig: 2048B -> Comp: 393B, Ratio: 5.21, Saved: 80.8%)
[PASS] 13. MP3 Audio Format             (Orig: 1536B -> Comp: 282B, Ratio: 5.45, Saved: 81.6%)
[PASS] 14. Large 1MB File Streaming     (Orig: 1048576B -> Comp: 787045B, Ratio: 1.33, Saved: 24.9%)
[PASS] 15. Rejection of Invalid Magic   (Invalid magic number. This file was not created by HuffmanX.)
[PASS] 16. Rejection of Truncated Archive (File is too small to be a valid HuffmanX archive (truncated header).)
[PASS] 17. Detection of Checksum Mismatch (Integrity check failed: Checksum mismatch!)
[PASS] 18. Graceful Non-Existent File Handling (Input file does not exist)

========================================================
  TEST EXECUTION SUMMARY
========================================================
Total Tests: 18 | Passed: 18 | Failed: 0
ALL 18 AUTOMATED UNIT & ROUNDTRIP TESTS PASSED WITH 100% SUCCESS!
```

---

## 10. Honest Compression Behavior & Shannon Entropy

ByteShrink is an authentic, educational general-purpose byte-level entropy encoder. It **does not fabricate fake compression ratios**.

- **Files that compress well:** Uncompressed text, repetitive logs, raw audio/bitmaps, and source code exhibit high symbol bias (low entropy), achieving 40% to 85% reduction.
- **Files that may enlarge:** Pre-compressed media files (such as JPEG, MP4, MP3) and ZIP containers (DOCX) already possess near-maximum Shannon entropy (near uniform distribution across all 256 bytes). Compressing them losslessly cannot reduce payload bits, and the 23-byte header plus frequency table results in a slightly larger file. ByteShrink explicitly identifies and educates users about this honest physical reality.

---

## 11. Viva Voce & Faculty Presentation Guide

When presenting this project to your college faculty or laboratory examiners, refer directly to these files and concepts:

### Key Questions & Answers for Viva:

1. **Which folder contains the actual C++ Huffman Algorithm?**
   - The core algorithms are in `src/` and `include/`.
   - `src/HuffmanEncoder.cpp`: Pass 1 frequency counting & Pass 2 bitstream encoding.
   - `src/HuffmanDecoder.cpp`: Tree reconstruction & bit-by-bit bitstream decoding.
   - `src/HuffmanTree.cpp`: Greedy tree construction & DFS code table generation.
   - `include/MinHeap.h`: Custom array-based binary min-heap priority queue.
   - `src/BitWriter.cpp` & `src/BitReader.cpp`: Byte-boundary bit packing/unpacking.

2. **Why is Huffman Coding called a "Greedy Algorithm"?**
   - At each step, it locally extracts the two lowest-frequency subtrees from the priority queue and combines them into a single parent node without backtracking. This local optimal choice provably yields the global optimal prefix-free tree.

3. **What is the Time Complexity of Tree Construction?**
   - For an alphabet of $k$ distinct symbols ($k \le 256$):
     - Building initial heap: $O(k)$ using bottom-up heapify (or $O(k \log k)$ using successive insertions).
     - Merging subtrees: $(k - 1)$ iterations, each doing two `extractMin` and one `insert` ($O(\log k)$ each).
     - Total time: $O(k \log k)$. Since $k \le 256$, tree construction takes under a few microseconds.

4. **Why did we write `using namespace std;` in all `.cpp` files?**
   - To adhere to clean, student-friendly academic C++ programming standards, making variables like `string`, `vector`, `cout`, `cin`, `ifstream`, and `ofstream` direct and easy to read during laboratory demonstrations without cluttering code with `std::`.

5. **How does the decompressor know when to stop reading bits?**
   - The 23-byte header stores two crucial fields: `originalSize` (total uncompressed bytes) and `validBitsInLastByte` (how many bits in the last byte belong to the bitstream). The decoder stops as soon as `originalSize` bytes are decoded, ignoring any padding bits in the final byte.

---

## 12. Documentation Links

- [System Architecture](docs/architecture.md)
- [Binary Archive Format Specification](docs/compression_format.md)
- [DSA Theory, Complexity & Viva Voce Q&A Guide](docs/algorithm_explanation.md)
