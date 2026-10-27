#include "NativeFileDialog.h"

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <codecvt>
#include <locale>

static std::wstring toWideString(const std::string& str) {
    if (str.empty()) return L"";
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0);
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), &wstr[0], sizeNeeded);
    return wstr;
}

static std::string toUtf8String(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
    std::string str(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), &str[0], sizeNeeded, NULL, NULL);
    return str;
}

std::string NativeFileDialog::openFileDialog(const std::string& filter, const std::string& defaultExt) {
    wchar_t fileName[MAX_PATH] = L"";

    // Build filter with double null termination
    std::wstring wFilter = L"All Supported Files\0*.pdf;*.docx;*.doc;*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.webp;*.mp4;*.mp3;*.wav;*.txt;*.bin;*.hfx\0"
                           L"HuffmanX Archives (*.hfx)\0*.hfx\0"
                           L"Documents (*.pdf;*.docx;*.doc;*.txt)\0*.pdf;*.docx;*.doc;*.txt\0"
                           L"Images (*.jpg;*.png;*.bmp;*.gif;*.webp)\0*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.webp\0"
                           L"Videos (*.mp4;*.vod)\0*.mp4;*.vod\0"
                           L"Audio (*.mp3;*.wav)\0*.mp3;*.wav\0"
                           L"All Files (*.*)\0*.*\0\0";

    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = wFilter.c_str();
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    std::wstring wDefExt = toWideString(defaultExt);
    if (!wDefExt.empty()) {
        ofn.lpstrDefExt = wDefExt.c_str();
    }

    if (GetOpenFileNameW(&ofn)) {
        return toUtf8String(fileName);
    }
    return "";
}

std::string NativeFileDialog::saveFileDialog(const std::string& defaultFileName,
                                            const std::string& filter,
                                            const std::string& defaultExt) {
    wchar_t fileName[MAX_PATH] = L"";
    std::wstring wDefault = toWideString(defaultFileName);
    if (!wDefault.empty()) {
        wcsncpy(fileName, wDefault.c_str(), MAX_PATH - 1);
    }

    std::wstring wFilter = L"HuffmanX Archive (*.hfx)\0*.hfx\0"
                           L"All Files (*.*)\0*.*\0\0";

    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = wFilter.c_str();
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    std::wstring wDefExt = toWideString(defaultExt.empty() ? "hfx" : defaultExt);
    ofn.lpstrDefExt = wDefExt.c_str();

    if (GetSaveFileNameW(&ofn)) {
        return toUtf8String(fileName);
    }
    return "";
}

std::string NativeFileDialog::selectFolderDialog() {
    BROWSEINFOW bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.hwndOwner = NULL;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpszTitle = L"Select Destination Directory";

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (pidl != NULL) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path)) {
            CoTaskMemFree(pidl);
            return toUtf8String(path);
        }
        CoTaskMemFree(pidl);
    }
    return "";
}

#else

std::string NativeFileDialog::openFileDialog(const std::string& filter, const std::string& defaultExt) { return ""; }
std::string NativeFileDialog::saveFileDialog(const std::string& def, const std::string& filter, const std::string& ext) { return ""; }
std::string NativeFileDialog::selectFolderDialog() { return ""; }

#endif
