#ifndef APP_WINDOW_H
#define APP_WINDOW_H

#include "../include/HuffmanEncoder.h"
#include "../include/HuffmanDecoder.h"
#include "TreeRenderer.h"
#include "Theme.h"
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#include <d3d11.h>
#endif

enum class AppTab {
    Dashboard,
    Compress,
    Decompress,
    AlgorithmExplorer,
    SettingsHelp
};

struct ActivityLogEntry {
    std::string timestamp;
    std::string actionType;   // "Compressed" or "Decompressed"
    std::string filename;
    uint64_t originalSize;
    uint64_t resultSize;
    std::string summary;
    bool success;
};

class AppWindow {
private:
    // Window & Direct3D handles
#ifdef _WIN32
    HWND hWnd;
    ID3D11Device* d3dDevice;
    ID3D11DeviceContext* d3dDeviceContext;
    IDXGISwapChain* swapChain;
    ID3D11RenderTargetView* mainRenderTargetView;
#endif

    bool isRunning;
    AppTab currentTab;
    ThemeMode currentTheme;

    // Compression View State
    char compressInputPath[1024];
    char compressOutputDir[1024];
    char compressOutputName[256];
    bool isCompressing;
    float compressProgress;
    char compressStage[256];
    std::atomic<bool> cancelCompressFlag;
    CompressionResult lastCompressResult;
    bool hasCompressResult;

    // Decompression View State
    char decompressArchivePath[1024];
    char decompressOutputDir[1024];
    char decompressOutputName[256];
    bool isDecompressing;
    float decompressProgress;
    char decompressStage[256];
    std::atomic<bool> cancelDecompressFlag;
    DecompressionResult lastDecompressResult;
    bool hasDecompressResult;
    ArchiveMetadata inspectedArchiveMeta;
    bool hasInspectedArchive;

    // Algorithm Explorer State
    HuffmanTree explorerTree;
    std::vector<uint64_t> explorerFrequencies;
    std::vector<VisualNode> explorerVisualNodes;
    uint64_t explorerTotalBytes;
    char explorerCustomText[4096];
    TreeRenderer treeRenderer;
    int explorerSubTab; // 0 = Tree Visualizer, 1 = Symbol Table, 2 = Merge Timeline

    // Activity Log
    std::vector<ActivityLogEntry> recentActivities;

    // Threading
    std::thread workerThread;
    std::mutex stateMutex;

    // Helpers
    void renderHeader();
    void renderDashboardTab();
    void renderCompressTab();
    void renderDecompressTab();
    void renderAlgorithmExplorerTab();
    void renderSettingsHelpTab();

    void updateExplorerWithFrequencies(const std::vector<uint64_t>& freqs, uint64_t totalBytes);
    void updateExplorerWithText(const std::string& text);

public:
    AppWindow();
    ~AppWindow();

    bool init();
    void run();
    void cleanup();

    void onFileDropped(const std::string& filePath);

#ifdef _WIN32
    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
};

#endif // APP_WINDOW_H
