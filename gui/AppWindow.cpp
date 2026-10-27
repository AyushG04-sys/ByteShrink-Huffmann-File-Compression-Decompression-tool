#include "AppWindow.h"
#include "NativeFileDialog.h"
#include "../include/FileUtils.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/backends/imgui_impl_win32.h"
#include "../third_party/imgui/backends/imgui_impl_dx11.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <ctime>
#include <cstring>

namespace fs = std::filesystem;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static AppWindow* g_AppWindowInstance = nullptr;

static std::string getCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
#ifdef _WIN32
    localtime_s(&tm_now, &now_c);
#else
    localtime_r(&now_c, &tm_now);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm_now);
    return std::string(buf);
}

AppWindow::AppWindow()
    : hWnd(NULL),
      d3dDevice(NULL),
      d3dDeviceContext(NULL),
      swapChain(NULL),
      mainRenderTargetView(NULL),
      isRunning(false),
      currentTab(AppTab::Dashboard),
      currentTheme(ThemeMode::Dark),
      isCompressing(false),
      compressProgress(0.0f),
      cancelCompressFlag(false),
      hasCompressResult(false),
      isDecompressing(false),
      decompressProgress(0.0f),
      cancelDecompressFlag(false),
      hasDecompressResult(false),
      hasInspectedArchive(false),
      explorerTotalBytes(0),
      explorerSubTab(0) {
    g_AppWindowInstance = this;

    std::memset(compressInputPath, 0, sizeof(compressInputPath));
    std::memset(compressOutputDir, 0, sizeof(compressOutputDir));
    std::memset(compressOutputName, 0, sizeof(compressOutputName));
    std::memset(compressStage, 0, sizeof(compressStage));

    std::memset(decompressArchivePath, 0, sizeof(decompressArchivePath));
    std::memset(decompressOutputDir, 0, sizeof(decompressOutputDir));
    std::memset(decompressOutputName, 0, sizeof(decompressOutputName));
    std::memset(decompressStage, 0, sizeof(decompressStage));

    std::string sample = "Huffman coding is a greedy algorithm for lossless data compression!";
    std::strncpy(explorerCustomText, sample.c_str(), sizeof(explorerCustomText) - 1);
    updateExplorerWithText(sample);
}

AppWindow::~AppWindow() {
    cleanup();
    g_AppWindowInstance = nullptr;
}

LRESULT WINAPI AppWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;
    }

    switch (msg) {
        case WM_SIZE:
            if (g_AppWindowInstance && g_AppWindowInstance->d3dDevice != NULL && wParam != SIZE_MINIMIZED) {
                if (g_AppWindowInstance->mainRenderTargetView) {
                    g_AppWindowInstance->mainRenderTargetView->Release();
                    g_AppWindowInstance->mainRenderTargetView = NULL;
                }
                g_AppWindowInstance->swapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                ID3D11Texture2D* pBackBuffer = NULL;
                g_AppWindowInstance->swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
                if (pBackBuffer) {
                    g_AppWindowInstance->d3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_AppWindowInstance->mainRenderTargetView);
                    pBackBuffer->Release();
                }
            }
            return 0;

        case WM_DROPFILES: {
            HDROP hDrop = (HDROP)wParam;
            wchar_t droppedPath[MAX_PATH];
            if (DragQueryFileW(hDrop, 0, droppedPath, MAX_PATH)) {
                int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, droppedPath, -1, NULL, 0, NULL, NULL);
                std::string path(sizeNeeded, 0);
                WideCharToMultiByte(CP_UTF8, 0, droppedPath, -1, &path[0], sizeNeeded, NULL, NULL);
                if (g_AppWindowInstance) {
                    g_AppWindowInstance->onFileDropped(path.c_str());
                }
            }
            DragFinish(hDrop);
            return 0;
        }

        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) {
                return 0; // Disable ALT application menu focus
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool AppWindow::init() {
    WNDCLASSEXW wc = {
        sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
        GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
        L"HuffmanXWindowClass", NULL
    };
    RegisterClassExW(&wc);

    hWnd = CreateWindowW(wc.lpszClassName,
                         L"HuffmanX — Smart File Compression Studio",
                         WS_OVERLAPPEDWINDOW,
                         100, 100, 1200, 800,
                         NULL, NULL, wc.hInstance, NULL);

    if (!hWnd) return false;

    // Enable drag and drop
    DragAcceptFiles(hWnd, TRUE);

    // Setup Direct3D 11
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT res = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION,
        &sd, &swapChain, &d3dDevice, &featureLevel, &d3dDeviceContext);

    if (res != S_OK) return false;

    ID3D11Texture2D* pBackBuffer = NULL;
    swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        d3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
        pBackBuffer->Release();
    }

    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Apply Theme
    Theme::applyTheme(currentTheme);

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(d3dDevice, d3dDeviceContext);

    isRunning = true;
    return true;
}

