# Huffman Coding — DSA Theory, Mathematical Foundations & Viva Voce Guide

This document provides a thorough theoretical reference for the **ByteShrink** project, prepared specifically for college computer science/engineering Data Structures and Algorithms evaluations, project presentations, and viva voce examinations.

---

## 1. What is Huffman Coding?

**Huffman Coding** is a lossless data compression algorithm developed by David A. Huffman in 1952. It assigns **variable-length binary codes** to input symbols based on their frequencies of occurrence:
- Frequently occurring symbols receive **shorter** binary codes (e.g. 2–3 bits).
- Infrequently occurring symbols receive **longer** binary codes (e.g. 10–12 bits).

By replacing standard 8-bit fixed-length bytes with optimal variable-length codes, the overall size of the bitstream is minimized:
$$\text{Average Code Length } L = \sum_{i=1}^{k} p_i \cdot \text{length}(c_i)$$
where $p_i$ is the relative frequency (probability) of symbol $i$, and $\text{length}(c_i)$ is the number of bits in its binary code.

---

## 2. Theoretical Foundations

### 2.1 Fixed-Length vs Variable-Length Codes
In standard ASCII / raw binary storage, every symbol occupies exactly 8 bits ($2^8 = 256$ possible values), regardless of whether it appears 10,000 times or 1 time.
In variable-length coding, symbol lengths vary. However, without special rules, an ambiguous stream like `010` could mean symbol `A` (`0`), `B` (`10`), or `C` (`01`), `D` (`0`).

### 2.2 The Prefix-Free Property
Huffman codes are strictly **prefix-free**:
> **Definition:** No valid code word is a prefix of any other valid code word.

Because of this property:
- **No delimiter or separator** (such as spaces or commas) is needed between encoded symbols in the bitstream.
- Decompression is uniquely decipherable in a single left-to-right pass.
- Each bit sequence corresponds to exactly one path from the tree's root to a unique leaf node.

---

## 3. The Greedy Algorithm

Huffman's algorithm is a classic example of a **Greedy Algorithm**. At every step, it makes the locally optimal choice in the hope of finding a global optimum.

### 3.1 The Greedy Choice Property
At each step, combine the two available subtrees that have the **lowest frequencies**:
1. Form a new internal node whose frequency is the sum of their frequencies:
   $$f(\text{parent}) = f(\text{left}) + f(\text{right})$$
2. Make the two extracted nodes the left and right children of this parent.
3. Re-insert the parent node back into the collection of candidates.
4. Repeat until only one candidate remains: the **Root** of the Huffman Tree.

### 3.2 Optimal Substructure
An optimal tree for an alphabet of size $k$ contains within it the optimal solution for the sub-problem of size $k - 1$ obtained by replacing the two minimum symbols with their combined parent. This mathematical property guarantees that the greedy choice yields a provably optimal prefix-free tree.

---

## 4. Priority Queue / Min-Heap Mechanics

To find the two minimum nodes repeatedly, a naive scan of an array takes $O(k)$ per step, leading to $O(k^2)$ total time.
By utilizing a **Min-Heap (Priority Queue)**, each extraction and insertion takes $O(\log k)$, reducing total tree construction time to **$O(k \log k)$**.

### 4.1 Complete Binary Heap Array Formulas (0-Indexed)
For any element at index $i$ in the heap array:
- **Parent Index:** $\lfloor(i - 1) / 2\rfloor$
- **Left Child Index:** $2i + 1$
- **Right Child Index:** $2i + 2$

### 4.2 Sift-Up (`insert`)
When a new node is pushed at the end of the array, it bubbles up by comparing itself with its parent until the min-heap invariant ($heap[parent] \le heap[child]$) is satisfied:
$$\text{Time Complexity: } O(\log k)$$

### 4.3 Sift-Down (`extractMin`)
The root (minimum element) is removed and replaced by the last element in the array. It is then sifted down by swapping with the smaller of its two children until the heap property is restored:
$$\text{Time Complexity: } O(\log k)$$

