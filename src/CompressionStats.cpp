#include "CompressionStats.h"
#include <iomanip>
#include <sstream>
#include <cmath>

using namespace std;

CompressionStats::CompressionStats()
    : originalSize(0),
      compressedSize(0),
      spaceSaved(0),
      compressionRatio(0.0),
      spaceSavedPercent(0.0),
      durationSeconds(0.0),
      throughputMBps(0.0),
      wasEnlarged(false),
      isZeroByteFile(false),
      statusMessage("Not started") {}

void CompressionStats::compute(uint64_t origSize, uint64_t compSize, double durationSec) {
    originalSize = origSize;
    compressedSize = compSize;
    durationSeconds = durationSec;
    spaceSaved = static_cast<int64_t>(origSize) - static_cast<int64_t>(compSize);
    wasEnlarged = (compSize > origSize);
    isZeroByteFile = (origSize == 0);

    // Compression Ratio = Original size / Compressed size
    if (compSize > 0) {
        compressionRatio = static_cast<double>(origSize) / static_cast<double>(compSize);
    } else {
        compressionRatio = 0.0;
    }

    // Space Saved (%) = ((Original size - Compressed size) / Original size) * 100
    if (origSize > 0) {
        spaceSavedPercent = (static_cast<double>(spaceSaved) / static_cast<double>(origSize)) * 100.0;
    } else {
        spaceSavedPercent = 0.0;
    }

    // Throughput in MB/s
    if (durationSec > 0.0001) {
        double megabytes = static_cast<double>(origSize) / (1024.0 * 1024.0);
        throughputMBps = megabytes / durationSec;
    } else {
        throughputMBps = 0.0;
    }

    // Formulate honest, educational status messages
    if (isZeroByteFile) {
        statusMessage = "Zero-byte file compressed. Archive header and metadata were stored.";
    } else if (compSize < origSize) {
        ostringstream oss;
        oss << "Compression successful! Space saved: " << fixed << setprecision(1)
            << spaceSavedPercent << "% (" << formatBytes(static_cast<uint64_t>(spaceSaved)) << ").";
        statusMessage = oss.str();
    } else if (compSize > origSize) {
        ostringstream oss;
        oss << "Notice: Output is larger than input by " << formatBytes(static_cast<uint64_t>(-spaceSaved))
            << " (" << fixed << setprecision(1) << -spaceSavedPercent
            << "% increase). This file is already compressed or contains high entropy.";
        statusMessage = oss.str();
    } else {
        statusMessage = "Compression completed. Output size is identical to input size.";
    }
}

string CompressionStats::formatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double dBytes = static_cast<double>(bytes);

    while (dBytes >= 1024.0 && unitIndex < 4) {
        dBytes /= 1024.0;
        unitIndex++;
    }

    ostringstream oss;
    if (unitIndex == 0) {
        oss << bytes << " B";
    } else {
        oss << fixed << setprecision(2) << dBytes << " " << units[unitIndex];
    }
    return oss.str();
}

string CompressionStats::formatSignedBytes(int64_t bytes) {
    if (bytes >= 0) {
        return "+" + formatBytes(static_cast<uint64_t>(bytes));
    } else {
        return "-" + formatBytes(static_cast<uint64_t>(-bytes));
    }
}

string CompressionStats::formatDuration(double seconds) {
    ostringstream oss;
    if (seconds < 0.001) {
        oss << fixed << setprecision(1) << (seconds * 1000000.0) << " \u00B5s";
    } else if (seconds < 1.0) {
        oss << fixed << setprecision(1) << (seconds * 1000.0) << " ms";
    } else {
        oss << fixed << setprecision(2) << seconds << " s";
    }
    return oss.str();
}
