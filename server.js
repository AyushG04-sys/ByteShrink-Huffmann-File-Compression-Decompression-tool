/**
 * ByteShrink — Smart File Compression Studio
 * Local Node.js Application Bridge & Offline Server
 * 
 * Provides an offline, zero-dependency HTTP server that bridges the
 * modern frontend UI with the native C++ Huffman compression engine (ByteShrink.exe).
 */

const http = require('http');
const fs = require('fs');
const path = require('path');
const { execFile, spawn } = require('child_process');

const PORT = process.env.PORT || 3000;
const WEB_DIR = path.join(__dirname, 'web');
const WORKSPACE_DIR = path.join(__dirname, 'test_workspace');
const BINARY_PATH = path.join(__dirname, 'build', 'ByteShrink.exe');

// Ensure test_workspace exists
if (!fs.existsSync(WORKSPACE_DIR)) {
  fs.mkdirSync(WORKSPACE_DIR, { recursive: true });
}

// MIME Types Map
const MIME_TYPES = {
  '.html': 'text/html; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.js': 'application/javascript; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.jpeg': 'image/jpeg',
  '.bmp': 'image/bmp',
  '.webp': 'image/webp',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.hfx': 'application/octet-stream',
  '.txt': 'text/plain; charset=utf-8',
  '.pdf': 'application/pdf'
};

// Check if C++ engine binary exists
function getEnginePath() {
  if (fs.existsSync(BINARY_PATH)) {
    return BINARY_PATH;
  }
  const alt = path.join(__dirname, 'build', 'HuffmanX.exe');
  if (fs.existsSync(alt)) {
    return alt;
  }
  return null;
}

const server = http.createServer((req, res) => {
  const parsedUrl = new URL(req.url, `http://${req.headers.host}`);
  const pathname = parsedUrl.pathname;

  // Set CORS & Security Headers
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (req.method === 'OPTIONS') {
    res.writeHead(204);
    res.end();
    return;
  }

  // API Endpoints
  if (pathname.startsWith('/api/')) {
    handleApi(req, res, pathname, parsedUrl);
    return;
  }

  // Static File Serving
  handleStatic(req, res, pathname);
});

function handleStatic(req, res, pathname) {
  let safePath = pathname === '/' ? '/index.html' : pathname;
  // Prevent directory traversal
  const resolvedPath = path.normalize(path.join(WEB_DIR, safePath));
  if (!resolvedPath.startsWith(WEB_DIR)) {
    res.writeHead(403, { 'Content-Type': 'text/plain' });
    res.end('Access Denied');
    return;
  }

  fs.stat(resolvedPath, (err, stats) => {
    if (err || !stats.isFile()) {
      res.writeHead(404, { 'Content-Type': 'text/plain' });
      res.end('404 Not Found');
      return;
    }

    const ext = path.extname(resolvedPath).toLowerCase();
    const contentType = MIME_TYPES[ext] || 'application/octet-stream';

    res.writeHead(200, {
      'Content-Type': contentType,
      'Content-Length': stats.size,
      'Cache-Control': 'no-cache'
    });

    const stream = fs.createReadStream(resolvedPath);
    stream.pipe(res);
  });
}

function handleApi(req, res, pathname, parsedUrl) {
  const engine = getEnginePath();

  if (pathname === '/api/status') {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({
      status: 'online',
      app: 'ByteShrink',
      version: '1.0.0',
      enginePath: engine,
      engineAvailable: engine !== null
    }));
    return;
  }

  if (pathname === '/api/compress' && req.method === 'POST') {
    handleCompressApi(req, res, engine);
    return;
  }

  if (pathname === '/api/decompress' && req.method === 'POST') {
    handleDecompressApi(req, res, engine);
    return;
  }

  if (pathname === '/api/inspect' && req.method === 'POST') {
    handleInspectApi(req, res, engine);
    return;
  }

  if (pathname === '/api/open-file' && req.method === 'POST') {
    handleOpenFileApi(req, res);
    return;
  }

  if (pathname === '/api/download' && req.method === 'GET') {
    let resolved = null;
    let downloadFilename = 'download';

    const fileNameParam = parsedUrl.searchParams.get('file');
    const pathParam = parsedUrl.searchParams.get('path');

    if (fileNameParam) {
      const safeName = path.basename(fileNameParam);
      resolved = path.join(WORKSPACE_DIR, safeName);
      downloadFilename = safeName;
    } else if (pathParam) {
      const safeParam = path.normalize(pathParam);
      // Ensure path stays within the workspace or project directory
      if (safeParam.startsWith(WORKSPACE_DIR) || safeParam.startsWith(__dirname)) {
        resolved = safeParam;
        downloadFilename = path.basename(safeParam);
      }
    }

    if (resolved && fs.existsSync(resolved) && fs.statSync(resolved).isFile()) {
      const stats = fs.statSync(resolved);
      res.writeHead(200, {
        'Content-Type': 'application/octet-stream',
        'Content-Length': stats.size,
        'Content-Disposition': `attachment; filename="${downloadFilename}"`,
        'Cache-Control': 'no-cache'
      });
      fs.createReadStream(resolved).pipe(res);
      return;
    }

    res.writeHead(404, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ error: 'File Not Found or Access Denied' }));
    return;
  }

  res.writeHead(404, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify({ error: 'Endpoint Not Found' }));
}

