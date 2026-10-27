#include "HuffmanTree.h"
#include <algorithm>
#include <iostream>

using namespace std;

HuffmanTree::HuffmanTree()
    : root(nullptr), distinctSymbolCount(0) {
    for (int i = 0; i < 256; ++i) {
        codeTable[i] = "";
    }
}

HuffmanTree::~HuffmanTree() {
    clear();
}

void HuffmanTree::clear() {
    delete root;
    root = nullptr;
    distinctSymbolCount = 0;
    codeMap.clear();
    mergeHistory.clear();
    for (int i = 0; i < 256; ++i) {
        codeTable[i] = "";
    }
}

bool HuffmanTree::buildFromFrequencies(const vector<uint64_t>& frequencies) {
    clear();

    if (frequencies.size() < 256) {
        return false;
    }

    MinHeap<HuffmanNode*, HuffmanNodeLess> minHeap;

    // Phase 1: Create leaf nodes for all non-zero frequency symbols
    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] > 0) {
            HuffmanNode* leaf = new HuffmanNode(static_cast<uint8_t>(i), frequencies[i]);
            minHeap.insert(leaf);
            distinctSymbolCount++;
        }
    }

    // Special Case 1: Empty file (0 symbols)
    if (minHeap.empty()) {
        root = nullptr;
        return true;
    }

    // Special Case 2: Exactly 1 distinct symbol
    if (minHeap.size() == 1) {
        root = minHeap.extractMin();
        generateCodes();
        return true;
    }

    // Phase 2: Greedy merging of two minimum frequency subtrees
    size_t step = 1;
    while (minHeap.size() > 1) {
        HuffmanNode* left = minHeap.extractMin();
        HuffmanNode* right = minHeap.extractMin();

        // Record merge step for viva / Algorithm Explorer
        MergeStep mStep;
        mStep.stepNumber = step++;
        mStep.leftLabel = left->getDisplayLabel();
        mStep.leftFreq = left->frequency;
        mStep.rightLabel = right->getDisplayLabel();
        mStep.rightFreq = right->frequency;
        mStep.combinedFreq = left->frequency + right->frequency;
        mergeHistory.push_back(mStep);

        // Combine into parent node
        HuffmanNode* parent = new HuffmanNode(left, right);
        minHeap.insert(parent);
    }

    // Final remaining node is the root of the Huffman Tree
    root = minHeap.extractMin();

    // Phase 3: Traverse tree to generate prefix-free variable length binary codes
    generateCodes();
    return true;
}

void HuffmanTree::generateCodes() {
    codeMap.clear();
    for (int i = 0; i < 256; ++i) {
        codeTable[i] = "";
    }

    if (!root) {
        return;
    }

    // If only one symbol exists, assign code "0"
    if (root->isLeaf()) {
        codeMap[root->byteValue] = "0";
        codeTable[root->byteValue] = "0";
        return;
    }

    generateCodesRecursive(root, "");
}

void HuffmanTree::generateCodesRecursive(HuffmanNode* node, const string& currentCode) {
    if (!node) return;

    if (node->isLeaf()) {
        codeMap[node->byteValue] = currentCode;
        codeTable[node->byteValue] = currentCode;
        return;
    }

    // Left edge represents bit '0'
    generateCodesRecursive(node->left, currentCode + "0");

    // Right edge represents bit '1'
    generateCodesRecursive(node->right, currentCode + "1");
}

void HuffmanTree::buildVisualNodesRecursive(HuffmanNode* node, int& nextId, int parentId, bool isLeft,
                                            float x, float y, float xOffset, float ySpacing,
                                            vector<VisualNode>& outNodes) const {
    if (!node) return;

    int currentId = nextId++;
    VisualNode vNode;
    vNode.id = currentId;
    vNode.x = x;
    vNode.y = y;
    vNode.radius = node->isLeaf() ? 24.0f : 20.0f;
    vNode.label = node->getDisplayLabel();
    vNode.isLeaf = node->isLeaf();
    vNode.byteValue = node->byteValue;
    vNode.frequency = node->frequency;
    vNode.leftId = -1;
    vNode.rightId = -1;

    if (node->isLeaf()) {
        auto it = codeMap.find(node->byteValue);
        vNode.code = (it != codeMap.end()) ? it->second : "";
    }

    // Reserve index and append
    outNodes.push_back(vNode);
    size_t currentIndex = outNodes.size() - 1;

    float nextOffset = max(xOffset * 0.52f, 28.0f);

    if (node->left) {
        int leftStartId = nextId;
        buildVisualNodesRecursive(node->left, nextId, currentId, true,
                                  x - xOffset, y + ySpacing, nextOffset, ySpacing, outNodes);
        outNodes[currentIndex].leftId = leftStartId;
    }

    if (node->right) {
        int rightStartId = nextId;
        buildVisualNodesRecursive(node->right, nextId, currentId, false,
                                  x + xOffset, y + ySpacing, nextOffset, ySpacing, outNodes);
        outNodes[currentIndex].rightId = rightStartId;
    }
}

vector<VisualNode> HuffmanTree::generateVisualLayout(float startX, float startY,
                                                         float initialSpacing,
                                                         float levelSpacing) const {
    vector<VisualNode> result;
    if (!root) return result;

    int nextId = 0;
    buildVisualNodesRecursive(root, nextId, -1, false, startX, startY, initialSpacing, levelSpacing, result);
    return result;
}
