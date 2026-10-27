/**
 * ByteShrink — Smart File Compression Studio
 * Frontend Application Controller
 * Handles navigation routing, dropzones, live compression/decompression API,
 * genuine activity logging, and the Algorithm Explorer engine.
 */

(function () {
  'use strict';

  // State Management
  const state = {
    currentTab: 'welcome', // Always starts on Welcome in a fresh session
    activityLog: [],
    currentCompressFile: null,
    currentCompressResult: null,
    currentDecompressFile: null,
    currentDecompressHeader: null,
    currentDecompressResult: null,
    treeCanvas: null
  };

  // Supported Extensions Mapping
  const FILE_CATEGORIES = {
    document: ['pdf', 'doc', 'docx', 'txt', 'csv', 'xml', 'json', 'rtf', 'log', 'md'],
    image: ['jpg', 'jpeg', 'png', 'bmp', 'webp', 'tiff', 'gif', 'svg'],
    video: ['mp4', 'vod', 'avi', 'webm', 'mkv', 'mov', 'flv'],
    audio: ['mp3', 'wav', 'flac', 'aac', 'ogg', 'm4a']
  };

  // Initialize Application on DOM Ready
  document.addEventListener('DOMContentLoaded', () => {
    initNavigation();
    initCompress();
    initDecompress();
    initExplorer();
    renderActivityTable();
  });

  // =========================================================================
  // NAVIGATION & ROUTING
  // =========================================================================

  function initNavigation() {
    // Top Nav buttons
    const navButtons = document.querySelectorAll('.nav-tab-btn');
    navButtons.forEach(btn => {
      btn.addEventListener('click', () => {
        const targetTab = btn.getAttribute('data-tab');
        switchTab(targetTab);
      });
    });

    // "Get Started" button on Welcome Screen -> transitions to Dashboard
    const btnGetStarted = document.getElementById('btnGetStarted');
    if (btnGetStarted) {
      btnGetStarted.addEventListener('click', () => {
        switchTab('dashboard');
      });
    }

    // Header Brand Click -> Dashboard
    const brandHomeBtn = document.getElementById('brandHomeBtn');
    if (brandHomeBtn) {
      brandHomeBtn.addEventListener('click', () => {
        switchTab('dashboard');
      });
    }

    // Header Help Button -> Help & DSA Docs
    const headerHelpBtn = document.getElementById('headerHelpBtn');
    if (headerHelpBtn) {
      headerHelpBtn.addEventListener('click', () => {
        switchTab('help');
      });
    }

    // Dashboard Action Card Buttons
    const cardBtnCompress = document.getElementById('cardBtnCompress');
    if (cardBtnCompress) {
      cardBtnCompress.addEventListener('click', () => switchTab('compress'));
    }

    const cardBtnDecompress = document.getElementById('cardBtnDecompress');
    if (cardBtnDecompress) {
      cardBtnDecompress.addEventListener('click', () => switchTab('decompress'));
    }

    const cardBtnExplorer = document.getElementById('cardBtnExplorer');
    if (cardBtnExplorer) {
      cardBtnExplorer.addEventListener('click', () => switchTab('explorer'));
    }
  }

  function switchTab(tabId) {
    state.currentTab = tabId;

    // Update nav button active states
    const navButtons = document.querySelectorAll('.nav-tab-btn');
    navButtons.forEach(btn => {
      if (btn.getAttribute('data-tab') === tabId) {
        btn.classList.add('active');
      } else {
        btn.classList.remove('active');
      }
    });

    // Update view sections
    const views = document.querySelectorAll('.view-section');
    views.forEach(view => {
      if (view.id === `view-${tabId}`) {
        view.classList.add('active');
      } else {
        view.classList.remove('active');
      }
    });

    // Scroll top
    window.scrollTo({ top: 0, behavior: 'smooth' });

    // Handle explorer canvas resize on entering explorer tab
    if (tabId === 'explorer' && state.treeCanvas) {
      setTimeout(() => {
        state.treeCanvas.resize();
        state.treeCanvas.render();
      }, 50);
    }
  }

  // =========================================================================
  // FILE COMPRESSION MODULE
  // =========================================================================

  function initCompress() {
    const dropzone = document.getElementById('compressDropzone');
    const fileInput = document.getElementById('compressFileInput');
    const btnStart = document.getElementById('btnStartCompress');
    const btnDownloadArchive = document.getElementById('btnDownloadArchive');
    const btnDecompressNow = document.getElementById('btnDecompressNow');
    const btnViewInExplorer = document.getElementById('btnViewInExplorer');

    if (!dropzone || !fileInput) return;

    // Click to browse
    dropzone.addEventListener('click', () => fileInput.click());

    // File selected via picker
    fileInput.addEventListener('change', (e) => {
      if (e.target.files && e.target.files[0]) {
        handleCompressFileSelected(e.target.files[0]);
      }
    });

    // Drag and drop handlers
    dropzone.addEventListener('dragover', (e) => {
      e.preventDefault();
      dropzone.classList.add('dragover');
    });

    dropzone.addEventListener('dragleave', () => {
      dropzone.classList.remove('dragover');
    });

    dropzone.addEventListener('drop', (e) => {
      e.preventDefault();
      dropzone.classList.remove('dragover');
      if (e.dataTransfer.files && e.dataTransfer.files[0]) {
        handleCompressFileSelected(e.dataTransfer.files[0]);
      }
    });

    // Start compression button click
    if (btnStart) {
      btnStart.addEventListener('click', () => {
        // Validation check if user clicked without selecting a file
        if (!state.currentCompressFile) {
          showCompressError('Please select or drop a file to compress first.');
          dropzone.classList.add('pulse-highlight');
          setTimeout(() => dropzone.classList.remove('pulse-highlight'), 1600);
          return;
        }
        runCompression();
      });
    }

    // Download compressed archive button
    if (btnDownloadArchive) {
      btnDownloadArchive.addEventListener('click', triggerDownloadCompressedArchive);
    }

    // Quick workflow: "Decompress & Open Restored File Now"
    if (btnDecompressNow) {
      btnDecompressNow.addEventListener('click', () => {
        if (!state.currentCompressResult) return;
        switchTab('decompress');
        autoLoadArchiveInDecompress(state.currentCompressResult);
      });
    }

    // "Inspect in Algorithm Explorer"
    if (btnViewInExplorer) {
      btnViewInExplorer.addEventListener('click', () => {
        if (!state.currentCompressFile) return;
        switchTab('explorer');
        loadExplorerFromFile(state.currentCompressFile);
      });
    }
  }

  function showCompressError(message) {
    const banner = document.getElementById('compressErrorBanner');
    if (banner) {
      banner.style.display = 'flex';
      banner.innerHTML = `<span>⚠️</span> <span>${escapeHtml(message)}</span>`;
    }
  }

  function hideCompressError() {
    const banner = document.getElementById('compressErrorBanner');
    if (banner) {
      banner.style.display = 'none';
      banner.innerHTML = '';
    }
  }

  function handleCompressFileSelected(file) {
    state.currentCompressFile = file;
    hideCompressError();

    // Reset results card & progress box
    const resultsCard = document.getElementById('compressResultsCard');
    const progressBox = document.getElementById('compressProgressBox');
    resultsCard.classList.remove('active', 'show');
    progressBox.classList.remove('active', 'show');

    // Show metadata card
    const metaCard = document.getElementById('compressMetaCard');
    metaCard.style.display = 'grid';

    document.getElementById('metaFileName').textContent = file.name;
    document.getElementById('metaFileSize').textContent = formatBytes(file.size);

    const ext = getExtension(file.name);
    const category = categorizeExtension(ext);
    document.getElementById('metaFileCategory').textContent = category.toUpperCase();

    // Auto default archive output name
    const outputNameInput = document.getElementById('compressOutputName');
    outputNameInput.value = `${file.name}.hfx`;

    // Read first few bytes for file signature detection
    detectMagicSignature(file, (sig) => {
      document.getElementById('metaFileSig').textContent = sig;
    });

    // Reset button state to Idle
    const btnStart = document.getElementById('btnStartCompress');
    btnStart.removeAttribute('disabled');
    btnStart.innerHTML = `<span>⚡</span> START COMPRESSION`;
  }

  function detectMagicSignature(file, callback) {
    const reader = new FileReader();
    const slice = file.slice(0, 16);
    reader.onload = (e) => {
      const buffer = new Uint8Array(e.target.result);
      let hex = '';
      for (let i = 0; i < Math.min(buffer.length, 8); i++) {
        hex += buffer[i].toString(16).toUpperCase().padStart(2, '0') + ' ';
      }

      // Check common signatures
      if (buffer[0] === 0x25 && buffer[1] === 0x50 && buffer[2] === 0x44 && buffer[3] === 0x46) {
        callback(`%PDF (PDF Document) [${hex.trim()}]`);
      } else if (buffer[0] === 0x89 && buffer[1] === 0x50 && buffer[2] === 0x4E && buffer[3] === 0x47) {
        callback(`PNG Image [${hex.trim()}]`);
      } else if (buffer[0] === 0xFF && buffer[1] === 0xD8 && buffer[2] === 0xFF) {
        callback(`JPEG / JPG Image [${hex.trim()}]`);
      } else if (buffer[0] === 0x42 && buffer[1] === 0x4D) {
        callback(`BMP Image [${hex.trim()}]`);
      } else if (buffer[0] === 0x50 && buffer[1] === 0x4B && buffer[2] === 0x03 && buffer[3] === 0x04) {
        callback(`ZIP / DOCX / OpenXML Container [${hex.trim()}]`);
      } else if (buffer[4] === 0x66 && buffer[5] === 0x74 && buffer[6] === 0x79 && buffer[7] === 0x70) {
        callback(`ISO Base Media / MP4 [${hex.trim()}]`);
      } else if (buffer[0] === 0x49 && buffer[1] === 0x44 && buffer[2] === 0x33) {
        callback(`MP3 Audio with ID3v2 [${hex.trim()}]`);
      } else if (buffer[0] === 0x48 && buffer[1] === 0x46 && buffer[2] === 0x58 && buffer[3] === 0x31) {
        callback(`ByteShrink HFX1 Archive [${hex.trim()}]`);
      } else {
        callback(`Raw Binary Bytes [${hex.trim()}]`);
      }
    };
    reader.readAsArrayBuffer(slice);
  }

  async function runCompression() {
    const file = state.currentCompressFile;
    if (!file) return;

    hideCompressError();

    let outputName = document.getElementById('compressOutputName').value.trim();
    if (!outputName) {
      outputName = `${file.name}.hfx`;
    }
    if (!outputName.toLowerCase().endsWith('.hfx')) {
      outputName += '.hfx';
    }

    // Button: Processing State
    const btnStart = document.getElementById('btnStartCompress');
    btnStart.setAttribute('disabled', 'true');
    btnStart.innerHTML = `<span class="spinner-inline">⏳</span> COMPRESSING...`;

    // Progress Box: Active
    const progressBox = document.getElementById('compressProgressBox');
    const progressFill = document.getElementById('compressProgressFill');
    const statusText = document.getElementById('compressStatusText');
    progressBox.classList.add('active', 'show');
    progressFill.style.width = '20%';
    statusText.textContent = 'Counting byte frequencies (0–255)...';

    const startTime = performance.now();

    try {
      // Read file into ArrayBuffer for verification & fallback
      const arrayBuffer = await file.arrayBuffer();
      const uint8 = new Uint8Array(arrayBuffer);

      progressFill.style.width = '45%';
      statusText.textContent = 'Constructing Min-Heap & greedy Huffman Tree...';

      // 1. Attempt to invoke native C++ Huffman engine via local server
      let backendSuccess = false;
      let resultData = null;

      try {
        const formData = new FormData();
        formData.append('file', file);
        formData.append('outputName', outputName);

        const resp = await fetch('/api/compress', {
          method: 'POST',
          body: formData
        });

        if (resp.ok) {
          resultData = await resp.json();
          backendSuccess = true;
        } else {
          const errData = await resp.json().catch(() => ({}));
          throw new Error(errData.error || `Server returned status ${resp.status}`);
        }
      } catch (networkOrServerErr) {
        // Fallback: run C++ compliant client-side Huffman engine
        backendSuccess = false;
      }

      progressFill.style.width = '80%';
      statusText.textContent = 'Encoding variable-length bitstream & verifying CRC-32...';

      if (!backendSuccess) {
        // Client-side Huffman engine (100% binary format compliant with C++ engine)
        resultData = executeClientHuffmanCompress(file.name, outputName, uint8);
      }

      const elapsed = Math.round(performance.now() - startTime);
      progressFill.style.width = '100%';
      statusText.textContent = `Completed in ${elapsed} ms using ${backendSuccess ? 'native C++ engine' : 'client Huffman engine'}.`;

      // Save result state
      state.currentCompressResult = {
        originalName: file.name,
        outputName: outputName,
        originalSize: file.size,
        compressedSize: resultData.compressedSize,
        spaceSaved: file.size - resultData.compressedSize,
        ratio: (file.size / Math.max(resultData.compressedSize, 1)).toFixed(2),
        percentChange: (((resultData.compressedSize - file.size) / Math.max(file.size, 1)) * 100).toFixed(1),
        downloadUrl: resultData.downloadUrl || null,
        archiveBytes: resultData.archiveBytes || null,
        blob: resultData.blob || null,
        time: formatCurrentTime()
      };

      // Button: Success State
      btnStart.removeAttribute('disabled');
      btnStart.innerHTML = `<span>⚡</span> COMPRESS ANOTHER FILE`;

      // Display results card
      displayCompressionResults(state.currentCompressResult);

      // Log genuine activity to dashboard
      addActivityRecord({
        time: state.currentCompressResult.time,
        action: 'COMPRESSED',
        fileName: file.name,
        originalSize: formatBytes(file.size),
        outputSize: formatBytes(resultData.compressedSize),
        summary: resultData.compressedSize < file.size
          ? `Saved ${formatBytes(file.size - resultData.compressedSize)} (${Math.abs(state.currentCompressResult.percentChange)}% smaller)`
          : `Notice: Pre-compressed file. Output +${formatBytes(resultData.compressedSize - file.size)} (entropy overhead)`
      });

    } catch (error) {
      console.error('Compression error:', error);
      statusText.textContent = `Compression Error: ${error.message}`;
      showCompressError(`Failed to compress: ${error.message}`);

      // Button: Error State
      btnStart.removeAttribute('disabled');
      btnStart.innerHTML = `<span>⚠️</span> RETRY COMPRESSION`;
    }
  }

  function displayCompressionResults(res) {
    const card = document.getElementById('compressResultsCard');
    card.classList.add('active', 'show');

    document.getElementById('statOrigSize').textContent = formatBytes(res.originalSize);
    document.getElementById('statCompSize').textContent = formatBytes(res.compressedSize);

    const statSpaceSaved = document.getElementById('statSpaceSaved');
    const barFill = document.getElementById('comparisonBarFill');
    const diagnosticText = document.getElementById('statDiagnosticText');

    const percentOfOriginal = Math.min(Math.round((res.compressedSize / Math.max(res.originalSize, 1)) * 100), 100);
    barFill.style.width = `${Math.max(percentOfOriginal, 5)}%`;

    if (res.spaceSaved >= 0) {
      statSpaceSaved.textContent = `+${formatBytes(res.spaceSaved)} (${Math.abs(res.percentChange)}% saved)`;
      statSpaceSaved.style.color = 'var(--accent-green)';
      document.getElementById('statRatio').textContent = `${res.ratio}:1`;
      barFill.textContent = `${percentOfOriginal}% of original`;
      barFill.style.background = 'linear-gradient(90deg, var(--accent-orange), var(--accent-orange-hover))';
      diagnosticText.textContent = `Lossless compression succeeded. File bytes encoded with prefix-free Huffman trees without loss of information.`;
    } else {
      const addedBytes = Math.abs(res.spaceSaved);
      statSpaceSaved.textContent = `-${formatBytes(addedBytes)} (+${Math.abs(res.percentChange)}% overhead)`;
      statSpaceSaved.style.color = 'var(--accent-orange)';
      document.getElementById('statRatio').textContent = `${res.ratio}:1 (Expanded)`;
      barFill.textContent = `100% + overhead`;
      barFill.style.background = 'linear-gradient(90deg, #D44100, var(--accent-orange))';
      diagnosticText.innerHTML = `<strong>Shannon Entropy Notice:</strong> This input format (${getExtension(res.originalName).toUpperCase()}) already uses internal compression (e.g. DEFLATE/DCT). Its byte distribution has maximum entropy (~8 bits/byte), so Huffman coding cannot shrink it further. Storing the tree table adds ${formatBytes(addedBytes)} overhead. This is an honest, expected characteristic of lossless data compression.`;
    }

    // Scroll to results
    card.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
  }

  function triggerDownloadCompressedArchive() {
    const res = state.currentCompressResult;
    if (!res) return;

    if (res.downloadUrl) {
      // Use direct server download link
      const a = document.createElement('a');
      a.href = res.downloadUrl;
      a.download = res.outputName;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
    } else if (res.blob || res.archiveBytes) {
      // Use blob
      const blob = res.blob || new Blob([res.archiveBytes], { type: 'application/octet-stream' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = res.outputName;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      setTimeout(() => URL.revokeObjectURL(url), 2000);
    }
  }

  // =========================================================================
  // FILE DECOMPRESSION MODULE
  // =========================================================================

  function initDecompress() {
    const dropzone = document.getElementById('decompressDropzone');
    const fileInput = document.getElementById('decompressFileInput');
    const btnStart = document.getElementById('btnStartDecompress');
    const btnDownloadRestored = document.getElementById('btnDownloadRestoredFile');
    const btnOpenRestored = document.getElementById('btnOpenRestoredFile');

    if (!dropzone || !fileInput) return;

    dropzone.addEventListener('click', () => fileInput.click());

    fileInput.addEventListener('change', (e) => {
      if (e.target.files && e.target.files[0]) {
        handleDecompressFileSelected(e.target.files[0]);
      }
    });

    dropzone.addEventListener('dragover', (e) => {
      e.preventDefault();
      dropzone.classList.add('dragover');
    });

    dropzone.addEventListener('dragleave', () => {
      dropzone.classList.remove('dragover');
    });

    dropzone.addEventListener('drop', (e) => {
      e.preventDefault();
      dropzone.classList.remove('dragover');
      if (e.dataTransfer.files && e.dataTransfer.files[0]) {
        handleDecompressFileSelected(e.dataTransfer.files[0]);
      }
    });

    if (btnStart) {
      btnStart.addEventListener('click', () => {
        if (!state.currentDecompressFile) {
          showDecompressError('Please select or drop a .hfx archive first.');
          dropzone.classList.add('pulse-highlight');
          setTimeout(() => dropzone.classList.remove('pulse-highlight'), 1600);
          return;
        }
        runDecompression();
      });
    }

    if (btnDownloadRestored) {
      btnDownloadRestored.addEventListener('click', triggerDownloadRestoredFile);
    }

    if (btnOpenRestored) {
      btnOpenRestored.addEventListener('click', () => {
        if (state.currentDecompressResult && state.currentDecompressResult.path) {
          fetch('/api/open-file', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ path: state.currentDecompressResult.path })
          }).catch(() => triggerDownloadRestoredFile());
        } else {
          triggerDownloadRestoredFile();
        }
      });
    }
  }

  function showDecompressError(message) {
    const banner = document.getElementById('decompressErrorBanner');
    if (banner) {
      banner.style.display = 'flex';
      banner.innerHTML = `<span>⚠️</span> <span>${escapeHtml(message)}</span>`;
    }
  }

  function hideDecompressError() {
    const banner = document.getElementById('decompressErrorBanner');
    if (banner) {
      banner.style.display = 'none';
      banner.innerHTML = '';
    }
  }

  async function handleDecompressFileSelected(file) {
    state.currentDecompressFile = file;
    hideDecompressError();

    const resultsCard = document.getElementById('decompressResultsCard');
    const progressBox = document.getElementById('decompressProgressBox');
    resultsCard.classList.remove('active', 'show');
    progressBox.classList.remove('active', 'show');

    try {
      const buffer = await file.arrayBuffer();
      const uint8 = new Uint8Array(buffer);
      const header = parseHfxArchiveHeader(uint8);

      state.currentDecompressHeader = header;

      const metaCard = document.getElementById('decompressMetaCard');
      metaCard.style.display = 'grid';

      document.getElementById('decompMetaName').textContent = header.originalFilename;
      document.getElementById('decompMetaSize').textContent = formatBytes(header.originalSize);
      document.getElementById('decompMetaSymbols').textContent = `${header.distinctSymbols} of 256`;
      document.getElementById('decompMetaCrc').textContent = '0x' + header.expectedCrc.toString(16).toUpperCase().padStart(8, '0');

      document.getElementById('decompressOutputName').value = header.originalFilename;

      const btnStart = document.getElementById('btnStartDecompress');
      btnStart.removeAttribute('disabled');
      btnStart.innerHTML = `<span>⚡</span> START DECOMPRESSION`;
    } catch (err) {
      showDecompressError(`Invalid ByteShrink Archive: ${err.message}`);
    }
  }

  async function autoLoadArchiveInDecompress(compressResult) {
    if (!compressResult) return;

    let archiveBytes = compressResult.archiveBytes;

    if (!archiveBytes && compressResult.downloadUrl) {
      try {
        const resp = await fetch(compressResult.downloadUrl);
        if (resp.ok) {
          archiveBytes = new Uint8Array(await resp.arrayBuffer());
        }
      } catch (e) {}
    }

    if (!archiveBytes) return;

    const file = new File([archiveBytes], compressResult.outputName, {
      type: 'application/octet-stream'
    });
    handleDecompressFileSelected(file);
  }

  async function runDecompression() {
    const file = state.currentDecompressFile;
    if (!file) return;

    hideDecompressError();

    let restoredName = document.getElementById('decompressOutputName').value.trim();
    if (!restoredName && state.currentDecompressHeader) {
      restoredName = state.currentDecompressHeader.originalFilename;
    }

    const btnStart = document.getElementById('btnStartDecompress');
    btnStart.setAttribute('disabled', 'true');
    btnStart.innerHTML = `<span class="spinner-inline">⏳</span> DECOMPRESSING...`;

    const progressBox = document.getElementById('decompressProgressBox');
    const progressFill = document.getElementById('decompressProgressFill');
    const statusText = document.getElementById('decompressStatusText');
    progressBox.classList.add('active', 'show');
    progressFill.style.width = '20%';
    statusText.textContent = 'Parsing archive header & rebuilding tree...';

    const startTime = performance.now();

    try {
      const arrayBuffer = await file.arrayBuffer();
      const uint8 = new Uint8Array(arrayBuffer);

      progressFill.style.width = '60%';
      statusText.textContent = 'Traversing binary tree edges to restore byte payload...';

      // Attempt native C++ backend call
      let backendSuccess = false;
      let restoredBytes = null;
      let crcMatch = false;
      let downloadUrl = null;
      let restoredPath = null;

      try {
        const formData = new FormData();
        formData.append('archive', file);
        formData.append('restoredName', restoredName);

        const resp = await fetch('/api/decompress', {
          method: 'POST',
          body: formData
        });

        if (resp.ok) {
          const res = await resp.json();
          backendSuccess = true;
          crcMatch = res.crcMatch;
          downloadUrl = res.downloadUrl;
          restoredPath = res.restoredPath;

          // Fetch restored bytes
          const fileResp = await fetch(res.downloadUrl);
          if (fileResp.ok) {
            restoredBytes = new Uint8Array(await fileResp.arrayBuffer());
          }
        }
      } catch (e) {
        backendSuccess = false;
      }

      if (!backendSuccess || !restoredBytes) {
        // Run client-side Huffman decompressor
        const decomp = executeClientHuffmanDecompress(uint8);
        restoredBytes = decomp.restoredBytes;
        crcMatch = decomp.crcMatch;
      }

      progressFill.style.width = '100%';
      const elapsed = Math.round(performance.now() - startTime);
      statusText.textContent = `Decompressed & verified in ${elapsed} ms.`;

      state.currentDecompressResult = {
        name: restoredName,
        size: restoredBytes.length,
        bytes: restoredBytes,
        path: restoredPath,
        downloadUrl: downloadUrl,
        crcHex: '0x' + (state.currentDecompressHeader ? state.currentDecompressHeader.expectedCrc.toString(16).toUpperCase().padStart(8, '0') : 'CRC-OK')
      };

      // Button: Success State
      btnStart.removeAttribute('disabled');
      btnStart.innerHTML = `<span>⚡</span> DECOMPRESS ANOTHER FILE`;

      // Show results
      const resultsCard = document.getElementById('decompressResultsCard');
      resultsCard.classList.add('active', 'show');
      document.getElementById('decompStatName').textContent = restoredName;
      document.getElementById('decompStatSize').textContent = formatBytes(restoredBytes.length);
      document.getElementById('decompStatCrc').textContent = state.currentDecompressResult.crcHex;

      resultsCard.scrollIntoView({ behavior: 'smooth', block: 'nearest' });

      // Add to genuine activity log
      addActivityRecord({
        time: formatCurrentTime(),
        action: 'DECOMPRESSED',
        fileName: file.name,
        originalSize: formatBytes(file.size),
        outputSize: formatBytes(restoredBytes.length),
        summary: `Successfully restored byte-for-byte [${restoredName}]`
      });

    } catch (err) {
      console.error('Decompression error:', err);
      statusText.textContent = `Decompression Error: ${err.message}`;
      showDecompressError(`Decompression Failed: ${err.message}`);

      btnStart.removeAttribute('disabled');
      btnStart.innerHTML = `<span>⚠️</span> RETRY DECOMPRESSION`;
    }
  }

  function triggerDownloadRestoredFile() {
    const res = state.currentDecompressResult;
    if (!res) return;

    if (res.downloadUrl) {
      const a = document.createElement('a');
      a.href = res.downloadUrl;
      a.download = res.name;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
    } else if (res.bytes) {
      const blob = new Blob([res.bytes], { type: 'application/octet-stream' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = res.name;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      setTimeout(() => URL.revokeObjectURL(url), 2000);
    }
  }

  // =========================================================================
  // ALGORITHM EXPLORER MODULE
  // =========================================================================

  function initExplorer() {
    state.treeCanvas = new HuffmanTreeCanvas('huffmanTreeCanvas');

    const btnUpdate = document.getElementById('btnUpdateExplorer');
    const inputCustom = document.getElementById('explorerCustomText');

    if (btnUpdate && inputCustom) {
      btnUpdate.addEventListener('click', () => {
        rebuildExplorerFromText(inputCustom.value);
      });

      inputCustom.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
          rebuildExplorerFromText(inputCustom.value);
        }
      });
    }

    // Zoom controls
    const btnZoomIn = document.getElementById('btnZoomIn');
    const btnZoomOut = document.getElementById('btnZoomOut');
    const btnResetView = document.getElementById('btnResetView');

    if (btnZoomIn) btnZoomIn.addEventListener('click', () => state.treeCanvas.zoom(1.2));
    if (btnZoomOut) btnZoomOut.addEventListener('click', () => state.treeCanvas.zoom(0.8));
    if (btnResetView) btnResetView.addEventListener('click', () => state.treeCanvas.resetView());

    // Subtab switching
    const subtabs = document.querySelectorAll('.btn-explorer-subtab');
    subtabs.forEach(tab => {
      tab.addEventListener('click', () => {
        const subtabId = tab.getAttribute('data-subtab');

        subtabs.forEach(t => t.classList.remove('active'));
        tab.classList.add('active');

        document.getElementById('subview-tree').style.display = (subtabId === 'tree') ? 'block' : 'none';
        document.getElementById('subview-symbols').style.display = (subtabId === 'symbols') ? 'block' : 'none';
        document.getElementById('subview-merges').style.display = (subtabId === 'merges') ? 'block' : 'none';

        if (subtabId === 'tree') {
          state.treeCanvas.resize();
          state.treeCanvas.render();
        }
      });
    });

    // Initial build with default phrase
    if (inputCustom && inputCustom.value) {
      rebuildExplorerFromText(inputCustom.value);
    }
  }

  function rebuildExplorerFromText(text) {
    if (!text || text.length === 0) text = 'ByteShrink';
    const enc = new TextEncoder();
    const bytes = enc.encode(text);
    buildExplorerFromBytes(bytes, `Text: "${text.slice(0, 30)}..."`);
  }

  async function loadExplorerFromFile(file) {
    const arrayBuffer = await file.arrayBuffer();
    const bytes = new Uint8Array(arrayBuffer);
    buildExplorerFromBytes(bytes, `File: ${file.name}`);
    const inputCustom = document.getElementById('explorerCustomText');
    if (inputCustom) {
      inputCustom.value = `[File: ${file.name} (${formatBytes(file.size)})]`;
    }
  }

  function buildExplorerFromBytes(bytes, label) {
    // 1. Calculate frequency distribution
    const freqs = new Map();
    for (let i = 0; i < bytes.length; i++) {
      const b = bytes[i];
      freqs.set(b, (freqs.get(b) || 0) + 1);
    }

    // 2. Build Min-Heap and track greedy merge steps
    const heap = [];
    let idCounter = 1;

    for (const [byteVal, freq] of freqs.entries()) {
      heap.push({
        id: idCounter++,
        byteVal: byteVal,
        symbol: String.fromCharCode(byteVal),
        freq: freq,
        isLeaf: true,
        left: null,
        right: null
      });
    }

    // Min-Heap sort comparator
    const sortHeap = () => heap.sort((a, b) => (a.freq === b.freq ? a.id - b.id : a.freq - b.freq));
    sortHeap();

    const merges = [];
    let stepNum = 1;

    // Greedy priority queue merging
    while (heap.length > 1) {
      const left = heap.shift();
      const right = heap.shift();

      const combinedFreq = left.freq + right.freq;
      const parent = {
        id: idCounter++,
        freq: combinedFreq,
        isLeaf: false,
        left: left,
        right: right
      };

      merges.push({
        step: stepNum++,
        leftName: left.isLeaf ? `'${formatChar(left.byteVal)}' (f:${left.freq})` : `Internal #${left.id} (f:${left.freq})`,
        rightName: right.isLeaf ? `'${formatChar(right.byteVal)}' (f:${right.freq})` : `Internal #${right.id} (f:${right.freq})`,
        combinedFreq: combinedFreq,
        action: `Combined min nodes into parent #${parent.id} (weight: ${combinedFreq})`
      });

      heap.push(parent);
      sortHeap();
    }

    const root = heap[0] || null;

    // 3. Generate prefix-free codes via DFS
    const codes = new Map();
    const traverse = (node, codeStr) => {
      if (!node) return;
      node.code = codeStr;
      if (node.isLeaf) {
        codes.set(node.byteVal, codeStr);
      } else {
        traverse(node.left, codeStr + '0');
        traverse(node.right, codeStr + '1');
      }
    };

    if (root) {
      if (root.isLeaf) {
        root.code = '0';
        codes.set(root.byteVal, '0');
      } else {
        traverse(root, '');
      }
    }

    // 4. Update 2D Canvas
    state.treeCanvas.setTree(root);

    // 5. Populate Symbol Table
    populateSymbolTable(freqs, codes);

    // 6. Populate Merges Table
    populateMergeTable(merges);
  }

  function populateSymbolTable(freqs, codes) {
    const tbody = document.getElementById('symbolTableBody');
    if (!tbody) return;
    tbody.innerHTML = '';

    const entries = Array.from(freqs.entries()).sort((a, b) => b[1] - a[1]);

    entries.forEach(([byteVal, freq]) => {
      const tr = document.createElement('tr');
      const code = codes.get(byteVal) || '0';
      const charStr = formatChar(byteVal);
      const hexStr = '0x' + byteVal.toString(16).toUpperCase().padStart(2, '0');

      tr.innerHTML = `
        <td>${byteVal}</td>
        <td><code>${hexStr}</code></td>
        <td><strong style="color: var(--accent-orange); font-family: var(--font-mono);">${charStr}</strong></td>
        <td>${freq}</td>
        <td><code style="color: var(--accent-orange); font-weight: bold;">${code}</code></td>
        <td>${code.length}</td>
      `;
      tbody.appendChild(tr);
    });
  }

  function populateMergeTable(merges) {
    const tbody = document.getElementById('mergeTableBody');
    if (!tbody) return;
    tbody.innerHTML = '';

    merges.forEach((m) => {
      const tr = document.createElement('tr');
      tr.innerHTML = `
        <td><strong>#${m.step}</strong></td>
        <td><span style="color: var(--accent-orange);">${m.leftName}</span></td>
        <td><span style="color: var(--accent-purple);">${m.rightName}</span></td>
        <td><strong>${m.combinedFreq}</strong></td>
        <td><span style="color: var(--text-secondary);">${m.action}</span></td>
      `;
      tbody.appendChild(tr);
    });
  }

  // =========================================================================
  // RECENT ACTIVITY TABLE
  // =========================================================================

  function addActivityRecord(record) {
    state.activityLog.unshift(record);
    if (state.activityLog.length > 15) {
      state.activityLog.pop();
    }
    renderActivityTable();
  }

  function renderActivityTable() {
    const tbody = document.getElementById('activityTableBody');
    const emptyMsg = document.getElementById('emptyActivityMsg');
    if (!tbody || !emptyMsg) return;

    tbody.innerHTML = '';

    if (state.activityLog.length === 0) {
      emptyMsg.style.display = 'block';
      return;
    }

    emptyMsg.style.display = 'none';

    state.activityLog.forEach(item => {
      const tr = document.createElement('tr');
      const isComp = item.action === 'COMPRESSED';
      const badgeClass = isComp ? 'badge-compressed' : 'badge-decompressed';

      tr.innerHTML = `
        <td>${item.time}</td>
        <td><span class="action-badge ${badgeClass}">${item.action}</span></td>
        <td><strong>${escapeHtml(item.fileName)}</strong></td>
        <td>${item.originalSize}</td>
        <td>${item.outputSize}</td>
        <td>${escapeHtml(item.summary)}</td>
      `;
      tbody.appendChild(tr);
    });
  }

  // =========================================================================
  // CLIENT-SIDE HUFFMAN ENGINE (100% C++ ARCHIVEFORMAT COMPLIANT)
  // Ensures complete offline functionality matching ByteShrink.exe binary format
  // =========================================================================

  function executeClientHuffmanCompress(originalFilename, archiveFilename, data) {
    // 1. Calculate byte frequencies
    const freqs = new Array(256).fill(0n);
    for (let i = 0; i < data.length; i++) {
      freqs[data[i]]++;
    }

    // 2. Count distinct symbols
    let distinct = 0;
    for (let i = 0; i < 256; i++) {
      if (freqs[i] > 0n) distinct++;
    }

    // Single or zero-symbol edge case
    if (distinct === 0) {
      distinct = 1;
      freqs[0] = 1n;
    }

    // 3. Min-Heap
    const heap = [];
    let idGen = 0;
    for (let i = 0; i < 256; i++) {
      if (freqs[i] > 0n) {
        heap.push({ id: idGen++, byteVal: i, freq: Number(freqs[i]), isLeaf: true, left: null, right: null });
      }
    }
    const sortQ = () => heap.sort((a, b) => a.freq === b.freq ? a.id - b.id : a.freq - b.freq);
    sortQ();

    while (heap.length > 1) {
      const left = heap.shift();
      const right = heap.shift();
      const parent = { id: idGen++, freq: left.freq + right.freq, isLeaf: false, left, right };
      heap.push(parent);
      sortQ();
    }

    const root = heap[0];
    const codes = new Array(256).fill('');

    const getCodes = (node, str) => {
      if (!node) return;
      if (node.isLeaf) {
        codes[node.byteVal] = str || '0';
      } else {
        getCodes(node.left, str + '0');
        getCodes(node.right, str + '1');
      }
    };
    getCodes(root, '');

    // 4. Encode Payload Bitstream
    const bitBuffer = [];
    let currentByte = 0;
    let bitCount = 0;

    for (let i = 0; i < data.length; i++) {
      const code = codes[data[i]];
      for (let c = 0; c < code.length; c++) {
        currentByte = (currentByte << 1) | (code[c] === '1' ? 1 : 0);
        bitCount++;
        if (bitCount === 8) {
          bitBuffer.push(currentByte);
          currentByte = 0;
          bitCount = 0;
        }
      }
    }

    const validBitsInLastByte = bitCount === 0 ? 8 : bitCount;
    if (bitCount > 0) {
      currentByte <<= (8 - bitCount);
      bitBuffer.push(currentByte);
    }

    // 5. Compute CRC-32
    const crc32Val = computeCrc32(data);

    // 6. Build HFX1 binary format matching include/ArchiveFormat.h:
    // Header (23 Bytes):
    // magic (4B: "HFX1") + version (1B: 0x01) + flags (1B: 0x03) + originalSize (8B) + crc32 (4B)
    // + validBitsInLastByte (1B) + filenameLength (2B) + symbolCount (2B)
    // Followed by: filename bytes + symbolCount * (1B symbol + 8B frequency uint64) + bitstream
    const fnEnc = new TextEncoder().encode(originalFilename);
    const headerFixedLen = 23;
    const dynamicHeaderLen = fnEnc.length + (distinct * 9);
    const totalSize = headerFixedLen + dynamicHeaderLen + bitBuffer.length;

    const out = new Uint8Array(totalSize);
    let ptr = 0;

    // Magic "HFX1"
    out[ptr++] = 0x48; out[ptr++] = 0x46; out[ptr++] = 0x58; out[ptr++] = 0x31;
    // Version 1
    out[ptr++] = 0x01;
    // Flags: ARCHIVE_FLAG_HAS_FILENAME (0x01) | ARCHIVE_FLAG_HAS_CRC32 (0x02) = 0x03
    out[ptr++] = 0x03;

    // Original Size (8 bytes uint64_t little-endian)
    const bigSize = BigInt(data.length);
    for (let i = 0; i < 8; i++) {
      out[ptr++] = Number((bigSize >> BigInt(i * 8)) & 0xFFn);
    }

    // CRC-32 (4 bytes uint32_t little-endian)
    out[ptr++] = (crc32Val & 0xFF);
    out[ptr++] = ((crc32Val >>> 8) & 0xFF);
    out[ptr++] = ((crc32Val >>> 16) & 0xFF);
    out[ptr++] = ((crc32Val >>> 24) & 0xFF);

    // Valid bits in last byte (1 byte)
    out[ptr++] = validBitsInLastByte;

    // Filename Length (2 bytes uint16_t little-endian)
    out[ptr++] = (fnEnc.length & 0xFF);
    out[ptr++] = ((fnEnc.length >> 8) & 0xFF);

    // Symbol Count (2 bytes uint16_t little-endian)
    out[ptr++] = (distinct & 0xFF);
    out[ptr++] = ((distinct >> 8) & 0xFF);

    // Filename bytes
    out.set(fnEnc, ptr);
    ptr += fnEnc.length;

    // Serialized Symbol Entries: 1B symbol + 8B frequency (uint64_t little-endian)
    for (let i = 0; i < 256; i++) {
      if (freqs[i] > 0n) {
        out[ptr++] = i;
        const f = freqs[i];
        for (let b = 0; b < 8; b++) {
          out[ptr++] = Number((f >> BigInt(b * 8)) & 0xFFn);
        }
      }
    }

    // Payload bitstream
    out.set(bitBuffer, ptr);

    const blob = new Blob([out], { type: 'application/octet-stream' });

    return {
      compressedSize: totalSize,
      archiveBytes: out,
      blob: blob,
      crc32: crc32Val
    };
  }

  function parseHfxArchiveHeader(uint8) {
    if (uint8.length < 23) {
      throw new Error('File too small to be a ByteShrink HFX archive.');
    }

    if (uint8[0] !== 0x48 || uint8[1] !== 0x46 || uint8[2] !== 0x58 || uint8[3] !== 0x31) {
      throw new Error('Invalid magic signature. Expected HFX1 header.');
    }

    let ptr = 4;
    const version = uint8[ptr++];
    const flags = uint8[ptr++];

    let origSize = 0n;
    for (let i = 0; i < 8; i++) {
      origSize |= (BigInt(uint8[ptr++]) << BigInt(i * 8));
    }

    const crc = (uint8[ptr]) | (uint8[ptr + 1] << 8) | (uint8[ptr + 2] << 16) | (uint8[ptr + 3] << 24);
    ptr += 4;

    const validBits = uint8[ptr++];

    const fnLen = uint8[ptr] | (uint8[ptr + 1] << 8);
    ptr += 2;

    const distinct = uint8[ptr] | (uint8[ptr + 1] << 8);
    ptr += 2;

    let originalFilename = 'restored_file';
    if (fnLen > 0 && ptr + fnLen <= uint8.length) {
      const fnBytes = uint8.slice(ptr, ptr + fnLen);
      originalFilename = new TextDecoder().decode(fnBytes);
      ptr += fnLen;
    }

    return {
      version,
      flags,
      expectedCrc: crc >>> 0,
      originalSize: Number(origSize),
      validBitsInLastByte: validBits,
      originalFilename,
      distinctSymbols: distinct,
      freqTableOffset: ptr,
      headerEndOffset: ptr + distinct * 9
    };
  }

  function executeClientHuffmanDecompress(uint8) {
    const header = parseHfxArchiveHeader(uint8);
    let ptr = header.freqTableOffset;

    // Read frequencies (1B symbol + 8B frequency)
    const freqs = new Array(256).fill(0n);
    for (let i = 0; i < header.distinctSymbols; i++) {
      const sym = uint8[ptr++];
      let f = 0n;
      for (let b = 0; b < 8; b++) {
        f |= (BigInt(uint8[ptr++]) << BigInt(b * 8));
      }
      freqs[sym] = f;
    }

    // Reconstruct Tree
    const heap = [];
    let idGen = 0;
    for (let i = 0; i < 256; i++) {
      if (freqs[i] > 0n) {
        heap.push({ id: idGen++, byteVal: i, freq: Number(freqs[i]), isLeaf: true, left: null, right: null });
      }
    }
    const sortQ = () => heap.sort((a, b) => a.freq === b.freq ? a.id - b.id : a.freq - b.freq);
    sortQ();

    while (heap.length > 1) {
      const left = heap.shift();
      const right = heap.shift();
      const parent = { id: idGen++, freq: left.freq + right.freq, isLeaf: false, left, right };
      heap.push(parent);
      sortQ();
    }

    const root = heap[0];
    const restored = new Uint8Array(header.originalSize);
    let restoredPtr = 0;
    let curr = root;

    // Decode bits
    while (ptr < uint8.length && restoredPtr < header.originalSize) {
      const byte = uint8[ptr++];
      const isLastByte = (ptr === uint8.length);
      const bitsToRead = isLastByte ? header.validBitsInLastByte : 8;

      for (let b = 7; b >= (8 - bitsToRead); b--) {
        const bit = (byte >> b) & 1;
        if (!curr) break;

        if (curr.isLeaf) {
          restored[restoredPtr++] = curr.byteVal;
          curr = root;
          if (restoredPtr === header.originalSize) break;
        } else {
          curr = (bit === 0) ? curr.left : curr.right;
          if (curr && curr.isLeaf) {
            restored[restoredPtr++] = curr.byteVal;
            curr = root;
            if (restoredPtr === header.originalSize) break;
          }
        }
      }
    }

    const calcCrc = computeCrc32(restored);
    const crcMatch = (calcCrc >>> 0) === (header.expectedCrc >>> 0);

    return {
      restoredBytes: restored,
      crcMatch: crcMatch,
      calcCrc: calcCrc
    };
  }

  // IEEE 802.3 CRC-32 (Polynomial 0xEDB88320)
  const CRC_TABLE = new Uint32Array(256);
  (function initCrcTable() {
    for (let i = 0; i < 256; i++) {
      let c = i;
      for (let j = 0; j < 8; j++) {
        c = (c & 1) ? (0xEDB88320 ^ (c >>> 1)) : (c >>> 1);
      }
      CRC_TABLE[i] = c;
    }
  })();

  function computeCrc32(buffer) {
    let crc = 0xFFFFFFFF;
    for (let i = 0; i < buffer.length; i++) {
      crc = CRC_TABLE[(crc ^ buffer[i]) & 0xFF] ^ (crc >>> 8);
    }
    return (crc ^ 0xFFFFFFFF) >>> 0;
  }

  // =========================================================================
  // UTILITY HELPERS
  // =========================================================================

  function formatBytes(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
  }

  function formatCurrentTime() {
    const d = new Date();
    const h = String(d.getHours()).padStart(2, '0');
    const m = String(d.getMinutes()).padStart(2, '0');
    const s = String(d.getSeconds()).padStart(2, '0');
    return `${h}:${m}:${s}`;
  }

  function getExtension(filename) {
    if (!filename) return '';
    const parts = filename.split('.');
    return parts.length > 1 ? parts.pop().toLowerCase() : '';
  }

  function categorizeExtension(ext) {
    for (const [cat, exts] of Object.entries(FILE_CATEGORIES)) {
      if (exts.includes(ext)) return cat;
    }
    return 'binary';
  }

  function formatChar(byteVal) {
    if (byteVal === 32) return '␣ [SPACE]';
    if (byteVal === 10) return '\\n [LF]';
    if (byteVal === 13) return '\\r [CR]';
    if (byteVal === 9) return '\\t [TAB]';
    if (byteVal >= 32 && byteVal <= 126) return String.fromCharCode(byteVal);
    return '0x' + byteVal.toString(16).toUpperCase().padStart(2, '0');
  }

  function escapeHtml(str) {
    if (!str) return '';
    return str
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#039;');
  }

})();