function parseMultipartBody(req, callback) {
  const boundaryMatch = req.headers['content-type'] && req.headers['content-type'].match(/boundary=(?:"([^"]+)"|([^;]+))/i);
  if (!boundaryMatch) {
    return callback(new Error('Missing multipart boundary'));
  }

  const boundary = boundaryMatch[1] || boundaryMatch[2];
  const chunks = [];

  req.on('data', chunk => chunks.push(chunk));
  req.on('end', () => {
    const buffer = Buffer.concat(chunks);
    const boundaryBuffer = Buffer.from('--' + boundary);
    const parts = [];

    let start = 0;
    while ((start = buffer.indexOf(boundaryBuffer, start)) !== -1) {
      start += boundaryBuffer.length;
      if (buffer[start] === 0x2D && buffer[start + 1] === 0x2D) {
        break; // End boundary '--'
      }
      if (buffer[start] === 0x0D && buffer[start + 1] === 0x0A) {
        start += 2;
      }

      const next = buffer.indexOf(boundaryBuffer, start);
      if (next === -1) break;

      const partBuffer = buffer.slice(start, next - 2); // Exclude \r\n before next boundary
      const headerEnd = partBuffer.indexOf('\r\n\r\n');
      if (headerEnd !== -1) {
        const headerStr = partBuffer.slice(0, headerEnd).toString('utf8');
        const content = partBuffer.slice(headerEnd + 4);

        const nameMatch = headerStr.match(/name="([^"]+)"/);
        const filenameMatch = headerStr.match(/filename="([^"]+)"/);

        parts.push({
          name: nameMatch ? nameMatch[1] : null,
          filename: filenameMatch ? filenameMatch[1] : null,
          data: content
        });
      }
      start = next;
    }

    callback(null, parts);
  });
  req.on('error', err => callback(err));
}

function handleCompressApi(req, res, engine) {
  if (!engine) {
    res.writeHead(500, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ error: 'Native ByteShrink C++ engine not found in build directory.' }));
    return;
  }

  parseMultipartBody(req, (err, parts) => {
    if (err) {
      res.writeHead(400, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ error: err.message }));
      return;
    }

    const filePart = parts.find(p => p.filename);
    const outputNamePart = parts.find(p => p.name === 'outputName');

    if (!filePart) {
      res.writeHead(400, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ error: 'No file uploaded' }));
      return;
    }

    const originalName = path.basename(filePart.filename);
    let archiveName = path.basename(outputNamePart ? outputNamePart.data.toString('utf8').trim() : `${originalName}.hfx`);
    if (!archiveName.toLowerCase().endsWith('.hfx')) {
      archiveName += '.hfx';
    }

    const inputPath = path.join(WORKSPACE_DIR, originalName);
    const outputPath = path.join(WORKSPACE_DIR, archiveName);

    try {
      fs.writeFileSync(inputPath, filePart.data);

      // Invoke C++ CLI engine: ByteShrink.exe -c <input> <output>
      execFile(engine, ['-c', inputPath, outputPath], (execErr, stdout, stderr) => {
        if (execErr) {
          res.writeHead(500, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify({ error: `C++ Engine Execution Failed: ${stderr || execErr.message}` }));
          return;
        }

        const outStats = fs.statSync(outputPath);
        const inStats = fs.statSync(inputPath);

        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({
          success: true,
          originalName,
          archiveName,
          originalSize: inStats.size,
          compressedSize: outStats.size,
          archivePath: outputPath,
          downloadUrl: `/api/download?file=${encodeURIComponent(archiveName)}`,
          stdout: stdout
        }));
      });
    } catch (e) {
      res.writeHead(500, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ error: e.message }));
    }
  });
}

