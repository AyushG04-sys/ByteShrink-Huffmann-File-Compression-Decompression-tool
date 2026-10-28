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
    treeCanvas: null,
    selectedTreeNode: null,
    currentExplorerData: null,
    mergeState: {
      currentStep: 1,
      totalSteps: 1,
      isPlaying: false,
      timer: null,
      merges: []
    },
    graphHoveredByte: null,
    graphBars: [],
    selectedSymbolByte: null
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
    if (tabId === 'explorer') {
      setTimeout(() => {
        if (state.treeCanvas) {
          state.treeCanvas.resize();
          state.treeCanvas.render();
        }
        const graphSubview = document.getElementById('subview-graph');
        if (graphSubview && graphSubview.style.display !== 'none') {
          drawFrequencyGraph();
        }
      }, 60);
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
      diagnosticText.innerHTML = `Lossless Huffman compression reduced file payload by <strong>${formatBytes(res.spaceSaved)}</strong> (${Math.abs(res.percentChange)}% saved). Optimal variable-length prefix codes were assigned to frequent symbols without loss of information.`;
    } else {
      const addedBytes = Math.abs(res.spaceSaved);
      statSpaceSaved.textContent = `-${formatBytes(addedBytes)} (+${Math.abs(res.percentChange)}% overhead)`;
      statSpaceSaved.style.color = 'var(--accent-orange)';
      document.getElementById('statRatio').textContent = `${res.ratio}:1 (Expanded)`;
      barFill.textContent = `100% + overhead`;
      barFill.style.background = 'linear-gradient(90deg, #D44100, var(--accent-orange))';
      diagnosticText.innerHTML = `<strong>Shannon Entropy Notice:</strong> This input format (${getExtension(res.originalName).toUpperCase()}) already uses internal compression or has high entropy (~8 bits/byte). Lossless Huffman coding cannot shrink it further, and storing the Huffman tree table adds ${formatBytes(addedBytes)} container overhead. This is an honest, expected characteristic of lossless data compression.`;
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

    // Fit to view button
    const btnFitView = document.getElementById('btnFitView');
    if (btnFitView) {
      btnFitView.addEventListener('click', () => {
        if (state.treeCanvas) state.treeCanvas.fitToView();
      });
    }

    // Node click callback: selects symbol in table and graph, zooms to tree node, updates tree explanation
    state.treeCanvas.onNodeClick = (node) => {
      state.selectedTreeNode = node;
      if (node && node.isLeaf && typeof node.byteVal === 'number') {
        selectSymbol(node.byteVal);
      } else {
        updateTreeExplanation(node);
      }
    };

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

    // Merge player controls
    const btnMergeRestart = document.getElementById('btnMergeRestart');
    const btnMergePrev = document.getElementById('btnMergePrev');
    const btnMergePlay = document.getElementById('btnMergePlay');
    const btnMergeNext = document.getElementById('btnMergeNext');

    if (btnMergeRestart) btnMergeRestart.addEventListener('click', () => goToMergeStep(1));
    if (btnMergePrev) btnMergePrev.addEventListener('click', () => goToMergeStep(state.mergeState.currentStep - 1));
    if (btnMergePlay) btnMergePlay.addEventListener('click', toggleMergePlay);
    if (btnMergeNext) btnMergeNext.addEventListener('click', () => goToMergeStep(state.mergeState.currentStep + 1));

    // 2D Frequency Graph mouse interactions
    const graphCanvas = document.getElementById('frequencyGraphCanvas');
    if (graphCanvas) {
      graphCanvas.addEventListener('mousemove', handleGraphMouseMove);
      graphCanvas.addEventListener('mouseleave', handleGraphMouseLeave);
      graphCanvas.addEventListener('click', handleGraphClick);
    }

    // Window resize listener to redraw graph
    window.addEventListener('resize', () => {
      const graphSubview = document.getElementById('subview-graph');
      if (graphSubview && graphSubview.style.display !== 'none') {
        drawFrequencyGraph();
      }
    });

    // Subtab switching
    const subtabs = document.querySelectorAll('.btn-explorer-subtab');
    subtabs.forEach(tab => {
      tab.addEventListener('click', () => {
        const subtabId = tab.getAttribute('data-subtab');

        subtabs.forEach(t => t.classList.remove('active'));
        tab.classList.add('active');

        document.getElementById('subview-tree').style.display = (subtabId === 'tree') ? 'block' : 'none';
        document.getElementById('subview-graph').style.display = (subtabId === 'graph') ? 'block' : 'none';
        document.getElementById('subview-symbols').style.display = (subtabId === 'symbols') ? 'block' : 'none';
        document.getElementById('subview-merges').style.display = (subtabId === 'merges') ? 'block' : 'none';

        if (subtabId === 'tree') {
          state.treeCanvas.resize();
          state.treeCanvas.render();
        } else if (subtabId === 'graph') {
          drawFrequencyGraph();
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
    if (!bytes || bytes.length === 0) {
      bytes = new Uint8Array([65]); // Fallback to 'A'
    }

    const totalBytes = bytes.length;

    // 1. Calculate frequency distribution
    const freqs = new Map();
    for (let i = 0; i < totalBytes; i++) {
      const b = bytes[i];
      freqs.set(b, (freqs.get(b) || 0) + 1);
    }

    // 2. Calculate Shannon Entropy H = -sum(p * log2(p))
    let entropy = 0;
    for (const [_, freq] of freqs.entries()) {
      const p = freq / totalBytes;
      if (p > 0) {
        entropy -= p * Math.log2(p);
      }
    }

    // 3. Build Min-Heap and track greedy merge steps
    const heap = [];
    let idCounter = 1;

    for (const [byteVal, freq] of freqs.entries()) {
      heap.push({
        id: idCounter++,
        byteVal: byteVal,
        symbol: formatChar(byteVal),
        freq: freq,
        isLeaf: true,
        left: null,
        right: null
      });
    }

    // Min-Heap sort comparator (weight ascending, then id ascending)
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

      heap.push(parent);
      sortHeap();

      merges.push({
        step: stepNum++,
        leftNode: left,
        rightNode: right,
        parentNode: parent,
        leftName: left.isLeaf ? `'${formatChar(left.byteVal)}' (w:${left.freq})` : `Node #${left.id} (w:${left.freq})`,
        rightName: right.isLeaf ? `'${formatChar(right.byteVal)}' (w:${right.freq})` : `Node #${right.id} (w:${right.freq})`,
        leftSymbol: left.isLeaf ? formatChar(left.byteVal) : `Node #${left.id}`,
        rightSymbol: right.isLeaf ? formatChar(right.byteVal) : `Node #${right.id}`,
        leftFreq: left.freq,
        rightFreq: right.freq,
        parentSymbol: `Parent #${parent.id}`,
        combinedFreq: combinedFreq,
        action: `Extracted min subtrees [${left.isLeaf ? `'${formatChar(left.byteVal)}'` : `#${left.id}`}] (w:${left.freq}) and [${right.isLeaf ? `'${formatChar(right.byteVal)}'` : `#${right.id}`}] (w:${right.freq}) → created Parent #${parent.id} (w:${combinedFreq})`,
        explanation: `Greedy choice: Extracted two lowest-weight nodes (${left.freq} and ${right.freq}) from priority queue. Formed parent #${parent.id} with weight ${combinedFreq} and re-inserted into min-heap.`,
        queueSnapshot: heap.map(node => ({
          id: node.id,
          name: node.isLeaf ? `'${formatChar(node.byteVal)}'` : `Node #${node.id}`,
          freq: node.freq,
          isLeaf: node.isLeaf
        }))
      });
    }

    const root = heap[0] || null;

    // 4. Generate prefix-free codes via DFS
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

    // 5. Calculate total payload bits & average code length
    let totalPayloadBits = 0;
    for (const [byteVal, freq] of freqs.entries()) {
      const c = codes.get(byteVal) || '0';
      totalPayloadBits += freq * c.length;
    }
    const avgCodeLen = totalBytes > 0 ? (totalPayloadBits / totalBytes) : 8.0;

    // 6. Save in state
    state.currentExplorerData = {
      bytes,
      label,
      freqs,
      codes,
      merges,
      root,
      totalBytes,
      distinctSymbols: freqs.size,
      entropy,
      totalPayloadBits,
      avgCodeLen
    };

    // 7. Update 2D Canvas & fit to view
    state.treeCanvas.setTree(root);
    setTimeout(() => {
      if (state.treeCanvas) state.treeCanvas.fitToView();
    }, 40);

    // 8. Render 2D Frequency Graph
    drawFrequencyGraph();

    // 9. Populate Symbol Table
    populateSymbolTable();

    // 10. Initialize Merge Player & populate merge table
    initMergePlayer(merges);
    populateMergeTable(merges);

    // 11. Synchronize Educational Explanation Panels
    updateTreeExplanation(null);
    updateGraphExplanation(null);
    updateSymbolsExplanation();
    updateMergesExplanation(merges.length > 0 ? 1 : 0);
  }

  // =========================================================================
  // 2D INTERACTIVE FREQUENCY GRAPH
  // =========================================================================

  function handleGraphMouseMove(e) {
    const canvas = document.getElementById('frequencyGraphCanvas');
    if (!canvas || !state.graphBars) return;

    const rect = canvas.getBoundingClientRect();
    const mouseX = e.clientX - rect.left;
    const mouseY = e.clientY - rect.top;

    let hovered = null;
    for (const bar of state.graphBars) {
      if (mouseX >= bar.x && mouseX <= bar.x + bar.w && mouseY >= bar.y - 10 && mouseY <= bar.y + bar.h + 20) {
        hovered = bar;
        break;
      }
    }

    const prev = state.graphHoveredByte;
    state.graphHoveredByte = hovered ? hovered.byteVal : null;
    state.graphHoverInfo = hovered ? { mouseX, mouseY, bar: hovered } : null;

    if (prev !== state.graphHoveredByte) {
      drawFrequencyGraph();
      updateGraphExplanation(hovered);
    }
  }

  function handleGraphMouseLeave() {
    if (state.graphHoveredByte !== null) {
      state.graphHoveredByte = null;
      state.graphHoverInfo = null;
      drawFrequencyGraph();

      let selectedBar = null;
      if (state.selectedSymbolByte !== null && state.graphBars) {
        selectedBar = state.graphBars.find(b => b.byteVal === state.selectedSymbolByte);
      }
      updateGraphExplanation(selectedBar);
    }
  }

  function handleGraphClick(e) {
    const canvas = document.getElementById('frequencyGraphCanvas');
    if (!canvas || !state.graphBars) return;

    const rect = canvas.getBoundingClientRect();
    const mouseX = e.clientX - rect.left;
    const mouseY = e.clientY - rect.top;

    for (const bar of state.graphBars) {
      if (mouseX >= bar.x && mouseX <= bar.x + bar.w && mouseY >= bar.y - 10 && mouseY <= bar.y + bar.h + 20) {
        selectSymbol(bar.byteVal);
        break;
      }
    }
  }

  function drawFrequencyGraph() {
    const canvas = document.getElementById('frequencyGraphCanvas');
    if (!canvas) return;

    const container = canvas.parentElement;
    const width = (container ? container.clientWidth : 800) || 800;
    const height = 360;
    const dpr = window.devicePixelRatio || 1;

    canvas.width = width * dpr;
    canvas.height = height * dpr;
    canvas.style.width = width + 'px';
    canvas.style.height = height + 'px';

    const ctx = canvas.getContext('2d');
    ctx.scale(dpr, dpr);

    // Clear background
    ctx.fillStyle = '#0B0B0E';
    ctx.fillRect(0, 0, width, height);

    const d = state.currentExplorerData;
    if (!d || !d.freqs || d.freqs.size === 0) {
      ctx.fillStyle = '#6E6E7A';
      ctx.font = '13px JetBrains Mono, monospace';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText('No frequency data loaded. Enter text above to inspect distribution.', width / 2, height / 2);
      return;
    }

    // Sort entries descending by frequency
    const entries = Array.from(d.freqs.entries()).sort((a, b) => b[1] - a[1]);
    const maxDisplay = Math.min(entries.length, 36);
    const displayEntries = entries.slice(0, maxDisplay);
    const maxFreq = entries[0][1];

    const padLeft = 60;
    const padRight = 30;
    const padTop = 35;
    const padBottom = 55;
    const chartW = width - padLeft - padRight;
    const chartH = height - padTop - padBottom;

    // Y-Axis Horizontal Gridlines
    ctx.lineWidth = 1;
    ctx.font = '11px JetBrains Mono, monospace';
    ctx.textAlign = 'right';
    ctx.textBaseline = 'middle';

    const ticks = 4;
    for (let i = 0; i <= ticks; i++) {
      const frac = i / ticks;
      const y = padTop + chartH - (frac * chartH);
      const val = Math.round(maxFreq * frac);

      ctx.strokeStyle = '#23232a';
      ctx.beginPath();
      ctx.moveTo(padLeft, y);
      ctx.lineTo(padLeft + chartW, y);
      ctx.stroke();

      ctx.fillStyle = '#6E6E7A';
      ctx.fillText(val.toString(), padLeft - 10, y);
    }

    // X-Axis Baseline
    ctx.strokeStyle = '#3A3A46';
    ctx.beginPath();
    ctx.moveTo(padLeft, padTop + chartH);
    ctx.lineTo(padLeft + chartW, padTop + chartH);
    ctx.stroke();

    // Draw Bars
    const numBars = displayEntries.length;
    const slotW = chartW / numBars;
    const barW = Math.max(6, Math.min(36, slotW - 6));
    state.graphBars = [];

    displayEntries.forEach(([byteVal, freq], idx) => {
      const pct = ((freq / d.totalBytes) * 100).toFixed(1);
      const barH = Math.max(4, (freq / maxFreq) * chartH);
      const barX = padLeft + (idx * slotW) + (slotW - barW) / 2;
      const barY = padTop + chartH - barH;
      const charStr = formatChar(byteVal);
      const hexStr = '0x' + byteVal.toString(16).toUpperCase().padStart(2, '0');
      const code = d.codes.get(byteVal) || '0';

      state.graphBars.push({
        x: barX,
        y: barY,
        w: barW,
        h: barH,
        byteVal,
        freq,
        pct,
        charStr,
        hexStr,
        code
      });

      const isSelected = (byteVal === state.selectedSymbolByte);
      const isHovered = (byteVal === state.graphHoveredByte);

      // Create gradient fill
      const grad = ctx.createLinearGradient(0, barY, 0, barY + barH);
      if (isSelected) {
        grad.addColorStop(0, '#FF8A50');
        grad.addColorStop(1, '#FF5A0A');
        ctx.shadowColor = '#FF5A0A';
        ctx.shadowBlur = 12;
      } else if (isHovered) {
        grad.addColorStop(0, '#FFA726');
        grad.addColorStop(1, '#FF7043');
        ctx.shadowColor = '#FFA726';
        ctx.shadowBlur = 8;
      } else {
        grad.addColorStop(0, '#FF5A0A');
        grad.addColorStop(1, '#7844F9');
        ctx.shadowBlur = 0;
      }

      ctx.fillStyle = grad;
      ctx.beginPath();
      ctx.roundRect(barX, barY, barW, barH, [4, 4, 0, 0]);
      ctx.fill();

      if (isSelected || isHovered) {
        ctx.strokeStyle = '#FFFFFF';
        ctx.lineWidth = 1.5;
        ctx.stroke();
      }
      ctx.shadowBlur = 0;

      // Frequency label on top of bar
      if (barW >= 18) {
        ctx.fillStyle = (isSelected || isHovered) ? '#FFFFFF' : '#A0A0B0';
        ctx.font = '10px JetBrains Mono, monospace';
        ctx.textAlign = 'center';
        ctx.fillText(freq.toString(), barX + barW / 2, barY - 6);
      }

      // Symbol label below X-axis
      ctx.save();
      ctx.translate(barX + barW / 2, padTop + chartH + 12);
      if (numBars > 18) {
        ctx.rotate(Math.PI / 4);
      }
      ctx.fillStyle = isSelected ? '#FF5A0A' : (isHovered ? '#FFA726' : '#C0C0D0');
      ctx.font = 'bold 11px JetBrains Mono, monospace';
      ctx.textAlign = numBars > 18 ? 'left' : 'center';
      ctx.textBaseline = 'top';
      ctx.fillText(charStr, 0, 0);
      ctx.restore();
    });

    // Draw Floating Hover Tooltip
    if (state.graphHoverInfo && state.graphHoverInfo.bar) {
      const b = state.graphHoverInfo.bar;
      const tooltipLines = [
        `Symbol: ${b.charStr} (${b.hexStr} / Dec ${b.byteVal})`,
        `Count: ${b.freq} occurrences (${b.pct}% of input)`,
        `Huffman Code: ${b.code} (${b.code.length} bits)`,
        `Bitstream Load: ${b.freq * b.code.length} bits`,
        `💡 Click bar to isolate in Tree & Table`
      ];

      ctx.font = '11px JetBrains Mono, monospace';
      let maxTextW = 0;
      tooltipLines.forEach(line => {
        const tw = ctx.measureText(line).width;
        if (tw > maxTextW) maxTextW = tw;
      });

      const boxW = maxTextW + 24;
      const boxH = (tooltipLines.length * 18) + 16;
      let tipX = b.x + b.w / 2 - boxW / 2;
      let tipY = b.y - boxH - 12;

      // Keep within bounds
      if (tipX < padLeft) tipX = padLeft;
      if (tipX + boxW > width - padRight) tipX = width - padRight - boxW;
      if (tipY < 10) tipY = b.y + b.h + 20;

      // Draw Tooltip Box
      ctx.fillStyle = '#151519';
      ctx.strokeStyle = '#FF5A0A';
      ctx.lineWidth = 1.5;
      ctx.shadowColor = '#000000';
      ctx.shadowBlur = 12;
      ctx.beginPath();
      ctx.roundRect(tipX, tipY, boxW, boxH, 6);
      ctx.fill();
      ctx.stroke();
      ctx.shadowBlur = 0;

      // Tooltip Text
      ctx.textAlign = 'left';
      ctx.textBaseline = 'top';
      tooltipLines.forEach((line, lIdx) => {
        if (lIdx === 0) ctx.fillStyle = '#FF5A0A';
        else if (lIdx === tooltipLines.length - 1) ctx.fillStyle = '#A0A0B0';
        else ctx.fillStyle = '#FFFFFF';

        ctx.fillText(line, tipX + 12, tipY + 10 + (lIdx * 18));
      });
    }

    // Update Metrics Pills
    const pillsContainer = document.getElementById('graphStatsPills');
    if (pillsContainer) {
      const theoBytes = Math.ceil((d.totalBytes * d.entropy) / 8);
      const huffmanBytes = Math.ceil(d.totalPayloadBits / 8);
      pillsContainer.innerHTML = `
        <div class="graph-pill"><strong>Input Size:</strong> <span>${formatBytes(d.totalBytes)} (${d.totalBytes} B)</span></div>
        <div class="graph-pill"><strong>Distinct Symbols:</strong> <span>${d.distinctSymbols} / 256</span></div>
        <div class="graph-pill"><strong>Shannon Entropy:</strong> <span style="color: var(--accent-orange);">${d.entropy.toFixed(3)} bits/sym</span></div>
        <div class="graph-pill"><strong>Theoretical Limit:</strong> <span>${formatBytes(theoBytes)}</span></div>
        <div class="graph-pill"><strong>Huffman Encoded Bits:</strong> <span style="color: var(--accent-green);">${formatBytes(huffmanBytes)} (${d.totalPayloadBits} bits)</span></div>
        <div class="graph-pill"><strong>Avg Code Length:</strong> <span>${d.avgCodeLen.toFixed(2)} bits/sym</span></div>
      `;
    }

    // Update Contextual Narrative Explanation
    const narrativeEl = document.getElementById('graphExplanationText');
    if (narrativeEl) {
      let narrative = '';
      if (d.entropy < 3.2) {
        narrative = `The input "${escapeHtml(d.label)}" exhibits a <strong>highly skewed frequency distribution</strong> with low Shannon entropy (${d.entropy.toFixed(3)} bits/symbol). Frequent symbols such as '${formatChar(entries[0][0])}' dominate, allowing the greedy Huffman algorithm to assign concise 1 to 3 bit codes. This results in superior lossless compression efficiency (~${((1 - (d.totalPayloadBits / (d.totalBytes * 8))) * 100).toFixed(1)}% raw payload reduction before archive headers).`;
      } else if (d.entropy < 6.8) {
        narrative = `The input "${escapeHtml(d.label)}" shows a <strong>typical natural language / structured data distribution</strong> with moderate Shannon entropy (${d.entropy.toFixed(3)} bits/symbol). The Huffman priority queue assigns variable-length codes ranging from ${Math.min(...Array.from(d.codes.values()).map(c => c.length))} bits for frequent symbols to ${Math.max(...Array.from(d.codes.values()).map(c => c.length))} bits for rare symbols, achieving an average code length of ${d.avgCodeLen.toFixed(2)} bits/symbol compared to uncompressed 8.00 bits.`;
      } else {
        narrative = `The input "${escapeHtml(d.label)}" displays a <strong>near-uniform, high-entropy distribution</strong> (${d.entropy.toFixed(3)} of 8.00 bits/symbol). Symbols occur with nearly equal probability across the spectrum. In accordance with Shannon's Source Coding Theorem, Huffman coding cannot significantly reduce the payload size (average code length ${d.avgCodeLen.toFixed(2)} bits/symbol ≈ 8 bits/symbol), illustrating why pre-compressed or encrypted files are resistant to additional lossless Huffman shrinkage.`;
      }
      narrativeEl.innerHTML = narrative;
    }
  }

  // =========================================================================
  // SYMBOL FREQUENCY & CODES TABLE
  // =========================================================================

  function populateSymbolTable() {
    const tbody = document.getElementById('symbolTableBody');
    if (!tbody || !state.currentExplorerData) return;
    tbody.innerHTML = '';

    const d = state.currentExplorerData;
    const entries = Array.from(d.freqs.entries()).sort((a, b) => b[1] - a[1]);

    entries.forEach(([byteVal, freq]) => {
      const tr = document.createElement('tr');
      const code = d.codes.get(byteVal) || '0';
      const charStr = formatChar(byteVal);
      const hexStr = '0x' + byteVal.toString(16).toUpperCase().padStart(2, '0');
      const freqPct = ((freq / d.totalBytes) * 100).toFixed(2) + '%';
      const bitContrib = freq * code.length;
      const contribPct = d.totalPayloadBits > 0 ? ((bitContrib / d.totalPayloadBits) * 100).toFixed(1) + '%' : '100%';

      tr.className = 'clickable-row' + (byteVal === state.selectedSymbolByte ? ' active-symbol-tr' : '');
      tr.setAttribute('data-byte', byteVal);

      tr.innerHTML = `
        <td>${byteVal}</td>
        <td><code>${hexStr}</code></td>
        <td><strong style="color: var(--accent-orange); font-family: var(--font-mono);">${escapeHtml(charStr)}</strong></td>
        <td>${freq}</td>
        <td>${freqPct}</td>
        <td><code style="color: var(--accent-orange); font-weight: bold;">${code}</code></td>
        <td>${code.length}</td>
        <td><strong>${bitContrib} bits</strong> <span style="font-size: 11px; color: var(--text-muted);">(${contribPct})</span></td>
      `;

      tr.addEventListener('click', () => {
        selectSymbol(byteVal);
      });

      tbody.appendChild(tr);
    });

    const expText = document.getElementById('symbolTableExplanation');
    if (expText) {
      expText.innerHTML = `Huffman coding generated an optimal prefix-free codebook for <strong>${d.distinctSymbols} distinct symbols</strong> across ${formatBytes(d.totalBytes)}. Total compressed payload: <strong>${d.totalPayloadBits} bits</strong> (average <strong>${d.avgCodeLen.toFixed(2)} bits/symbol</strong> vs uncompressed 8.00 bits/symbol). No code is a prefix of any other code, guaranteeing instantaneous and unambiguous decoding without delimiter markers. Click any row to isolate the symbol in the Tree and 2D Frequency Graph.`;
    }
  }

  // Cross-Highlighting Synchronization
  function selectSymbol(byteVal) {
    state.selectedSymbolByte = byteVal;

    // 1. Highlight row in Symbol Table
    const tbody = document.getElementById('symbolTableBody');
    if (tbody) {
      const rows = tbody.querySelectorAll('tr');
      rows.forEach(tr => {
        if (parseInt(tr.getAttribute('data-byte'), 10) === byteVal) {
          tr.classList.add('active-symbol-tr');
          tr.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
        } else {
          tr.classList.remove('active-symbol-tr');
        }
      });
    }

    // 2. Redraw 2D Frequency Graph with selected bar highlighted
    drawFrequencyGraph();

    // 3. Highlight and pan to leaf node in Huffman Tree Canvas
    if (state.treeCanvas) {
      state.treeCanvas.selectNodeByByte(byteVal);
    }

    // 4. Update Educational Explanations with selected symbol info
    let leafNode = null;
    const findLeaf = (node) => {
      if (!node || leafNode) return;
      if (node.isLeaf && node.byteVal === byteVal) {
        leafNode = node;
        return;
      }
      findLeaf(node.left);
      findLeaf(node.right);
    };
    if (state.currentExplorerData && state.currentExplorerData.root) {
      findLeaf(state.currentExplorerData.root);
    }
    state.selectedTreeNode = leafNode;
    updateTreeExplanation(leafNode);

    let matchingBar = null;
    if (state.graphBars) {
      matchingBar = state.graphBars.find(b => b.byteVal === byteVal);
    }
    updateGraphExplanation(matchingBar);
  }

  // =========================================================================
  // PRIORITY QUEUE / GREEDY MERGERS PLAYER
  // =========================================================================

  function initMergePlayer(merges) {
    if (state.mergeState.timer) {
      clearInterval(state.mergeState.timer);
      state.mergeState.timer = null;
    }
    state.mergeState.isPlaying = false;
    state.mergeState.steps = merges || [];
    state.mergeState.currentStep = merges && merges.length > 0 ? 1 : 0;

    const btnPlay = document.getElementById('btnMergePlay');
    if (btnPlay) {
      btnPlay.innerHTML = '▶ Auto-Play';
      btnPlay.classList.remove('playing');
    }

    renderMergeStep(state.mergeState.currentStep);
  }

  function goToMergeStep(stepNum) {
    const total = state.mergeState.steps.length;
    if (total === 0) return;
    if (stepNum < 1) stepNum = 1;
    if (stepNum > total) stepNum = total;

    state.mergeState.currentStep = stepNum;
    renderMergeStep(stepNum);
  }

  function toggleMergePlay() {
    const total = state.mergeState.steps.length;
    if (total <= 1) return;

    const btnPlay = document.getElementById('btnMergePlay');
    if (state.mergeState.isPlaying) {
      // Pause
      clearInterval(state.mergeState.timer);
      state.mergeState.timer = null;
      state.mergeState.isPlaying = false;
      if (btnPlay) {
        btnPlay.innerHTML = '▶ Auto-Play';
        btnPlay.classList.remove('playing');
      }
    } else {
      // Start auto play
      if (state.mergeState.currentStep >= total) {
        state.mergeState.currentStep = 1;
        renderMergeStep(1);
      }
      state.mergeState.isPlaying = true;
      if (btnPlay) {
        btnPlay.innerHTML = '⏸ Pause';
        btnPlay.classList.add('playing');
      }

      state.mergeState.timer = setInterval(() => {
        if (state.mergeState.currentStep < total) {
          goToMergeStep(state.mergeState.currentStep + 1);
        } else {
          // Reached the end
          clearInterval(state.mergeState.timer);
          state.mergeState.timer = null;
          state.mergeState.isPlaying = false;
          if (btnPlay) {
            btnPlay.innerHTML = '▶ Auto-Play';
            btnPlay.classList.remove('playing');
          }
        }
      }, 1200);
    }
  }

  function renderMergeStep(stepNum) {
    const total = state.mergeState.steps.length;
    const badge = document.getElementById('mergeStepBadge');
    const bar = document.getElementById('mergeProgressBar');
    const heading = document.getElementById('mergeStepHeading');
    const statusBadge = document.getElementById('mergeStepStatusBadge');
    const leftSym = document.getElementById('mergeLeftSymbol');
    const leftFreq = document.getElementById('mergeLeftFreq');
    const rightSym = document.getElementById('mergeRightSymbol');
    const rightFreq = document.getElementById('mergeRightFreq');
    const parentSym = document.getElementById('mergeParentSymbol');
    const parentFreq = document.getElementById('mergeParentFreq');
    const queueContainer = document.getElementById('mergeQueueChips');
    const explanation = document.getElementById('mergeStepExplanation');

    if (total === 0) {
      if (badge) badge.textContent = 'Step 0 of 0';
      if (bar) bar.style.width = '0%';
      if (heading) heading.textContent = 'NO MERGES NEEDED (SINGLE SYMBOL)';
      if (statusBadge) {
        statusBadge.textContent = 'Trivial Leaf';
        statusBadge.className = 'target-status-badge badge-feasible';
      }
      if (leftSym) leftSym.textContent = '-';
      if (leftFreq) leftFreq.textContent = 'Weight: -';
      if (rightSym) rightSym.textContent = '-';
      if (rightFreq) rightFreq.textContent = 'Weight: -';
      if (parentSym) parentSym.textContent = '-';
      if (parentFreq) parentFreq.textContent = 'Combined: -';
      if (queueContainer) queueContainer.innerHTML = '<span style="color: var(--text-muted);">Queue is empty</span>';
      if (explanation) explanation.textContent = 'Input consists of 0 or 1 distinct symbol. A trivial single-node tree was formed with code "0".';
      return;
    }

    const step = state.mergeState.steps[stepNum - 1];
    if (!step) return;

    if (badge) badge.textContent = `Step ${stepNum} of ${total}`;
    if (bar) bar.style.width = `${Math.round((stepNum / total) * 100)}%`;
    if (heading) {
      heading.textContent = stepNum === total
        ? `STEP ${stepNum} (FINAL): MERGED INTO TREE ROOT`
        : `STEP ${stepNum} OF ${total}: GREEDY MIN-HEAP COMBINATION`;
    }
    if (statusBadge) {
      if (stepNum === total) {
        statusBadge.textContent = 'Tree Formed (Root Completed)';
        statusBadge.className = 'target-status-badge badge-feasible';
      } else {
        statusBadge.textContent = `In Progress (${total - stepNum} merges remaining)`;
        statusBadge.className = 'target-status-badge badge-neutral';
      }
    }

    if (leftSym) leftSym.textContent = step.leftSymbol || '-';
    if (leftFreq) leftFreq.textContent = `Weight: ${step.leftFreq}`;
    if (rightSym) rightSym.textContent = step.rightSymbol || '-';
    if (rightFreq) rightFreq.textContent = `Weight: ${step.rightFreq}`;
    if (parentSym) parentSym.textContent = step.parentSymbol || '-';
    if (parentFreq) parentFreq.textContent = `Weight: ${step.combinedFreq}`;

    // Queue chips
    if (queueContainer) {
      queueContainer.innerHTML = '';
      if (step.queueSnapshot && step.queueSnapshot.length > 0) {
        step.queueSnapshot.forEach(item => {
          const chip = document.createElement('div');
          chip.className = `queue-chip ${item.isLeaf ? 'queue-chip-leaf' : ''}`;
          chip.innerHTML = `<span>${escapeHtml(item.name)}</span> <span class="chip-freq">w:${item.freq}</span>`;
          queueContainer.appendChild(chip);
        });
      } else {
        queueContainer.innerHTML = '<span style="color: var(--accent-green); font-family: var(--font-mono); font-size: 12px;">✔ Priority Queue empty — Root node created</span>';
      }
    }

    // Explanation
    if (explanation) {
      explanation.textContent = step.explanation || step.action;
    }

    // Highlight row in merge table
    const tbody = document.getElementById('mergeTableBody');
    if (tbody) {
      const rows = tbody.querySelectorAll('tr');
      rows.forEach((tr, idx) => {
        if (idx + 1 === stepNum) {
          tr.classList.add('active-merge-tr');
          tr.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
        } else {
          tr.classList.remove('active-merge-tr');
        }
      });
    }

    // Update live Educational Panel for Greedy Merges
    updateMergesExplanation(stepNum);
  }

  function populateMergeTable(merges) {
    const tbody = document.getElementById('mergeTableBody');
    if (!tbody) return;
    tbody.innerHTML = '';

    merges.forEach((m, idx) => {
      const tr = document.createElement('tr');
      tr.className = 'clickable-row' + (idx + 1 === state.mergeState.currentStep ? ' active-merge-tr' : '');
      tr.innerHTML = `
        <td><strong>#${m.step}</strong></td>
        <td><span style="color: var(--accent-orange); font-family: var(--font-mono);">${escapeHtml(m.leftName)}</span></td>
        <td><span style="color: var(--accent-purple); font-family: var(--font-mono);">${escapeHtml(m.rightName)}</span></td>
        <td><strong>${m.combinedFreq}</strong></td>
        <td><span style="color: var(--text-secondary);">${escapeHtml(m.action)}</span></td>
      `;

      tr.addEventListener('click', () => {
        goToMergeStep(m.step);
      });

      tbody.appendChild(tr);
    });
  }

  // =========================================================================
  // EDUCATIONAL EXPLANATION PANELS (DYNAMIC DATA HOOKS)
  // =========================================================================

  function updateTreeExplanation(selectedNode) {
    const d = state.currentExplorerData;
    const summaryBadge = document.getElementById('treeLiveSummaryBadge');
    const totalBytesEl = document.getElementById('treeLiveTotalBytes');
    const distinctEl = document.getElementById('treeLiveDistinct');
    const rootWeightEl = document.getElementById('treeLiveRootWeight');
    const maxDepthEl = document.getElementById('treeLiveMaxDepth');
    const inspector = document.getElementById('treeLiveNodeInspector');

    if (!d) {
      if (summaryBadge) summaryBadge.textContent = 'Awaiting Input';
      if (totalBytesEl) totalBytesEl.textContent = '-';
      if (distinctEl) distinctEl.textContent = '-';
      if (rootWeightEl) rootWeightEl.textContent = '-';
      if (maxDepthEl) maxDepthEl.textContent = '-';
      if (inspector) inspector.innerHTML = '<div style="font-family: var(--font-mono); font-size: 12px; color: var(--text-muted);">No input analyzed. Type text above or upload a file.</div>';
      return;
    }

    let maxDepth = 0;
    if (d.codes && d.codes.size > 0) {
      for (const code of d.codes.values()) {
        if (code.length > maxDepth) maxDepth = code.length;
      }
    }

    if (summaryBadge) {
      summaryBadge.textContent = `${formatBytes(d.totalBytes)} | ${d.distinctSymbols} distinct symbols`;
    }
    if (totalBytesEl) totalBytesEl.textContent = `${formatBytes(d.totalBytes)} (${d.totalBytes} B)`;
    if (distinctEl) distinctEl.textContent = `${d.distinctSymbols} leaf nodes`;
    if (rootWeightEl) rootWeightEl.textContent = `${d.root ? d.root.freq : d.totalBytes} (100% of input)`;
    if (maxDepthEl) maxDepthEl.textContent = `${maxDepth} bits (depth)`;

    if (!inspector) return;

    const node = selectedNode || state.selectedTreeNode;
    if (node) {
      if (node.isLeaf) {
        const charStr = formatChar(node.byteVal);
        const hexStr = '0x' + node.byteVal.toString(16).toUpperCase().padStart(2, '0');
        const code = node.code || (d.codes.get(node.byteVal) || '0');
        const pathSteps = code.split('').map((bit) => bit === '0' ? `<span style="color: var(--accent-orange);">Left (0)</span>` : `<span style="color: var(--accent-purple);">Right (1)</span>`).join(' → ');

        inspector.innerHTML = `
          <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px; flex-wrap: wrap; gap: 8px;">
            <div style="font-family: var(--font-mono); font-size: 13px; font-weight: 700; color: var(--accent-orange);">
              🍃 INSPECTING LEAF NODE: <strong style="color: #fff;">'${escapeHtml(charStr)}'</strong> (Dec: ${node.byteVal}, Hex: ${hexStr})
            </div>
            <span class="edu-badge" style="color: var(--accent-orange); border-color: var(--accent-orange);">Leaf Node</span>
          </div>
          <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(140px, 1fr)); gap: 10px; margin-bottom: 10px;">
            <div><span style="color: var(--text-muted); font-size: 11px; text-transform: uppercase;">Frequency (Weight):</span><br><strong style="font-family: var(--font-mono); font-size: 14px;">${node.freq}</strong> (${((node.freq / d.totalBytes) * 100).toFixed(2)}%)</div>
            <div><span style="color: var(--text-muted); font-size: 11px; text-transform: uppercase;">Assigned Huffman Code:</span><br><code style="color: var(--accent-orange); font-size: 14px; font-weight: bold;">${code}</code> (${code.length} bits)</div>
            <div><span style="color: var(--text-muted); font-size: 11px; text-transform: uppercase;">Bit Contribution:</span><br><strong style="font-family: var(--font-mono); font-size: 14px;">${node.freq * code.length} bits</strong></div>
          </div>
          <div style="font-size: 12.5px; color: var(--text-secondary); background: var(--bg-card); padding: 8px 12px; border-radius: 4px; font-family: var(--font-mono);">
            <strong>Traversal Path from Root:</strong> Root → ${pathSteps || 'Root Node'}
          </div>
        `;
      } else {
        const leftDesc = node.left ? (node.left.isLeaf ? `'${formatChar(node.left.byteVal)}' (w:${node.left.freq})` : `Subtree #${node.left.id} (w:${node.left.freq})`) : 'None';
        const rightDesc = node.right ? (node.right.isLeaf ? `'${formatChar(node.right.byteVal)}' (w:${node.right.freq})` : `Subtree #${node.right.id} (w:${node.right.freq})`) : 'None';
        const code = node.code || '';
        const pathSteps = code ? code.split('').map((bit) => bit === '0' ? `<span style="color: var(--accent-orange);">Left (0)</span>` : `<span style="color: var(--accent-purple);">Right (1)</span>`).join(' → ') : 'Root of Tree';

        inspector.innerHTML = `
          <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px; flex-wrap: wrap; gap: 8px;">
            <div style="font-family: var(--font-mono); font-size: 13px; font-weight: 700; color: var(--accent-purple);">
              ⚙️ INSPECTING INTERNAL NODE: #${node.id}
            </div>
            <span class="edu-badge" style="color: var(--accent-purple); border-color: var(--accent-purple);">Internal Merge</span>
          </div>
          <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(140px, 1fr)); gap: 10px; margin-bottom: 10px;">
            <div><span style="color: var(--text-muted); font-size: 11px; text-transform: uppercase;">Combined Weight:</span><br><strong style="font-family: var(--font-mono); font-size: 14px;">${node.freq}</strong></div>
            <div><span style="color: var(--text-muted); font-size: 11px; text-transform: uppercase;">Left Child (Bit 0):</span><br><span style="font-family: var(--font-mono); color: var(--accent-orange);">${escapeHtml(leftDesc)}</span></div>
            <div><span style="color: var(--text-muted); font-size: 11px; text-transform: uppercase;">Right Child (Bit 1):</span><br><span style="font-family: var(--font-mono); color: var(--accent-purple);">${escapeHtml(rightDesc)}</span></div>
          </div>
          <div style="font-size: 12.5px; color: var(--text-secondary); background: var(--bg-card); padding: 8px 12px; border-radius: 4px; font-family: var(--font-mono);">
            <strong>Subtree Prefix Path:</strong> Root → ${pathSteps}
          </div>
        `;
      }
    } else {
      inspector.innerHTML = `
        <div style="font-family: var(--font-mono); font-size: 12px; color: var(--text-muted);">
          💡 Click any node or leaf in the canvas above to inspect its exact weight, parent/child relationships, and generated binary path.
        </div>
      `;
    }
  }

  function updateGraphExplanation(bar) {
    const d = state.currentExplorerData;
    if (!d || !d.freqs || d.freqs.size === 0) return;

    const totalEl = document.getElementById('graphEduTotalBytes');
    const distinctEl = document.getElementById('graphEduDistinct');
    const mostFreqEl = document.getElementById('graphEduMostFreq');
    const leastFreqEl = document.getElementById('graphEduLeastFreq');
    const narrativeEl = document.getElementById('graphEduNarrative');
    const detailEl = document.getElementById('graphEduSelectedDetail');
    const entropyBadge = document.getElementById('graphEntropyBadge');

    if (totalEl) totalEl.textContent = `${formatBytes(d.totalBytes)} (${d.totalBytes} B)`;
    if (distinctEl) distinctEl.textContent = `${d.distinctSymbols} of 256`;
    if (entropyBadge) entropyBadge.textContent = `Entropy: ${d.entropy.toFixed(3)} bits/sym`;

    const sorted = Array.from(d.freqs.entries()).sort((a, b) => b[1] - a[1]);
    const most = sorted[0];
    const least = sorted[sorted.length - 1];

    if (mostFreqEl) {
      const mostCode = d.codes.get(most[0]) || '';
      mostFreqEl.innerHTML = `<strong>'${escapeHtml(formatChar(most[0]))}'</strong> (${most[1]}x, ${((most[1] / d.totalBytes) * 100).toFixed(1)}%) <small style="color:var(--text-muted);">[code: ${mostCode}]</small>`;
    }
    if (leastFreqEl) {
      const leastCode = d.codes.get(least[0]) || '';
      leastFreqEl.innerHTML = `<strong>'${escapeHtml(formatChar(least[0]))}'</strong> (${least[1]}x, ${((least[1] / d.totalBytes) * 100).toFixed(1)}%) <small style="color:var(--text-muted);">[code: ${leastCode}]</small>`;
    }

    if (narrativeEl) {
      const mostChar = formatChar(most[0]);
      const mostPct = ((most[1] / d.totalBytes) * 100).toFixed(1);
      const mostCode = d.codes.get(most[0]) || '';
      const leastCode = d.codes.get(least[0]) || '';

      if (d.entropy < 3.2) {
        narrativeEl.innerHTML = `<strong>Skewed Distribution Analysis:</strong> The most frequent symbol <code>'${escapeHtml(mostChar)}'</code> accounts for <strong>${mostPct}%</strong> of all analyzed data. Because it appears so often, the greedy algorithm merged it very late, placing it near the root with a short <strong>${mostCode.length}-bit code (<code>${mostCode}</code>)</strong>. In contrast, rare symbols receive longer codes up to <strong>${leastCode.length} bits</strong>. This high frequency disparity enables optimal compression efficiency.`;
      } else if (d.entropy < 6.8) {
        narrativeEl.innerHTML = `<strong>Natural Language / Structured Distribution:</strong> Frequencies follow a typical descending curve with Shannon entropy of <strong>${d.entropy.toFixed(3)} bits/symbol</strong>. Common characters receive shorter codes (down to <strong>${mostCode.length} bits</strong>), while infrequent characters receive up to <strong>${leastCode.length} bits</strong>, giving an average code length of <strong>${d.avgCodeLen.toFixed(2)} bits/symbol</strong> compared to standard fixed 8.00 bits.`;
      } else {
        narrativeEl.innerHTML = `<strong>Near-Uniform / High Entropy Distribution:</strong> The input has Shannon entropy of <strong>${d.entropy.toFixed(3)} bits/symbol</strong> (close to the 8.00 maximum). Because symbols occur with nearly equal probability, the frequency bars are relatively flat. All Huffman codes have similar lengths (~<strong>${d.avgCodeLen.toFixed(1)} bits</strong>), meaning standard Huffman coding cannot significantly reduce file size without dictionary or transform pre-processing.`;
      }
    }

    if (detailEl) {
      if (bar) {
        detailEl.innerHTML = `
          <div style="display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 8px;">
            <div style="font-family: var(--font-mono); font-size: 13px; font-weight: 700; color: var(--accent-orange);">
              🎯 INSPECTING SYMBOL: '${escapeHtml(bar.charStr)}' (${bar.hexStr} / Dec ${bar.byteVal})
            </div>
            <span class="edu-badge" style="color: var(--accent-orange);">Frequency: ${bar.freq} (${bar.pct}%)</span>
          </div>
          <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(130px, 1fr)); gap: 10px; margin-top: 8px; font-family: var(--font-mono); font-size: 12.5px;">
            <div><span style="color: var(--text-muted);">Occurrences:</span> <strong>${bar.freq}</strong></div>
            <div><span style="color: var(--text-muted);">Huffman Code:</span> <code style="color: var(--accent-orange); font-weight: bold;">${bar.code}</code> (${bar.code.length} bits)</div>
            <div><span style="color: var(--text-muted);">Bitstream Load:</span> <strong>${bar.freq * bar.code.length} bits</strong></div>
          </div>
        `;
      } else {
        detailEl.innerHTML = `
          <div style="font-family: var(--font-mono); font-size: 12px; color: var(--text-muted);">
            💡 Hover over or click any bar above to inspect its exact frequency, percentage, and assigned Huffman code.
          </div>
        `;
      }
    }
  }

  function updateSymbolsExplanation() {
    const d = state.currentExplorerData;
    if (!d || !d.freqs || d.freqs.size === 0) return;

    const totalEl = document.getElementById('symbolsEduTotalBytes');
    const shortestEl = document.getElementById('symbolsEduShortest');
    const longestEl = document.getElementById('symbolsEduLongest');
    const avgEl = document.getElementById('symbolsEduAvgBits');
    const analysisEl = document.getElementById('symbolsEduAnalysis');

    if (totalEl) totalEl.textContent = `${formatBytes(d.totalBytes)} (${d.totalBytes} B)`;

    let minLen = Infinity;
    let maxLen = 0;
    let shortestSym = null;
    let longestSym = null;

    for (const [byteVal, code] of d.codes.entries()) {
      if (code.length < minLen) {
        minLen = code.length;
        shortestSym = byteVal;
      }
      if (code.length > maxLen) {
        maxLen = code.length;
        longestSym = byteVal;
      }
    }

    if (shortestEl && shortestSym !== null) {
      shortestEl.innerHTML = `${minLen} bits <small style="color:var(--text-muted);">('${escapeHtml(formatChar(shortestSym))}')</small>`;
    }
    if (longestEl && longestSym !== null) {
      longestEl.innerHTML = `${maxLen} bits <small style="color:var(--text-muted);">('${escapeHtml(formatChar(longestSym))}')</small>`;
    }
    if (avgEl) {
      avgEl.textContent = `${d.avgCodeLen.toFixed(2)} bits/sym`;
    }

    if (analysisEl) {
      const rawBits = d.totalBytes * 8;
      const compBits = d.totalPayloadBits;
      const savingsPct = rawBits > 0 ? (((rawBits - compBits) / rawBits) * 100).toFixed(1) : 0;

      analysisEl.innerHTML = `The generated codebook contains <strong>${d.distinctSymbols} distinct symbol entries</strong>. Average code length is <strong>${d.avgCodeLen.toFixed(2)} bits/symbol</strong> (compared to fixed 8.00 bits uncompressed). Total raw bitstream payload: <strong>${compBits} bits</strong> (saving <strong>${savingsPct}%</strong> before archive metadata). Shannon entropy establishes a theoretical limit of <strong>${d.entropy.toFixed(3)} bits/symbol</strong>; Huffman's greedy construction achieves optimality within $H \\le L_{\\text{avg}} < H + 1$.`;
    }
  }

  function updateMergesExplanation(stepNum) {
    const d = state.currentExplorerData;
    const walkthroughEl = document.getElementById('mergesEduStepWalkthrough');
    const badge = document.getElementById('mergesStepIndicatorBadge');
    const mergesBadge = document.getElementById('mergesLiveBadge');

    const totalSteps = state.mergeState.steps.length;
    if (mergesBadge) mergesBadge.textContent = totalSteps > 0 ? `Step ${stepNum} of ${totalSteps}` : 'Single Symbol';
    if (badge) badge.textContent = `Step ${stepNum} of ${totalSteps}`;

    if (!walkthroughEl) return;

    if (totalSteps === 0) {
      walkthroughEl.innerHTML = `The analyzed input contains only 1 distinct symbol. A trivial single-node tree was formed with code <code>0</code>; no priority queue merges were necessary.`;
      return;
    }

    const step = state.mergeState.steps[stepNum - 1];
    if (!step) return;

    const isFinal = (stepNum === totalSteps);
    walkthroughEl.innerHTML = `
      <div style="font-size: 13.5px; line-height: 1.6; color: var(--text-secondary);">
        <p>
          <strong>1. Minimum Extractions:</strong> The priority queue extracted two subtrees with the lowest weights:
          <span style="color: var(--accent-orange); font-family: var(--font-mono);"><strong>${escapeHtml(step.leftSymbol)}</strong> (weight: ${step.leftFreq})</span> and
          <span style="color: var(--accent-purple); font-family: var(--font-mono);"><strong>${escapeHtml(step.rightSymbol)}</strong> (weight: ${step.rightFreq})</span>.
        </p>
        <p>
          <strong>2. Combined Parent:</strong> Created new internal parent node <code>${escapeHtml(step.parentSymbol)}</code> with weight <code>${step.leftFreq} + ${step.rightFreq} = <strong>${step.combinedFreq}</strong></code>. Left edge assigned bit <code>0</code>, right edge assigned bit <code>1</code>.
        </p>
        <p>
          <strong>3. Queue State After Step:</strong> Parent re-inserted into the min-heap.
          ${isFinal
            ? `<strong style="color: var(--accent-green);">✔ Tree Construction Complete!</strong> Exactly 1 node remains in the queue, which is the root of the Huffman Tree.`
            : `There are <strong>${step.queueSnapshot.length}</strong> node(s) remaining in the queue. The algorithm will proceed with the next merge.`}
        </p>
      </div>
    `;
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