void AppWindow::cleanup() {
    if (workerThread.joinable()) {
        workerThread.join();
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (mainRenderTargetView) { mainRenderTargetView->Release(); mainRenderTargetView = NULL; }
    if (swapChain) { swapChain->Release(); swapChain = NULL; }
    if (d3dDeviceContext) { d3dDeviceContext->Release(); d3dDeviceContext = NULL; }
    if (d3dDevice) { d3dDevice->Release(); d3dDevice = NULL; }

    if (hWnd) {
        DestroyWindow(hWnd);
        UnregisterClassW(L"HuffmanXWindowClass", GetModuleHandle(NULL));
        hWnd = NULL;
    }
}

void AppWindow::onFileDropped(const std::string& filePath) {
    if (FileUtils::getFileExtension(filePath) == ".hfx") {
        currentTab = AppTab::Decompress;
        std::strncpy(decompressArchivePath, filePath.c_str(), sizeof(decompressArchivePath) - 1);
        HuffmanDecoder decoder;
        inspectedArchiveMeta = decoder.inspectArchive(filePath);
        hasInspectedArchive = inspectedArchiveMeta.isValid;
        if (hasInspectedArchive) {
            std::strncpy(decompressOutputName, inspectedArchiveMeta.originalFilename.c_str(), sizeof(decompressOutputName) - 1);
            std::string parentDir = FileUtils::getDirectoryPath(filePath);
            std::strncpy(decompressOutputDir, parentDir.c_str(), sizeof(decompressOutputDir) - 1);
        }
    } else {
        currentTab = AppTab::Compress;
        std::strncpy(compressInputPath, filePath.c_str(), sizeof(compressInputPath) - 1);
        std::string parentDir = FileUtils::getDirectoryPath(filePath);
        std::strncpy(compressOutputDir, parentDir.c_str(), sizeof(compressOutputDir) - 1);
        std::string base = FileUtils::getBaseFilename(filePath) + ".hfx";
        std::strncpy(compressOutputName, base.c_str(), sizeof(compressOutputName) - 1);
    }
}

void AppWindow::updateExplorerWithFrequencies(const std::vector<uint64_t>& freqs, uint64_t totalBytes) {
    explorerFrequencies = freqs;
    explorerTotalBytes = totalBytes;
    explorerTree.clear();
    explorerTree.buildFromFrequencies(freqs);
    explorerVisualNodes = explorerTree.generateVisualLayout(0.0f, 0.0f, 400.0f, 90.0f);
    treeRenderer.resetView(900.0f, 500.0f);
}

void AppWindow::updateExplorerWithText(const std::string& text) {
    std::vector<uint64_t> freqs(256, 0);
    for (unsigned char c : text) {
        freqs[c]++;
    }
    updateExplorerWithFrequencies(freqs, text.size());
}

void AppWindow::run() {
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    while (isRunning && msg.message != WM_QUIT) {
        if (PeekMessageW(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            continue;
        }

        // Start frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Fullscreen dock/window setup
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

        ImGui::Begin("MainWindow", nullptr, windowFlags);

        renderHeader();
        ImGui::Separator();

        switch (currentTab) {
            case AppTab::Dashboard:
                renderDashboardTab();
                break;
            case AppTab::Compress:
                renderCompressTab();
                break;
            case AppTab::Decompress:
                renderDecompressTab();
                break;
            case AppTab::AlgorithmExplorer:
                renderAlgorithmExplorerTab();
                break;
            case AppTab::SettingsHelp:
                renderSettingsHelpTab();
                break;
        }

        ImGui::End();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.08f, 0.09f, 0.12f, 1.0f };
        d3dDeviceContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
        d3dDeviceContext->ClearRenderTargetView(mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        swapChain->Present(1, 0); // VSync enabled
    }
}

void AppWindow::renderHeader() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(16, 8));

    // Logo & App Name
    ImGui::TextColored(Theme::getPrimaryColor(), "HuffmanX");
    ImGui::SameLine();
    ImGui::TextDisabled("— Smart File Compression Studio");

    ImGui::SameLine(ImGui::GetWindowWidth() - 320);

    // Theme Switcher Toggle
    if (currentTheme == ThemeMode::Dark) {
        if (ImGui::Button("Theme: Dark Mode", ImVec2(140, 26))) {
            currentTheme = ThemeMode::Light;
            Theme::applyTheme(currentTheme);
        }
    } else {
        if (ImGui::Button("Theme: Light Mode", ImVec2(140, 26))) {
            currentTheme = ThemeMode::Dark;
            Theme::applyTheme(currentTheme);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Help & DSA Docs", ImVec2(140, 26))) {
        currentTab = AppTab::SettingsHelp;
    }

    // Top Navigation Tabs
    ImGui::Spacing();
    auto tabButton = [&](const char* label, AppTab tab) {
        bool isActive = (currentTab == tab);
        if (isActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, Theme::getPrimaryColor());
        }
        if (ImGui::Button(label, ImVec2(170, 36))) {
            currentTab = tab;
        }
        if (isActive) {
            ImGui::PopStyleColor();
        }
        ImGui::SameLine();
    };

    tabButton("Dashboard", AppTab::Dashboard);
    tabButton("Compress Files", AppTab::Compress);
    tabButton("Decompress Files", AppTab::Decompress);
    tabButton("Algorithm Explorer", AppTab::AlgorithmExplorer);
    tabButton("About & Settings", AppTab::SettingsHelp);

    ImGui::NewLine();
    ImGui::PopStyleVar();
}