function handleDecompressApi(req, res, engine) {
  if (!engine) {
    res.writeHead(500, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ error: 'Native ByteShrink C++ engine not found in build directory.' }));
    return;
  }

  parseMultipartBody(req, (err, parts) => {
    if (err) {
      res.writeHead(400, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ error: err.message }));
      return;
    }

    const archivePart = parts.find(p => p.filename);
    const restoredNamePart = parts.find(p => p.name === 'restoredName');

    if (!archivePart) {
      res.writeHead(400, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ error: 'No archive uploaded' }));
      return;
    }

    const archiveName = path.basename(archivePart.filename);
    let restoredName = restoredNamePart ? path.basename(restoredNamePart.data.toString('utf8').trim()) : '';

    const archivePath = path.join(WORKSPACE_DIR, archiveName);

    try {
      fs.writeFileSync(archivePath, archivePart.data);

      // First inspect archive to get default filename if none provided
      execFile(engine, ['-i', archivePath], (inspErr, inspStdout) => {
        if (!restoredName) {
          const fnMatch = inspStdout && inspStdout.match(/Original Name\s*:\s*(.+)/i);
          restoredName = fnMatch ? path.basename(fnMatch[1].trim()) : 'restored_' + archiveName.replace(/\.hfx$/i, '');
        }

        const outputPath = path.join(WORKSPACE_DIR, restoredName);

        // Invoke C++ CLI engine: ByteShrink.exe -d <archive> <output>
        execFile(engine, ['-d', archivePath, outputPath], (execErr, stdout, stderr) => {
          if (execErr) {
            res.writeHead(500, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({ error: `Decompression Failed: ${stderr || execErr.message}` }));
            return;
          }

          const outStats = fs.statSync(outputPath);
          const crcMatch = !stdout.includes('CRC-32 Mismatch');

          res.writeHead(200, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify({
            success: true,
            restoredName,
            restoredSize: outStats.size,
            restoredPath: outputPath,
            downloadUrl: `/api/download?file=${encodeURIComponent(restoredName)}`,
            crcMatch: crcMatch,
            stdout: stdout
          }));
        });
      });
    } catch (e) {
      res.writeHead(500, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ error: e.message }));
    }
  });
}

function handleInspectApi(req, res, engine) {
  parseMultipartBody(req, (err, parts) => {
    if (err) {
      res.writeHead(400, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ error: err.message }));
      return;
    }

    const archivePart = parts.find(p => p.filename);
    if (!archivePart) {
      res.writeHead(400, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ error: 'No archive provided' }));
      return;
    }

    const archivePath = path.join(WORKSPACE_DIR, 'inspect_' + archivePart.filename);
    fs.writeFileSync(archivePath, archivePart.data);

    execFile(engine, ['-i', archivePath], (err, stdout, stderr) => {
      try { fs.unlinkSync(archivePath); } catch (_) {}

      if (err) {
        res.writeHead(400, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ error: stderr || err.message }));
        return;
      }

      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ success: true, stdout }));
    });
  });
}

function handleOpenFileApi(req, res) {
  let body = '';
  req.on('data', chunk => body += chunk);
  req.on('end', () => {
    try {
      const data = JSON.parse(body);
      const targetPath = data.path;

      if (targetPath && fs.existsSync(targetPath)) {
        // Windows command to open or select file in Explorer
        spawn('explorer.exe', ['/select,', targetPath], { detached: true, stdio: 'ignore' });
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ success: true }));
        return;
      }
      res.writeHead(404, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ error: 'File not found' }));
    } catch (e) {
      res.writeHead(400, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ error: e.message }));
    }
  });
}

server.on('error', (err) => {
  if (err.code === 'EADDRINUSE') {
    console.error(`\n[!] Port ${PORT} is already in use.`);
    console.error(`    ByteShrink is likely already running! You can simply open:`);
    console.error(`    👉 http://localhost:${PORT} in your browser.\n`);
    process.exit(1);
  } else {
    throw err;
  }
});

server.listen(PORT, () => {
  console.log(`================================================================`);
  console.log(` ByteShrink — Smart File Compression Studio (Offline Server)`);
  console.log(` Server running locally at: http://localhost:${PORT}`);
  console.log(` Engine binary: ${getEnginePath() || 'Not found (Using client fallback)'}`);
  console.log(`================================================================`);
});
