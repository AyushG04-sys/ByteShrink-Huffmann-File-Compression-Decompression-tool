#ifndef HUFFMAN_TREE_H
#define HUFFMAN_TREE_H

#include "HuffmanNode.h"
#include "MinHeap.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

/**
 * @brief Represents a single greedy step in the Huffman tree construction.
 * Used by the Algorithm Explorer to present the step-by-step min-heap merges.
 */
struct MergeStep {
    size_t stepNumber;
    std::string leftLabel;
    uint64_t leftFreq;
    std::string rightLabel;
    uint64_t rightFreq;
    uint64_t combinedFreq;
};

/**
 * @brief Visual representation of a tree node for the GUI tree visualizer.
 */
struct VisualNode {
    int id;
    float x;
    float y;
    float radius;
    std::string label;
    std::string code;
    bool isLeaf;
    uint8_t byteValue;
    uint64_t frequency;
    int leftId;
    int rightId;
};

/**
 * @brief HuffmanTree encapsulates the construction, code generation,
 * bitstream traversal, and visualization of the Huffman binary tree.
 */
class HuffmanTree {
private:
    HuffmanNode* root;
    size_t distinctSymbolCount;
    std::unordered_map<uint8_t, std::string> codeMap;
    std::string codeTable[256];
    std::vector<MergeStep> mergeHistory;

    void generateCodesRecursive(HuffmanNode* node, const std::string& currentCode);
    void buildVisualNodesRecursive(HuffmanNode* node, int& nextId, int parentId, bool isLeft,
                                   float x, float y, float xOffset, float ySpacing,
                                   std::vector<VisualNode>& outNodes) const;

public:
    HuffmanTree();
    ~HuffmanTree();

    HuffmanTree(const HuffmanTree&) = delete;
    HuffmanTree& operator=(const HuffmanTree&) = delete;

    /**
     * @brief Constructs the Huffman Tree from a 256-element byte frequency array.
     * Uses the Greedy Algorithm with the custom MinHeap.
     * @param frequencies Array of 256 uint64_t frequencies.
     * @return true if tree construction succeeded.
     */
    bool buildFromFrequencies(const std::vector<uint64_t>& frequencies);

    /**
     * @brief Traverses the tree to generate variable-length prefix-free codes.
     */
    void generateCodes();

    /**
     * @brief Get root node of the tree.
     */
    HuffmanNode* getRoot() const { return root; }

    /**
     * @brief Returns code table for fast O(1) lookup during encoding.
     */
    const std::string* getCodeTable() const { return codeTable; }

    /**
     * @brief Returns map of byte -> Huffman code.
     */
    const std::unordered_map<uint8_t, std::string>& getCodeMap() const { return codeMap; }

    /**
     * @brief Returns step-by-step history of greedy merges for Algorithm Explorer.
     */
    const std::vector<MergeStep>& getMergeHistory() const { return mergeHistory; }

    /**
     * @brief Number of distinct symbols in the tree.
     */
    size_t getDistinctSymbolCount() const { return distinctSymbolCount; }

    /**
     * @brief Checks if tree is empty.
     */
    bool isEmpty() const { return root == nullptr; }

    /**
     * @brief Generates 2D layout coordinates for rendering in the GUI visualizer.
     * @param startX Initial root X position.
     * @param startY Initial root Y position.
     * @param initialSpacing Initial horizontal spread between subtrees.
     * @param levelSpacing Vertical distance between levels.
     * @return std::vector<VisualNode> Flat list of nodes with calculated coordinates.
     */
    std::vector<VisualNode> generateVisualLayout(float startX, float startY,
                                                float initialSpacing = 300.0f,
                                                float levelSpacing = 80.0f) const;

    /**
     * @brief Frees current tree resources.
     */
    void clear();
};

#endif // HUFFMAN_TREE_H
