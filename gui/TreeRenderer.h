#ifndef TREE_RENDERER_H
#define TREE_RENDERER_H

#include "../include/HuffmanTree.h"
#include "../third_party/imgui/imgui.h"
#include <vector>

class TreeRenderer {
private:
    float panX;
    float panY;
    float zoom;
    int hoveredNodeId;

public:
    TreeRenderer();

    /**
     * @brief Resets pan and zoom to center the tree in the view.
     */
    void resetView(float canvasWidth, float canvasHeight);

    /**
     * @brief Renders the interactive visual Huffman Tree canvas.
     * @param visualNodes Flat list of nodes with layout coordinates.
     * @param totalFileBytes Total bytes in original file for calculating percentages.
     * @param canvasSize Dimensions of the drawing area.
     */
    void render(const std::vector<VisualNode>& visualNodes,
                uint64_t totalFileBytes,
                const ImVec2& canvasSize);
};

#endif // TREE_RENDERER_H
