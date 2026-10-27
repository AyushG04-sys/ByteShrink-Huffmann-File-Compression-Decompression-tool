#include "TreeRenderer.h"
#include <cmath>
#include <iomanip>
#include <sstream>
#include <algorithm>

TreeRenderer::TreeRenderer()
    : panX(0.0f), panY(40.0f), zoom(1.0f), hoveredNodeId(-1) {}

void TreeRenderer::resetView(float canvasWidth, float canvasHeight) {
    panX = canvasWidth * 0.5f;
    panY = 50.0f;
    zoom = 1.0f;
}

void TreeRenderer::render(const std::vector<VisualNode>& visualNodes,
                          uint64_t totalFileBytes,
                          const ImVec2& canvasSize) {
    if (visualNodes.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                           "No Huffman tree available. Compress a file or enter sample text to visualize.");
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImVec2 canvasP0 = ImGui::GetCursorScreenPos();
    ImVec2 canvasP1 = ImVec2(canvasP0.x + canvasSize.x, canvasP0.y + canvasSize.y);

    // Canvas background
    drawList->AddRectFilled(canvasP0, canvasP1, IM_COL32(18, 20, 28, 255), 8.0f);
    drawList->AddRect(canvasP0, canvasP1, IM_COL32(45, 52, 70, 255), 8.0f);

    // Clip rendering inside canvas
    drawList->PushClipRect(canvasP0, canvasP1, true);

    // Mouse interaction for Panning & Zooming
    ImGui::InvisibleButton("tree_canvas_hitbox", canvasSize,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    bool isHovered = ImGui::IsItemHovered();
    bool isActive = ImGui::IsItemActive();

    // Drag to pan
    if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
        panX += io.MouseDelta.x;
        panY += io.MouseDelta.y;
    }

    // Scroll wheel to zoom
    if (isHovered && io.MouseWheel != 0.0f) {
        float oldZoom = zoom;
        zoom += io.MouseWheel * 0.10f;
        zoom = std::clamp(zoom, 0.25f, 2.50f);

        // Zoom centered on mouse cursor
        ImVec2 mousePos = io.MousePos;
        float mouseRelX = mousePos.x - canvasP0.x;
        float mouseRelY = mousePos.y - canvasP0.y;

        panX = mouseRelX - (mouseRelX - panX) * (zoom / oldZoom);
        panY = mouseRelY - (mouseRelY - panY) * (zoom / oldZoom);
    }

    // Reset view button overlay
    ImVec2 btnPos = ImVec2(canvasP1.x - 110, canvasP0.y + 12);
    ImGui::SetCursorScreenPos(btnPos);
    if (ImGui::Button("Reset View", ImVec2(98, 26))) {
        resetView(canvasSize.x, canvasSize.y);
    }

    // Zoom indicator overlay
    ImVec2 zoomLabelPos = ImVec2(canvasP0.x + 14, canvasP0.y + 14);
    std::string zoomText = "Zoom: " + std::to_string(static_cast<int>(zoom * 100)) + "% | Pan: Drag mouse";
    drawList->AddText(zoomLabelPos, IM_COL32(180, 190, 210, 200), zoomText.c_str());

    // Coordinate transform lambda
    auto toScreen = [&](float wx, float wy) -> ImVec2 {
        return ImVec2(canvasP0.x + panX + wx * zoom, canvasP0.y + panY + wy * zoom);
    };

    hoveredNodeId = -1;
    const VisualNode* hoveredNodePtr = nullptr;

    // Step 1: Draw Branch Lines connecting parent to children
    for (const auto& node : visualNodes) {
        ImVec2 parentPos = toScreen(node.x, node.y);

        // Draw Left Child Branch ('0')
        if (node.leftId >= 0 && node.leftId < static_cast<int>(visualNodes.size())) {
            const auto& leftChild = visualNodes[node.leftId];
            ImVec2 childPos = toScreen(leftChild.x, leftChild.y);

            drawList->AddLine(parentPos, childPos, IM_COL32(75, 120, 210, 200), 2.0f * zoom);

            // Edge label "0"
            ImVec2 midPos = ImVec2((parentPos.x * 0.45f + childPos.x * 0.55f) - 10.0f * zoom,
                                   (parentPos.y * 0.45f + childPos.y * 0.55f));
            drawList->AddCircleFilled(midPos, 9.0f * zoom, IM_COL32(30, 40, 60, 220));
            drawList->AddText(ImVec2(midPos.x - 4.0f * zoom, midPos.y - 7.0f * zoom),
                              IM_COL32(100, 200, 255, 255), "0");
        }

        // Draw Right Child Branch ('1')
        if (node.rightId >= 0 && node.rightId < static_cast<int>(visualNodes.size())) {
            const auto& rightChild = visualNodes[node.rightId];
            ImVec2 childPos = toScreen(rightChild.x, rightChild.y);

            drawList->AddLine(parentPos, childPos, IM_COL32(210, 100, 140, 200), 2.0f * zoom);

            // Edge label "1"
            ImVec2 midPos = ImVec2((parentPos.x * 0.45f + childPos.x * 0.55f) + 10.0f * zoom,
                                   (parentPos.y * 0.45f + childPos.y * 0.55f));
            drawList->AddCircleFilled(midPos, 9.0f * zoom, IM_COL32(60, 30, 45, 220));
            drawList->AddText(ImVec2(midPos.x - 4.0f * zoom, midPos.y - 7.0f * zoom),
                              IM_COL32(255, 130, 170, 255), "1");
        }
    }

    // Step 2: Draw Nodes (Circles and Text)
    for (const auto& node : visualNodes) {
        ImVec2 center = toScreen(node.x, node.y);
        float r = node.radius * zoom;

        // Check if mouse hovers over this node
        float dx = io.MousePos.x - center.x;
        float dy = io.MousePos.y - center.y;
        bool isNodeHovered = (dx * dx + dy * dy <= r * r) && isHovered;

        if (isNodeHovered) {
            hoveredNodeId = node.id;
            hoveredNodePtr = &node;
        }

        ImU32 fillCol;
        ImU32 borderCol;

        if (node.isLeaf) {
            fillCol = isNodeHovered ? IM_COL32(30, 180, 120, 255) : IM_COL32(20, 140, 95, 230);
            borderCol = isNodeHovered ? IM_COL32(140, 255, 200, 255) : IM_COL32(40, 200, 140, 255);
        } else {
            fillCol = isNodeHovered ? IM_COL32(45, 75, 135, 255) : IM_COL32(35, 55, 100, 230);
            borderCol = isNodeHovered ? IM_COL32(140, 180, 255, 255) : IM_COL32(70, 110, 190, 255);
        }

        // Circle body & border
        drawList->AddCircleFilled(center, r, fillCol, 32);
        drawList->AddCircle(center, r, borderCol, 32, isNodeHovered ? 3.0f : 1.5f);

        // Text label
        std::string displayStr;
        if (node.isLeaf) {
            displayStr = node.label;
        } else {
            displayStr = std::to_string(node.frequency);
        }

        ImVec2 textSize = ImGui::CalcTextSize(displayStr.c_str());
        ImVec2 textPos = ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f);
        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), displayStr.c_str());
    }

    drawList->PopClipRect();

    // Step 3: Draw Tooltip for Hovered Node
    if (hoveredNodePtr) {
        ImGui::BeginTooltip();
        if (hoveredNodePtr->isLeaf) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.5f, 1.0f), "Leaf Node: %s", hoveredNodePtr->label.c_str());
            ImGui::Separator();
            ImGui::Text("Byte Value:  0x%02X (%d)", hoveredNodePtr->byteValue, hoveredNodePtr->byteValue);
            ImGui::Text("Frequency:   %llu occurrences", static_cast<unsigned long long>(hoveredNodePtr->frequency));

            if (totalFileBytes > 0) {
                double pct = (static_cast<double>(hoveredNodePtr->frequency) / static_cast<double>(totalFileBytes)) * 100.0;
                ImGui::Text("Occurrence:  %.2f%% of file", pct);
            }

            ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "Huffman Code: %s", hoveredNodePtr->code.c_str());
            ImGui::Text("Code Length:  %zu bits (vs 8 bits uncompressed)", hoveredNodePtr->code.size());
        } else {
            ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Internal Tree Node");
            ImGui::Separator();
            ImGui::Text("Combined Frequency: %llu", static_cast<unsigned long long>(hoveredNodePtr->frequency));
            if (totalFileBytes > 0) {
                double pct = (static_cast<double>(hoveredNodePtr->frequency) / static_cast<double>(totalFileBytes)) * 100.0;
                ImGui::Text("Subtree Weight:     %.2f%% of file", pct);
            }
        }
        ImGui::EndTooltip();
    }
}