void AppWindow::renderDashboardTab() {
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.95f, 1.0f),
                       "Welcome to HuffmanX. Lossless compression engine demonstrating greedy algorithms, min-heaps, and binary file handling.");
    ImGui::Spacing();

    // 3 Quick Action Cards
    float cardWidth = (ImGui::GetContentRegionAvail().x - 24) / 3.0f;
    float cardHeight = 150.0f;

    // Card 1: Compress
    ImGui::BeginChild("CardCompress", ImVec2(cardWidth, cardHeight), true);
    ImGui::TextColored(Theme::getPrimaryColor(), "COMPRESS FILES");
    ImGui::Separator();
    ImGui::TextWrapped("Compress documents, images, video, audio, and raw binary into .hfx archives.");
    ImGui::Spacing();
    if (ImGui::Button("Open Compressor", ImVec2(-1, 32))) {
        currentTab = AppTab::Compress;
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Card 2: Decompress
    ImGui::BeginChild("CardDecompress", ImVec2(cardWidth, cardHeight), true);
    ImGui::TextColored(Theme::getSuccessColor(), "DECOMPRESS ARCHIVES");
    ImGui::Separator();
    ImGui::TextWrapped("Restore exact byte-for-byte original files from .hfx archives with CRC32 verification.");
    ImGui::Spacing();
    if (ImGui::Button("Open Decompressor", ImVec2(-1, 32))) {
        currentTab = AppTab::Decompress;
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Card 3: Algorithm Explorer
    ImGui::BeginChild("CardExplorer", ImVec2(cardWidth, cardHeight), true);
    ImGui::TextColored(Theme::getWarningColor(), "ALGORITHM EXPLORER");
    ImGui::Separator();
    ImGui::TextWrapped("Inspect the Huffman Tree, min-heap merges, and generated prefix-free codes.");
    ImGui::Spacing();
    if (ImGui::Button("Explore Tree & Codes", ImVec2(-1, 32))) {
        currentTab = AppTab::AlgorithmExplorer;
    }
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Supported File Categories Section
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.95f, 1.0f), "Supported File Types & Byte-Level Processing");
    ImGui::Spacing();

    auto renderCategoryBadge = [](const char* category, const char* exts) {
        ImVec4 col = Theme::getCategoryBadgeColor(category);
        ImGui::TextColored(col, "[ %s ]", category);
        ImGui::SameLine(160);
        ImGui::TextDisabled("%s", exts);
    };

    renderCategoryBadge("Documents", ".pdf, .docx, .doc, .txt, .rtf, .csv, .json, .xml");
    renderCategoryBadge("Images", ".jpg, .jpeg, .png, .bmp, .gif, .webp, .tif, .tiff");
    renderCategoryBadge("Videos", ".mp4, .vod, .mkv, .avi, .mov, .flv, .webm");
    renderCategoryBadge("Audio", ".mp3, .wav, .ogg, .flac, .m4a, .aac");
    renderCategoryBadge("Other Binary", "Any arbitrary binary file format (all 256 byte values preserved)");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Recent Activity Table
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.95f, 1.0f), "Recent Activity");
    if (recentActivities.empty()) {
        ImGui::TextDisabled("No files compressed or decompressed yet in this session.");
    } else {
        if (ImGui::BeginTable("ActivityTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("File Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Original Size", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("Output Size", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("Result Summary", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (auto it = recentActivities.rbegin(); it != recentActivities.rend(); ++it) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", it->timestamp.c_str());
                ImGui::TableNextColumn();
                ImGui::TextColored(it->actionType == "Compressed" ? Theme::getPrimaryColor() : Theme::getSuccessColor(),
                                   "%s", it->actionType.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%s", it->filename.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%s", CompressionStats::formatBytes(it->originalSize).c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%s", CompressionStats::formatBytes(it->resultSize).c_str());
                ImGui::TableNextColumn();
                ImGui::TextColored(it->success ? ImVec4(0.8f, 0.9f, 0.8f, 1.0f) : Theme::getDangerColor(),
                                   "%s", it->summary.c_str());
            }
            ImGui::EndTable();
        }
    }
}

