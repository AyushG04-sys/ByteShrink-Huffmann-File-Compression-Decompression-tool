#ifndef HUFFMAN_NODE_H
#define HUFFMAN_NODE_H

#include <cstdint>
#include <memory>
#include <string>

/**
 * @brief HuffmanNode represents a node in the binary Huffman Tree.
 *
 * Each node can either be:
 * 1. A Leaf Node: Contains a concrete byte symbol (0-255) and its occurrence frequency.
 * 2. An Internal Node: Created during the greedy merging phase by combining two
 *    subtrees. Its frequency is the sum of its children's frequencies.
 *
 * For college viva / DSA evaluation:
 * - Deterministic tie-breaking is guaranteed via minByteInSubtree so that trees
 *   generated on different platforms or runs are identical bit-for-bit.
 */
class HuffmanNode {
public:
    uint8_t byteValue;            ///< The byte symbol (valid for leaf nodes)
    uint64_t frequency;           ///< Total frequency of this symbol or subtree
    HuffmanNode* left;            ///< Pointer to left child (represents binary bit '0')
    HuffmanNode* right;           ///< Pointer to right child (represents binary bit '1')
    uint16_t minByteInSubtree;    ///< Minimum byte value in this subtree for tie-breaking

    /**
     * @brief Leaf Node constructor.
     * @param val Byte value (0-255).
     * @param freq Occurrence frequency.
     */
    HuffmanNode(uint8_t val, uint64_t freq);

    /**
     * @brief Internal Node constructor.
     * Combines two existing nodes; frequency is the sum of children.
     * @param l Pointer to left child node.
     * @param r Pointer to right child node.
     */
    HuffmanNode(HuffmanNode* l, HuffmanNode* r);

    /**
     * @brief Recursive destructor to safely clean up entire subtree.
     */
    ~HuffmanNode();

    // Disable copy semantics to prevent double-free
    HuffmanNode(const HuffmanNode&) = delete;
    HuffmanNode& operator=(const HuffmanNode&) = delete;

    /**
     * @brief Checks if this node is a leaf (no children).
     * @return true if both left and right pointers are null.
     */
    bool isLeaf() const;

    /**
     * @brief Human-readable label for debugging and Algorithm Explorer.
     * @return Formatted string representation (e.g. "'A' (0x41) freq=120").
     */
    std::string getDisplayLabel() const;
};

/**
 * @brief Min-Heap comparator for HuffmanNode pointers.
 *
 * Defines the Greedy Priority:
 * Lower frequency -> Higher priority (extracted first from min-heap).
 * In case of identical frequencies, deterministic tie-breaking is performed
 * by comparing the minimum byte value contained in their subtrees.
 */
struct HuffmanNodeComparator {
    bool operator()(const HuffmanNode* a, const HuffmanNode* b) const {
        if (!a || !b) return a != nullptr;
        if (a->frequency != b->frequency) {
            return a->frequency > b->frequency; // Greater-than for std::priority_queue (min-heap behavior)
        }
        return a->minByteInSubtree > b->minByteInSubtree;
    }
};

/**
 * @brief Direct less-than comparator for custom MinHeap implementation.
 */
struct HuffmanNodeLess {
    bool operator()(const HuffmanNode* a, const HuffmanNode* b) const {
        if (!a || !b) return a != nullptr;
        if (a->frequency != b->frequency) {
            return a->frequency < b->frequency;
        }
        return a->minByteInSubtree < b->minByteInSubtree;
    }
};

#endif // HUFFMAN_NODE_H
