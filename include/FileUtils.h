#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <string>
#include <vector>
#include <cstdint>

/**
 * @brief Cross-platform binary file and filesystem utility helper.
 */
class FileUtils {
public:
    /**
     * @brief Returns size of file in bytes, or 0 if not accessible.
     */
    static uint64_t getFileSize(const std::string& path);

    /**
     * @brief Checks if a file exists on disk.
     */
    static bool fileExists(const std::string& path);

    /**
     * @brief Checks if a directory exists on disk.
     */
    static bool directoryExists(const std::string& path);

    /**
     * @brief Extracts lower-case file extension (including dot, e.g. ".pdf").
     */
    static std::string getFileExtension(const std::string& path);

    /**
     * @brief Extracts the file name without extension.
     */
    static std::string getFileNameWithoutExtension(const std::string& path);

    /**
     * @brief Extracts the basename (e.g. "report.docx" from "C:/docs/report.docx").
     */
    static std::string getBaseFilename(const std::string& path);

    /**
     * @brief Extracts the directory containing the file.
     */
    static std::string getDirectoryPath(const std::string& path);

    /**
     * @brief Categorizes file based on extension and signature:
     * "Documents", "Images", "Videos", "Audio", "HuffmanX Archive", or "General Binary".
     */
    static std::string detectFileTypeCategory(const std::string& path);

    /**
     * @brief Reads leading bytes to determine real binary file signature.
     */
    static std::string detectFileSignature(const std::string& path);

    /**
     * @brief Compares two files byte-by-byte. Returns true if identical.
     */
    static bool binaryCompareFiles(const std::string& pathA, const std::string& pathB);

    /**
     * @brief Generates a unique temporary file path next to targetPath.
     */
    static std::string createTempFilePath(const std::string& targetPath);

    /**
     * @brief Atomically renames temporary file to targetPath.
     */
    static bool atomicReplaceFile(const std::string& tempPath, const std::string& targetPath);

    /**
     * @brief Opens the folder containing the file in Windows File Explorer.
     */
    static bool openFolderInExplorer(const std::string& filePath);
};

#endif // FILE_UTILS_H