void AppWindow::renderCompressTab() {
    ImGui::Spacing();
    ImGui::TextColored(Theme::getPrimaryColor(), "FILE COMPRESSOR");
    ImGui::TextDisabled("Select any file to compress using Huffman Coding. Drag and drop onto this window is supported.");
    ImGui::Spacing();

    // Input file picker
    ImGui::Text("Source File:");
    ImGui::InputText("##CompressInput", compressInputPath, sizeof(compressInputPath));
    ImGui::SameLine();
    if (ImGui::Button("Browse File...", ImVec2(120, 24))) {
        std::string selected = NativeFileDialog::openFileDialog();
        if (!selected.empty()) {
            std::strncpy(compressInputPath, selected.c_str(), sizeof(compressInputPath) - 1);
            std::string dir = FileUtils::getDirectoryPath(selected);
            std::strncpy(compressOutputDir, dir.c_str(), sizeof(compressOutputDir) - 1);
            std::string base = FileUtils::getBaseFilename(selected) + ".hfx";
            std::strncpy(compressOutputName, base.c_str(), sizeof(compressOutputName) - 1);
        }
    }

    // Show source file info card if selected
    if (FileUtils::fileExists(compressInputPath)) {
        uint64_t fSize = FileUtils::getFileSize(compressInputPath);
        std::string cat = FileUtils::detectFileTypeCategory(compressInputPath);
        std::string sig = FileUtils::detectFileSignature(compressInputPath);

        ImGui::BeginChild("SrcInfoCard", ImVec2(-1, 70), true);
        ImGui::Columns(3, "SrcCols", false);
        ImGui::TextDisabled("Original Size:");
        ImGui::TextColored(ImVec4(1, 1, 1, 1), "%s (%llu bytes)",
                           CompressionStats::formatBytes(fSize).c_str(), static_cast<unsigned long long>(fSize));
        ImGui::NextColumn();
        ImGui::TextDisabled("Category:");
        ImGui::TextColored(Theme::getCategoryBadgeColor(cat.c_str()), "%s", cat.c_str());
        ImGui::NextColumn();
        ImGui::TextDisabled("File Signature:");
        ImGui::Text("%s", sig.c_str());
        ImGui::Columns(1);
        ImGui::EndChild();
    }

    ImGui::Spacing();

    // Output Directory & Filename
    ImGui::Text("Destination Directory:");
    ImGui::InputText("##CompressOutDir", compressOutputDir, sizeof(compressOutputDir));
    ImGui::SameLine();
    if (ImGui::Button("Browse Folder...", ImVec2(120, 24))) {
        std::string selected = NativeFileDialog::selectFolderDialog();
        if (!selected.empty()) {
            std::strncpy(compressOutputDir, selected.c_str(), sizeof(compressOutputDir) - 1);
        }
    }

    ImGui::Text("Archive Name:");
    ImGui::InputText("##CompressOutName", compressOutputName, sizeof(compressOutputName));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Start Compression Button
    bool canStart = !isCompressing && FileUtils::fileExists(compressInputPath) &&
                    std::strlen(compressOutputDir) > 0 && std::strlen(compressOutputName) > 0;

    if (!canStart) ImGui::BeginDisabled();
    if (ImGui::Button("Start Compression", ImVec2(200, 38))) {
        isCompressing = true;
        compressProgress = 0.0f;
        cancelCompressFlag = false;
        hasCompressResult = false;
        std::strncpy(compressStage, "Starting compression...", sizeof(compressStage) - 1);

        std::string inPath = compressInputPath;
        std::string outNameStr = compressOutputName;
        if (FileUtils::getFileExtension(outNameStr) != ".hfx") {
            outNameStr += ".hfx";
            std::strncpy(compressOutputName, outNameStr.c_str(), sizeof(compressOutputName) - 1);
        }
        std::string outPath = (fs::path(compressOutputDir) / outNameStr).string();

        if (workerThread.joinable()) workerThread.join();

        workerThread = std::thread([this, inPath, outPath]() {
            HuffmanEncoder encoder;
            CompressionResult res = encoder.compressFile(
                inPath, outPath,
                [this](float prog, const std::string& stage) {
                    std::lock_guard<std::mutex> lock(stateMutex);
                    compressProgress = prog;
                    std::strncpy(compressStage, stage.c_str(), sizeof(compressStage) - 1);
                },
                &cancelCompressFlag
            );

            std::lock_guard<std::mutex> lock(stateMutex);
            lastCompressResult = res;
            hasCompressResult = true;
            isCompressing = false;

            // Log activity
            ActivityLogEntry log;
            log.timestamp = getCurrentTimeString();
            log.actionType = "Compressed";
            log.filename = FileUtils::getBaseFilename(inPath);
            log.originalSize = res.stats.originalSize;
            log.resultSize = res.stats.compressedSize;
            log.summary = res.success ? res.stats.statusMessage : res.errorMessage;
            log.success = res.success;
            recentActivities.push_back(log);

            // Update Explorer with this file's tree
            if (res.success) {
                updateExplorerWithFrequencies(encoder.getLastFrequencies(), res.stats.originalSize);
            }
        });
    }
    if (!canStart) ImGui::EndDisabled();

    if (isCompressing) {
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 38))) {
            cancelCompressFlag = true;
        }
    }

    // Live Progress Bar
    if (isCompressing) {
        ImGui::Spacing();
        ImGui::ProgressBar(compressProgress, ImVec2(-1, 24));
        ImGui::TextColored(Theme::getPrimaryColor(), "%s", compressStage);
    }

    // Results Section
    if (hasCompressResult) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (lastCompressResult.success) {
            const auto& stats = lastCompressResult.stats;
            ImGui::TextColored(Theme::getSuccessColor(), "COMPRESSION COMPLETED SUCCESSFULLY");
            ImGui::Spacing();

            ImGui::BeginChild("ResultCard", ImVec2(-1, 210), true);

            ImGui::Columns(4, "StatsCols", false);
            ImGui::TextDisabled("Original Size");
            ImGui::TextColored(ImVec4(1, 1, 1, 1), "%s", CompressionStats::formatBytes(stats.originalSize).c_str());

            ImGui::NextColumn();
            ImGui::TextDisabled("Compressed Size");
            ImGui::TextColored(stats.wasEnlarged ? Theme::getWarningColor() : Theme::getSuccessColor(),
                               "%s", CompressionStats::formatBytes(stats.compressedSize).c_str());

            ImGui::NextColumn();
            ImGui::TextDisabled("Space Saved");
            ImGui::TextColored(stats.wasEnlarged ? Theme::getWarningColor() : Theme::getSuccessColor(),
                               "%s (%+.1f%%)",
                               CompressionStats::formatSignedBytes(stats.spaceSaved).c_str(),
                               stats.spaceSavedPercent);

            ImGui::NextColumn();
            ImGui::TextDisabled("Compression Ratio");
            ImGui::Text("%.2f : 1", stats.compressionRatio);

            ImGui::Columns(1);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Visual Size Comparison Bar
            ImGui::TextDisabled("Size Comparison (Compressed vs Original):");
            float fraction = (stats.originalSize > 0) ?
                             static_cast<float>(stats.compressedSize) / static_cast<float>(stats.originalSize) : 0.0f;
            float displayFraction = std::min(fraction, 1.0f);

            char barOverlay[64];
            snprintf(barOverlay, sizeof(barOverlay), "Compressed: %.1f%% of Original", fraction * 100.0f);

            if (stats.wasEnlarged) {
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Theme::getWarningColor());
            } else {
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Theme::getSuccessColor());
            }
            ImGui::ProgressBar(displayFraction, ImVec2(-1, 22), barOverlay);
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::TextDisabled("Diagnostic: ");
            ImGui::SameLine();
            ImGui::TextWrapped("%s", stats.statusMessage.c_str());

            ImGui::TextDisabled("Duration: %s  |  Throughput: %.2f MB/s",
                                CompressionStats::formatDuration(stats.durationSeconds).c_str(),
                                stats.throughputMBps);

            ImGui::EndChild();

            ImGui::Spacing();
            ImGui::TextColored(Theme::getWarningColor(),
                "[*] Note: This file is a Huffman binary archive (.hfx), like a .zip file.\n"
                "    It cannot be directly opened by Windows Photos or media players until decompressed.");
            ImGui::Spacing();

            if (ImGui::Button("Decompress & Open Restored File", ImVec2(240, 32))) {
                HuffmanDecoder decoder;
                std::string restoredPath = (fs::path(compressOutputDir) / ("restored_" + FileUtils::getBaseFilename(compressInputPath))).string();
                DecompressionResult dRes = decoder.decompressFile(lastCompressResult.outputPath, restoredPath, compressOutputDir);
                if (dRes.success) {
                    #ifdef _WIN32
                    ShellExecuteA(NULL, "open", restoredPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                    #endif
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Open Output Folder", ImVec2(160, 32))) {
                FileUtils::openFolderInExplorer(lastCompressResult.outputPath);
            }
            ImGui::SameLine();
            if (ImGui::Button("Inspect in Algorithm Explorer", ImVec2(220, 32))) {
                currentTab = AppTab::AlgorithmExplorer;
            }
        } else {
            ImGui::TextColored(Theme::getDangerColor(), "Compression Error: %s", lastCompressResult.errorMessage.c_str());
        }
    }
}