---

## 5. Step-by-Step Concrete Example

Suppose the input stream consists of the text: `"ABRACADABRA"`

### Step 1: Frequency Counting ($n = 11$ bytes)
| Symbol | Frequency |
| :---: | :---: |
| `'A'` | 5 |
| `'B'` | 2 |
| `'R'` | 2 |
| `'C'` | 1 |
| `'D'` | 1 |

### Step 2: Min-Heap Initialization
Candidates in Min-Heap: `[('C', 1), ('D', 1), ('B', 2), ('R', 2), ('A', 5)]`

### Step 3: Greedy Merging
1. **Merge 1:** Extract `'C'` (1) and `'D'` (1). Create parent $N_1$ with frequency $1 + 1 = 2$.
   - Heap: `[('B', 2), ('R', 2), (N1, 2), ('A', 5)]`
2. **Merge 2:** Extract `'B'` (2) and `'R'` (2). Create parent $N_2$ with frequency $2 + 2 = 4$.
   - Heap: `[(N1, 2), ('A', 5), (N2, 4)]`
3. **Merge 3:** Extract $N_1$ (2) and $N_2$ (4). Wait, smaller is $N_1$ (2) and `'A'` (5) or $N_2$ (4):
   - Smallest two: $N_1$ (2) and $N_2$ (4). Create parent $N_3$ with frequency $2 + 4 = 6$.
   - Heap: `[('A', 5), (N3, 6)]`
4. **Merge 4:** Extract `'A'` (5) and $N_3$ (6). Create Root with frequency $5 + 6 = 11$.

### Step 4: Generated Prefix Codes
- `'A'` $\rightarrow$ `0` (1 bit!)
- `'B'` $\rightarrow$ `110` (3 bits)
- `'R'` $\rightarrow$ `111` (3 bits)
- `'C'` $\rightarrow$ `100` (3 bits)
- `'D'` $\rightarrow$ `101` (3 bits)

### Step 5: Bit Calculation
- Original 8-bit ASCII size: $11 \times 8 = 88$ bits.
- Huffman Encoded size:
  - 5 `'A'`s $\times 1 = 5$ bits
  - 2 `'B'`s $\times 3 = 6$ bits
  - 2 `'R'`s $\times 3 = 6$ bits
  - 1 `'C'` $\times 3 = 3$ bits
  - 1 `'D'` $\times 3 = 3$ bits
  - **Total:** $5 + 6 + 6 + 3 + 3 = 23$ bits!
- **Space Saved in bitstream:** $\frac{88 - 23}{88} \times 100\% = 73.8\%$ reduction!

---

## 6. Time and Space Complexity Analysis

| Phase | Time Complexity | Auxiliary Space Complexity | Explanation |
| :--- | :---: | :---: | :--- |
| **1. Frequency Counting** | $O(n)$ | $O(1)$ | Single streaming pass over $n$ bytes; fixed 256-element frequency table. |
| **2. Min-Heap Construction** | $O(k \log k)$ | $O(k)$ | Inserting $k$ unique symbols into min-heap ($k \le 256$). |
| **3. Greedy Tree Merges** | $O(k \log k)$ | $O(k)$ | Exactly $k - 1$ merges, each performing two `extractMin` and one `insert` in $O(\log k)$ time. |
| **4. Code Generation** | $O(k)$ | $O(k)$ | Single recursive DFS traversal visiting each node of the $2k - 1$ node binary tree once. |
| **5. Bitstream Encoding** | $O(n)$ | $O(1)$ buffer | Translating each of the $n$ bytes via $O(1)$ array lookup into `BitWriter`. |
| **6. Bitstream Decoding** | $O(b)$ | $O(1)$ buffer | Traversing tree edges for each of the $b$ encoded bits ($b \le 8n$). |

