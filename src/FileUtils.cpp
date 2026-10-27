#include "FileUtils.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

using namespace std;
namespace fs = filesystem;

uint64_t FileUtils::getFileSize(const string& path) {
    try {
        if (fs::exists(path) && fs::is_regular_file(path)) {
            return static_cast<uint64_t>(fs::file_size(path));
        }
    } catch (...) {}
    return 0;
}

bool FileUtils::fileExists(const string& path) {
    try {
        return fs::exists(path) && fs::is_regular_file(path);
    } catch (...) {
        return false;
    }
}

bool FileUtils::directoryExists(const string& path) {
    try {
        return fs::exists(path) && fs::is_directory(path);
    } catch (...) {
        return false;
    }
}

string FileUtils::getFileExtension(const string& path) {
    try {
        fs::path p(path);
        string ext = p.extension().string();
        transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
            return static_cast<char>(tolower(c));
        });
        return ext;
    } catch (...) {
        return "";
    }
}

string FileUtils::getFileNameWithoutExtension(const string& path) {
    try {
        fs::path p(path);
        return p.stem().string();
    } catch (...) {
        return "file";
    }
}

string FileUtils::getBaseFilename(const string& path) {
    try {
        fs::path p(path);
        return p.filename().string();
    } catch (...) {
        return path;
    }
}

string FileUtils::getDirectoryPath(const string& path) {
    try {
        fs::path p(path);
        return p.parent_path().string();
    } catch (...) {
        return ".";
    }
}

string FileUtils::detectFileTypeCategory(const string& path) {
    string ext = getFileExtension(path);

    if (ext == ".hfx") {
        return "HuffmanX Archive";
    }
    if (ext == ".pdf" || ext == ".docx" || ext == ".doc" || ext == ".txt" ||
        ext == ".md" || ext == ".rtf" || ext == ".csv" || ext == ".json" || ext == ".xml") {
        return "Documents";
    }
    if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" ||
        ext == ".gif" || ext == ".webp" || ext == ".tif" || ext == ".tiff" || ext == ".ico") {
        return "Images";
    }
    if (ext == ".mp4" || ext == ".mkv" || ext == ".avi" || ext == ".mov" ||
        ext == ".wmv" || ext == ".flv" || ext == ".webm" || ext == ".vod") {
        return "Videos";
    }
    if (ext == ".mp3" || ext == ".wav" || ext == ".ogg" || ext == ".flac" ||
        ext == ".m4a" || ext == ".aac") {
        return "Audio";
    }
    return "Other Binary Files";
}

string FileUtils::detectFileSignature(const string& path) {
    ifstream file(path, ios::binary);
    if (!file) return "Unknown / Inaccessible";

    uint8_t magic[16] = {0};
    file.read(reinterpret_cast<char*>(magic), sizeof(magic));
    streamsize bytesRead = file.gcount();
    if (bytesRead < 4) return "Generic Binary";

    // Check HuffmanX magic "HFX1"
    if (magic[0] == 'H' && magic[1] == 'F' && magic[2] == 'X' && magic[3] == '1') {
        return "HuffmanX Compressed Archive (.hfx)";
    }
    // PDF "%PDF"
    if (magic[0] == '%' && magic[1] == 'P' && magic[2] == 'D' && magic[3] == 'F') {
        return "PDF Document (%PDF)";
    }
    // PNG
    if (magic[0] == 0x89 && magic[1] == 'P' && magic[2] == 'N' && magic[3] == 'G') {
        return "PNG Image Data";
    }
    // JPEG
    if (magic[0] == 0xFF && magic[1] == 0xD8 && magic[2] == 0xFF) {
        return "JPEG Encoded Image";
    }
    // GIF
    if (magic[0] == 'G' && magic[1] == 'I' && magic[2] == 'F' && magic[3] == '8') {
        return "GIF Animated Image";
    }
    // ZIP / DOCX (PK\x03\x04)
    if (magic[0] == 'P' && magic[1] == 'K' && magic[2] == 0x03 && magic[3] == 0x04) {
        string ext = getFileExtension(path);
        if (ext == ".docx") return "Microsoft Word Document (ZIP-packaged XML)";
        return "ZIP Compressed Container";
    }
    // BMP
    if (magic[0] == 'B' && magic[1] == 'M') {
        return "Windows Bitmap Image (BMP)";
    }
    // MP3 (ID3 tag or MPEG sync)
    if ((magic[0] == 'I' && magic[1] == 'D' && magic[2] == '3') ||
        (magic[0] == 0xFF && (magic[1] & 0xE0) == 0xE0)) {
        return "MPEG Audio Stream (MP3)";
    }
    // RIFF (WAV or WEBP)
    if (magic[0] == 'R' && magic[1] == 'I' && magic[2] == 'F' && magic[3] == 'F' && bytesRead >= 12) {
        if (magic[8] == 'W' && magic[9] == 'A' && magic[10] == 'V' && magic[11] == 'E') {
            return "Waveform Audio Format (WAV)";
        }
        if (magic[8] == 'W' && magic[9] == 'E' && magic[10] == 'B' && magic[11] == 'P') {
            return "WebP Image (RIFF)";
        }
    }
    // MP4 ("ftyp" at offset 4)
    if (bytesRead >= 8 && magic[4] == 'f' && magic[5] == 't' && magic[6] == 'y' && magic[7] == 'p') {
        return "ISO Base Media / MP4 Video";
    }

    return "Arbitrary Binary Stream";
}