void AppWindow::renderDecompressTab() {
    ImGui::Spacing();
    ImGui::TextColored(Theme::getSuccessColor(), "FILE DECOMPRESSOR");
    ImGui::TextDisabled("%s", "Select a .hfx archive to restore the original file. CRC32 checksum verification guarantees 100% integrity.");
    ImGui::Spacing();

    // Archive file picker
    ImGui::Text("HuffmanX Archive (.hfx):");
    ImGui::InputText("##DecompressInput", decompressArchivePath, sizeof(decompressArchivePath));
    ImGui::SameLine();
    if (ImGui::Button("Browse Archive...", ImVec2(130, 24))) {
        std::string selected = NativeFileDialog::openFileDialog("HuffmanX Archive (*.hfx)\0*.hfx\0\0", "hfx");
        if (!selected.empty()) {
            std::strncpy(decompressArchivePath, selected.c_str(), sizeof(decompressArchivePath) - 1);
            HuffmanDecoder decoder;
            inspectedArchiveMeta = decoder.inspectArchive(selected);
            hasInspectedArchive = inspectedArchiveMeta.isValid;
            if (hasInspectedArchive) {
                std::strncpy(decompressOutputName, inspectedArchiveMeta.originalFilename.c_str(), sizeof(decompressOutputName) - 1);
                std::string parentDir = FileUtils::getDirectoryPath(selected);
                std::strncpy(decompressOutputDir, parentDir.c_str(), sizeof(decompressOutputDir) - 1);
            }
        }
    }

    // Inspected Archive Card
    if (hasInspectedArchive) {
        ImGui::BeginChild("ArchiveInfoCard", ImVec2(-1, 80), true);
        ImGui::Columns(4, "ArchCols", false);
        ImGui::TextDisabled("Original Filename:");
        ImGui::TextColored(Theme::getPrimaryColor(), "%s", inspectedArchiveMeta.originalFilename.c_str());

        ImGui::NextColumn();
        ImGui::TextDisabled("Original Size:");
        ImGui::Text("%s", CompressionStats::formatBytes(inspectedArchiveMeta.header.originalSize).c_str());

        ImGui::NextColumn();
        ImGui::TextDisabled("Distinct Symbols:");
        ImGui::Text("%u / 256", inspectedArchiveMeta.header.symbolCount);

        ImGui::NextColumn();
        ImGui::TextDisabled("Expected CRC32:");
        ImGui::Text("0x%08X", inspectedArchiveMeta.header.originalCrc32);

        ImGui::Columns(1);
        ImGui::EndChild();
    } else if (std::strlen(decompressArchivePath) > 0 && FileUtils::fileExists(decompressArchivePath)) {
        ImGui::TextColored(Theme::getDangerColor(), "Selected file is not a valid HuffmanX archive: %s",
                           inspectedArchiveMeta.errorMessage.c_str());
    }

    ImGui::Spacing();

    // Output Directory & Filename
    ImGui::Text("Destination Directory:");
    ImGui::InputText("##DecompressOutDir", decompressOutputDir, sizeof(decompressOutputDir));
    ImGui::SameLine();
    if (ImGui::Button("Browse Folder...##Decompress", ImVec2(130, 24))) {
        std::string selected = NativeFileDialog::selectFolderDialog();
        if (!selected.empty()) {
            std::strncpy(decompressOutputDir, selected.c_str(), sizeof(decompressOutputDir) - 1);
        }
    }

    ImGui::Text("Restored Filename:");
    ImGui::InputText("##DecompressOutName", decompressOutputName, sizeof(decompressOutputName));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Start Decompression Button
    bool canStart = !isDecompressing && FileUtils::fileExists(decompressArchivePath) &&
                    std::strlen(decompressOutputDir) > 0 && std::strlen(decompressOutputName) > 0;

    if (!canStart) ImGui::BeginDisabled();
    if (ImGui::Button("Start Decompression", ImVec2(200, 38))) {
        isDecompressing = true;
        decompressProgress = 0.0f;
        cancelDecompressFlag = false;
        hasDecompressResult = false;
        std::strncpy(decompressStage, "Starting decompression...", sizeof(decompressStage) - 1);

        std::string archPath = decompressArchivePath;
        std::string outPath = (fs::path(decompressOutputDir) / decompressOutputName).string();
        std::string outDir = decompressOutputDir;

        if (workerThread.joinable()) workerThread.join();

        workerThread = std::thread([this, archPath, outPath, outDir]() {
            HuffmanDecoder decoder;
            DecompressionResult res = decoder.decompressFile(
                archPath, outPath, outDir,
                [this](float prog, const std::string& stage) {
                    std::lock_guard<std::mutex> lock(stateMutex);
                    decompressProgress = prog;
                    std::strncpy(decompressStage, stage.c_str(), sizeof(decompressStage) - 1);
                },
                &cancelDecompressFlag
            );

            std::lock_guard<std::mutex> lock(stateMutex);
            lastDecompressResult = res;
            hasDecompressResult = true;
            isDecompressing = false;

            // Log activity
            ActivityLogEntry log;
            log.timestamp = getCurrentTimeString();
            log.actionType = "Decompressed";
            log.filename = FileUtils::getBaseFilename(archPath);
            log.originalSize = res.restoredSize;
            log.resultSize = res.restoredSize;
            log.summary = res.success ? "Successfully restored byte-for-byte" : res.errorMessage;
            log.success = res.success;
            recentActivities.push_back(log);
        });
    }
    if (!canStart) ImGui::EndDisabled();

    if (isDecompressing) {
        ImGui::SameLine();
        if (ImGui::Button("Cancel##Decomp", ImVec2(100, 38))) {
            cancelDecompressFlag = true;
        }
    }

    // Live Progress Bar
    if (isDecompressing) {
        ImGui::Spacing();
        ImGui::ProgressBar(decompressProgress, ImVec2(-1, 24));
        ImGui::TextColored(Theme::getSuccessColor(), "%s", decompressStage);
    }

    // Results Section
    if (hasDecompressResult) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (lastDecompressResult.success) {
            ImGui::TextColored(Theme::getSuccessColor(), "DECOMPRESSION COMPLETED — ALL BYTES RESTORED EXACTLY!");
            ImGui::Spacing();

            ImGui::BeginChild("DecompResultCard", ImVec2(-1, 140), true);
            ImGui::Columns(3, "DecompCols", false);

            ImGui::TextDisabled("Restored File:");
            ImGui::Text("%s", FileUtils::getBaseFilename(lastDecompressResult.outputPath).c_str());

            ImGui::NextColumn();
            ImGui::TextDisabled("Restored Size:");
            ImGui::Text("%s (%llu bytes)",
                        CompressionStats::formatBytes(lastDecompressResult.restoredSize).c_str(),
                        static_cast<unsigned long long>(lastDecompressResult.restoredSize));

            ImGui::NextColumn();
            ImGui::TextDisabled("Integrity Check (CRC-32):");
            ImGui::TextColored(Theme::getSuccessColor(), "VERIFIED MATCH (0x%08X)", lastDecompressResult.actualCrc);

            ImGui::Columns(1);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextDisabled("Duration: %s  |  Throughput: %.2f MB/s  |  Path: %s",
                                CompressionStats::formatDuration(lastDecompressResult.durationSeconds).c_str(),
                                lastDecompressResult.throughputMBps,
                                lastDecompressResult.outputPath.c_str());

            ImGui::EndChild();

            ImGui::Spacing();
            if (ImGui::Button("Open Output Folder##Decomp", ImVec2(180, 32))) {
                FileUtils::openFolderInExplorer(lastDecompressResult.outputPath);
            }
        } else {
            ImGui::TextColored(Theme::getDangerColor(), "Decompression Failed: %s", lastDecompressResult.errorMessage.c_str());
        }
    }
}