*Here, $n$ is total input file bytes, $k$ is number of distinct byte values ($k \le 256$), and $b$ is the number of bits in the compressed payload.*

---

## 7. Viva Voce Questions & Answers

### Q1: Why is Huffman Coding called a "Greedy Algorithm"?
**Answer:** It makes the locally optimal choice at every step—namely, pairing together the two currently lowest-frequency subtrees—without ever backtracking or reconsidering previous merges. This local greedy choice provably constructs the globally optimal prefix code tree.

### Q2: What is the significance of the "Prefix-Free" property?
**Answer:** In a prefix-free code, no code word is a prefix of any other code word. This ensures unique decipherability: as we read bits sequentially from the compressed file, there is only one possible symbol match at any point, eliminating the need for delimiters between characters.

### Q3: How does the Min-Heap improve tree construction efficiency?
**Answer:** Building the tree requires finding the two minimum-frequency nodes $k - 1$ times. In an unsorted array or linked list, finding the minimum takes $O(k)$ time, leading to $O(k^2)$ overall. A Min-Heap extracts the minimum and inserts the combined parent in $O(\log k)$ time, reducing overall tree construction to $O(k \log k)$.

### Q4: How do you handle tie-breaking when two nodes have identical frequencies?
**Answer:** In ByteShrink, deterministic tie-breaking is enforced by comparing the minimum byte value contained within their respective subtrees (`minByteInSubtree`). This guarantees that tree construction is 100% deterministic, producing identical bitstreams across different platforms, compilers, and runs.

### Q5: Why do some files (like JPEG, MP4, MP3, or DOCX) become larger after compression?
**Answer:** JPEG images, MP4 videos, MP3 audio, and DOCX files (which are zipped XML archives) are already heavily compressed using algorithms like DCT, MPEG, or Deflate. Their byte frequency distribution is nearly uniform (high Shannon entropy, close to 8 bits/symbol). Compressing them with Huffman Coding cannot reduce the payload further, and adding the 23-byte archive header and frequency table results in a slightly larger file. ByteShrink reports this honestly.

### Q6: How is the compressed bitstream stored on disk? Are bits stored as text `'0'` and `'1'`?
**Answer:** No. Storing bits as ASCII `'0'` and `'1'` would use 8 bits per character, inflating the file by 800%! ByteShrink uses a custom `BitWriter` that shifts and packs individual bits into raw binary bytes (MSB first) and writes them in 64KB blocks to binary streams.

### Q7: How do you handle the final byte if the total number of bits is not a multiple of 8?
**Answer:** The remaining bits are shifted into the most significant bit positions of the last byte, with the remaining lower bits padded with zeros. The exact number of valid bits in this final byte (1 to 8) is stored in the `validBitsInLastByte` field of the archive header. The decoder reads only that many bits from the final byte.

### Q8: How does ByteShrink handle special cases like an empty file or a file with only one unique symbol?
**Answer:**
- **Empty file (0 bytes):** The header records `originalSize = 0` and `symbolCount = 0`. No payload is written. On decompression, an empty 0-byte file is created immediately.
- **Single-symbol file (e.g. 1000 `'A'`s):** The tree contains only 1 leaf node. A single code `'0'` is assigned, or the decoder emits the single symbol `originalSize` times directly.

### Q9: How is data integrity guaranteed during decompression?
**Answer:** During compression, the IEEE 802.3 CRC-32 checksum of the original file is computed and stored in the header. During decompression, the CRC-32 of the restored bytes is calculated on the fly. If the decoded checksum does not match the header checksum, decompression fails with an explicit corruption error.

### Q10: How does ByteShrink prevent security vulnerabilities like Path Traversal attacks?
**Answer:** The `ArchiveFormat::sanitizeFilename` function strips directory components (`/`, `\`, `:`), path traversal tokens (`..`), and illegal characters from stored filenames. Restored files are strictly written to the user-specified output folder.