bool FileUtils::binaryCompareFiles(const string& pathA, const string& pathB) {
    if (pathA == pathB) return true;

    uint64_t sizeA = getFileSize(pathA);
    uint64_t sizeB = getFileSize(pathB);
    if (sizeA != sizeB) return false;
    if (sizeA == 0) return true; // Both 0 bytes

    ifstream fileA(pathA, ios::binary);
    ifstream fileB(pathB, ios::binary);
    if (!fileA || !fileB) return false;

    const size_t bufferSize = 65536;
    vector<char> bufA(bufferSize);
    vector<char> bufB(bufferSize);

    while (fileA && fileB) {
        fileA.read(bufA.data(), bufferSize);
        fileB.read(bufB.data(), bufferSize);

        streamsize readA = fileA.gcount();
        streamsize readB = fileB.gcount();

        if (readA != readB) return false;
        if (readA == 0) break;

        if (memcmp(bufA.data(), bufB.data(), static_cast<size_t>(readA)) != 0) {
            return false;
        }
    }

    return fileA.eof() && fileB.eof();
}

string FileUtils::createTempFilePath(const string& targetPath) {
    fs::path target(targetPath);
    auto now = chrono::steady_clock::now().time_since_epoch().count();
    string tempName = target.stem().string() + ".tmp_" + to_string(now) + target.extension().string();
    return (target.parent_path() / tempName).string();
}

bool FileUtils::atomicReplaceFile(const string& tempPath, const string& targetPath) {
    try {
        fs::rename(tempPath, targetPath);
        return true;
    } catch (...) {
        try {
            // If direct rename fails due to cross-device or permission, copy then delete
            fs::copy_file(tempPath, targetPath, fs::copy_options::overwrite_existing);
            fs::remove(tempPath);
            return true;
        } catch (...) {
            return false;
        }
    }
}

bool FileUtils::openFolderInExplorer(const string& filePath) {
#ifdef _WIN32
    string normPath = filePath;
    replace(normPath.begin(), normPath.end(), '/', '\\');

    if (fs::is_regular_file(normPath)) {
        string param = "/select,\"" + normPath + "\"";
        HINSTANCE res = ShellExecuteA(NULL, "open", "explorer.exe", param.c_str(), NULL, SW_SHOWNORMAL);
        return reinterpret_cast<INT_PTR>(res) > 32;
    } else if (fs::is_directory(normPath)) {
        HINSTANCE res = ShellExecuteA(NULL, "open", normPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        return reinterpret_cast<INT_PTR>(res) > 32;
    }
#endif
    return false;
}