void AppWindow::renderAlgorithmExplorerTab() {
    ImGui::Spacing();
    ImGui::TextColored(Theme::getWarningColor(), "ALGORITHM EXPLORER — DSA VISUALIZATION");
    ImGui::TextDisabled("Interactive demonstration of Huffman Tree construction, greedy min-heap merges, and variable-length prefix codes.");
    ImGui::Spacing();

    // Custom Sample Text Playground
    ImGui::Text("Test with Custom Text / Live Input:");
    if (ImGui::InputText("##ExplorerCustom", explorerCustomText, sizeof(explorerCustomText))) {
        updateExplorerWithText(explorerCustomText);
    }
    ImGui::SameLine();
    if (ImGui::Button("Update Tree", ImVec2(110, 24))) {
        updateExplorerWithText(explorerCustomText);
    }

    ImGui::Spacing();

    // Explorer Sub-tabs
    if (ImGui::Button(explorerSubTab == 0 ? "[ Interactive Tree Visualizer ]" : "Interactive Tree Visualizer", ImVec2(210, 28))) {
        explorerSubTab = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button(explorerSubTab == 1 ? "[ Symbol & Code Table ]" : "Symbol & Code Table", ImVec2(180, 28))) {
        explorerSubTab = 1;
    }
    ImGui::SameLine();
    if (ImGui::Button(explorerSubTab == 2 ? "[ Priority Queue Merges ]" : "Priority Queue Merges", ImVec2(200, 28))) {
        explorerSubTab = 2;
    }

    ImGui::Spacing();

    // View 0: Interactive Tree Visualizer
    if (explorerSubTab == 0) {
        ImVec2 canvasAvail = ImGui::GetContentRegionAvail();
        canvasAvail.y = std::max(canvasAvail.y - 10.0f, 380.0f);
        treeRenderer.render(explorerVisualNodes, explorerTotalBytes, canvasAvail);
    }
    // View 1: Symbol & Code Table
    else if (explorerSubTab == 1) {
        const auto& codeMap = explorerTree.getCodeMap();
        ImGui::Text("Distinct Symbols: %zu / 256  |  Total Stream Bytes: %llu",
                    explorerTree.getDistinctSymbolCount(), static_cast<unsigned long long>(explorerTotalBytes));
        ImGui::Spacing();

        if (ImGui::BeginTable("SymbolTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(-1, 380))) {
            ImGui::TableSetupColumn("Byte (Dec)", ImGuiTableColumnFlags_WidthFixed, 90);
            ImGui::TableSetupColumn("Hex", ImGuiTableColumnFlags_WidthFixed, 70);
            ImGui::TableSetupColumn("Symbol / ASCII", ImGuiTableColumnFlags_WidthFixed, 120);
            ImGui::TableSetupColumn("Frequency", ImGuiTableColumnFlags_WidthFixed, 110);
            ImGui::TableSetupColumn("Huffman Prefix Code", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Bits (vs 8b)", ImGuiTableColumnFlags_WidthFixed, 90);
            ImGui::TableHeadersRow();

            for (int i = 0; i < 256; ++i) {
                if (i < static_cast<int>(explorerFrequencies.size()) && explorerFrequencies[i] > 0) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", i);

                    ImGui::TableNextColumn();
                    ImGui::Text("0x%02X", i);

                    ImGui::TableNextColumn();
                    if (i >= 32 && i <= 126) {
                        ImGui::Text("'%c'", static_cast<char>(i));
                    } else if (i == '\n') {
                        ImGui::TextDisabled("\\n (newline)");
                    } else if (i == '\r') {
                        ImGui::TextDisabled("\\r (return)");
                    } else if (i == '\t') {
                        ImGui::TextDisabled("\\t (tab)");
                    } else if (i == 0) {
                        ImGui::TextDisabled("NUL");
                    } else {
                        ImGui::TextDisabled("Control / Binary");
                    }

                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", static_cast<unsigned long long>(explorerFrequencies[i]));

                    ImGui::TableNextColumn();
                    auto it = codeMap.find(static_cast<uint8_t>(i));
                    std::string code = (it != codeMap.end()) ? it->second : "";
                    ImGui::TextColored(Theme::getPrimaryColor(), "%s", code.c_str());

                    ImGui::TableNextColumn();
                    ImGui::Text("%zu bits", code.size());
                }
            }
            ImGui::EndTable();
        }
    }
    // View 2: Priority Queue Merges Timeline
    else if (explorerSubTab == 2) {
        const auto& merges = explorerTree.getMergeHistory();
        ImGui::Text("Total Greedy Merge Steps: %zu (combining 2 smallest min-heap nodes each step)", merges.size());
        ImGui::Spacing();

        if (merges.empty()) {
            ImGui::TextDisabled("Only 0 or 1 distinct symbol present; no tree merges required.");
        } else {
            if (ImGui::BeginTable("MergeTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(-1, 380))) {
                ImGui::TableSetupColumn("Step", ImGuiTableColumnFlags_WidthFixed, 60);
                ImGui::TableSetupColumn("Min #1 Node (Left)", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Min #2 Node (Right)", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Combined Frequency", ImGuiTableColumnFlags_WidthFixed, 150);
                ImGui::TableSetupColumn("Greedy Action", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                for (const auto& step : merges) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("#%zu", step.stepNumber);

                    ImGui::TableNextColumn();
                    ImGui::TextColored(Theme::getPrimaryColor(), "%s (f=%llu)",
                                       step.leftLabel.c_str(), static_cast<unsigned long long>(step.leftFreq));

                    ImGui::TableNextColumn();
                    ImGui::TextColored(Theme::getWarningColor(), "%s (f=%llu)",
                                       step.rightLabel.c_str(), static_cast<unsigned long long>(step.rightFreq));

                    ImGui::TableNextColumn();
                    ImGui::TextColored(Theme::getSuccessColor(), "%llu",
                                       static_cast<unsigned long long>(step.combinedFreq));

                    ImGui::TableNextColumn();
                    ImGui::TextDisabled("Create parent -> Re-insert into Min-Heap");
                }
                ImGui::EndTable();
            }
        }
    }
}

void AppWindow::renderSettingsHelpTab() {
    ImGui::Spacing();
    ImGui::TextColored(Theme::getPrimaryColor(), "DSA THEORY & COLLEGE EVALUATION REFERENCE");
    ImGui::Spacing();

    ImGui::BeginChild("TheoryScroll", ImVec2(-1, -1), true);

    ImGui::TextColored(Theme::getSuccessColor(), "1. Core DSA Concepts Demonstrated in HuffmanX:");
    ImGui::BulletText("Greedy Algorithm: Constructs the optimal prefix-free tree bottom-up by repeatedly choosing the two subtrees with the smallest frequencies.");
    ImGui::BulletText("Min-Heap / Priority Queue: Maintains candidates in O(log k) extraction and insertion time, yielding overall O(k log k) tree construction.");
    ImGui::BulletText("Binary Tree: Organizes symbols at leaf nodes with left edges corresponding to '0' and right edges to '1'.");
    ImGui::BulletText("Tree Traversal: DFS traversal generates variable-length codes; root-to-leaf traversal decodes compressed bitstreams.");
    ImGui::BulletText("Binary File Streams & Bit Packing: Byte-safe I/O buffering arbitrary binary data and packing bits MSB-first into raw bytes.");
    ImGui::BulletText("Integrity Checksum (CRC-32): Polynomial 0xEDB88320 verifies byte-for-byte correctness on decompression.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(Theme::getWarningColor(), "2. Time and Space Complexity Analysis:");
    ImGui::Text("  • Frequency Counting:      O(n) time, O(1) space (fixed 256-symbol alphabet).");
    ImGui::Text("  • Min-Heap Tree Merge:     O(k log k) time where k <= 256.");
    ImGui::Text("  • Code Generation:         O(k) time via DFS recursion.");
    ImGui::Text("  • Bitstream Encoding:      O(n) time streaming with 64KB I/O buffers.");
    ImGui::Text("  • Bitstream Decoding:      O(b) time where b is the number of bits in the payload.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(Theme::getPrimaryColor(), "3. Why do some files (e.g. JPG, MP4, DOCX) enlarge after compression?");
    ImGui::TextWrapped("Formats such as JPEG, PNG, MP4, MP3, and DOCX (which is a ZIP container) ALREADY employ advanced entropy coding "
                       "(such as Deflate, DCT quantization, or Huffman coding). Their byte distribution exhibits near-maximum Shannon Entropy "
                       "(near uniform 1/256 probability per byte). Storing the HuffmanX archive header (23 bytes) and frequency metadata "
                       "naturally results in an output that is slightly larger than the source file. HuffmanX reports this honestly.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "4. Custom Archive Binary Header Specification (HFX1):");
    ImGui::Text("  • Magic Number:     4 bytes ('HFX1' -> 0x48, 0x46, 0x58, 0x31)");
    ImGui::Text("  • Format Version:   1 byte (0x01)");
    ImGui::Text("  • Flags:            1 byte (Bit 0: Filename, Bit 1: CRC-32, Bit 2: Raw Store)");
    ImGui::Text("  • Original Size:    8 bytes (uint64_t little-endian)");
    ImGui::Text("  • Original CRC-32:  4 bytes (uint32_t checksum)");
    ImGui::Text("  • Last Byte Bits:   1 byte (valid bit count in final byte: 1..8)");
    ImGui::Text("  • Filename Length:  2 bytes (uint16_t, sanitized to prevent path traversal)");
    ImGui::Text("  • Symbol Count:     2 bytes (uint16_t, 0..256)");
    ImGui::Text("  • Symbol Table:     Symbol (1 byte) + Frequency (8 bytes) per active symbol");

    ImGui::EndChild();
}
