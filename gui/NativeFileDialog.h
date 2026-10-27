#ifndef NATIVE_FILE_DIALOG_H
#define NATIVE_FILE_DIALOG_H

#include <string>
#include <vector>

/**
 * @brief Native Windows File and Folder picker dialogs.
 */
class NativeFileDialog {
public:
    /**
     * @brief Displays native Windows Open File dialog.
     * @param filter Filter string (e.g. "All Files (*.*)\0*.*\0Documents (*.pdf;*.docx)\0*.pdf;*.docx\0")
     * @param defaultExt Default file extension.
     * @return std::string Selected file path, or empty string if cancelled.
     */
    static std::string openFileDialog(const std::string& filter = "All Files (*.*)\0*.*\0",
                                      const std::string& defaultExt = "");

    /**
     * @brief Displays native Windows Save File dialog.
     * @param defaultFileName Pre-filled file name.
     * @param filter Filter string.
     * @param defaultExt Default extension (e.g. "hfx").
     * @return std::string Selected destination path, or empty string if cancelled.
     */
    static std::string saveFileDialog(const std::string& defaultFileName = "",
                                      const std::string& filter = "HuffmanX Archive (*.hfx)\0*.hfx\0All Files (*.*)\0*.*\0",
                                      const std::string& defaultExt = "hfx");

    /**
     * @brief Displays native Windows folder selection dialog.
     */
    static std::string selectFolderDialog();
};

#endif // NATIVE_FILE_DIALOG_H
