/**
 * ByteShrink — Smart File Compression Studio
 * TreeCanvas: Interactive 2D HTML5 Canvas Huffman Tree Visualizer
 * Supports hierarchical tree layout, pan, zoom, edge bit labels, and node hover inspection.
 */

class HuffmanTreeCanvas {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    if (!this.canvas) return;
    this.ctx = this.canvas.getContext('2d');

    // View transform state
    this.scale = 1.0;
    this.minScale = 0.15;
    this.maxScale = 4.0;
    this.offsetX = 0;
    this.offsetY = 40;

    // Interaction state
    this.isDragging = false;
    this.hasDragged = false;
    this.startX = 0;
    this.startY = 0;
    this.hoveredNode = null;
    this.selectedNode = null;
    this.onNodeClick = null;

    // Tree data
    this.treeData = null;
    this.nodes = [];
    this.edges = [];

    // Visual theme colors (ByteShrink retro-terminal palette)
    this.colors = {
      bg: '#0F0F12',
      grid: '#18181E',
      edge: '#534F5E',
      edgeLabel0: '#FF5A0A',
      edgeLabel1: '#9A6CF9',
      internalNode: '#7844F9',
      internalNodeBorder: '#9A6CF9',
      leafNode: '#1F1E24',
      leafNodeBorder: '#FF5A0A',
      leafNodeHover: '#FF5A0A',
      textPrimary: '#F4F1F7',
      textSecondary: '#C6C2CE',
      textMuted: '#A5A0AE',
      accentOrange: '#FF5A0A',
      accentPurple: '#7844F9'
    };

    this.initEvents();
    this.resize();
  }

  initEvents() {
    window.addEventListener('resize', () => {
      this.resize();
      this.render();
    });

    // Mouse drag for panning
    this.canvas.addEventListener('mousedown', (e) => {
      this.isDragging = true;
      this.hasDragged = false;
      this.startX = e.clientX - this.offsetX;
      this.startY = e.clientY - this.offsetY;
      this.canvas.style.cursor = 'grabbing';
    });

    window.addEventListener('mousemove', (e) => {
      if (this.isDragging) {
        const dx = Math.abs(e.clientX - (this.startX + this.offsetX));
        const dy = Math.abs(e.clientY - (this.startY + this.offsetY));
        if (dx > 3 || dy > 3) {
          this.hasDragged = true;
        }
        this.offsetX = e.clientX - this.startX;
        this.offsetY = e.clientY - this.startY;
        this.render();
      } else {
        this.handleHover(e);
      }
    });

    window.addEventListener('mouseup', () => {
      if (this.isDragging) {
        this.isDragging = false;
        this.canvas.style.cursor = 'grab';
      }
    });

    // Node click to select & inspect
    this.canvas.addEventListener('click', (e) => {
      if (this.hasDragged) return;
      const rect = this.canvas.getBoundingClientRect();
      const mouseX = (e.clientX - rect.left - this.offsetX) / this.scale;
      const mouseY = (e.clientY - rect.top - this.offsetY) / this.scale;

      let clicked = null;
      for (let i = this.nodes.length - 1; i >= 0; i--) {
        const n = this.nodes[i];
        const r = n.isLeaf ? 24 : 20;
        const dist = Math.hypot(mouseX - n.x, mouseY - n.y);
        if (dist <= r) {
          clicked = n;
          break;
        }
      }

      this.selectedNode = clicked;
      this.render();

      if (clicked && typeof this.onNodeClick === 'function') {
        this.onNodeClick(clicked);
      }
    });

    // Mouse wheel for zooming
    this.canvas.addEventListener('wheel', (e) => {
      e.preventDefault();
      const rect = this.canvas.getBoundingClientRect();
      const mouseX = e.clientX - rect.left;
      const mouseY = e.clientY - rect.top;

      const zoomFactor = e.deltaY < 0 ? 1.15 : 0.87;
      this.zoomAt(mouseX, mouseY, zoomFactor);
    }, { passive: false });

    // Touch support for mobile devices
    let touchStartDist = 0;
    let touchStartMid = { x: 0, y: 0 };

    this.canvas.addEventListener('touchstart', (e) => {
      if (e.touches.length === 1) {
        this.isDragging = true;
        this.startX = e.touches[0].clientX - this.offsetX;
        this.startY = e.touches[0].clientY - this.offsetY;
      } else if (e.touches.length === 2) {
        this.isDragging = false;
        const dx = e.touches[0].clientX - e.touches[1].clientX;
        const dy = e.touches[0].clientY - e.touches[1].clientY;
        touchStartDist = Math.hypot(dx, dy);
        touchStartMid = {
          x: (e.touches[0].clientX + e.touches[1].clientX) / 2,
          y: (e.touches[0].clientY + e.touches[1].clientY) / 2
        };
      }
    }, { passive: true });

    this.canvas.addEventListener('touchmove', (e) => {
      if (e.touches.length === 1 && this.isDragging) {
        this.offsetX = e.touches[0].clientX - this.startX;
        this.offsetY = e.touches[0].clientY - this.startY;
        this.render();
      } else if (e.touches.length === 2) {
        const dx = e.touches[0].clientX - e.touches[1].clientX;
        const dy = e.touches[0].clientY - e.touches[1].clientY;
        const dist = Math.hypot(dx, dy);
        if (touchStartDist > 0) {
          const factor = dist / touchStartDist;
          touchStartDist = dist;
          const rect = this.canvas.getBoundingClientRect();
          this.zoomAt(touchStartMid.x - rect.left, touchStartMid.y - rect.top, factor);
        }
      }
    }, { passive: true });

    this.canvas.addEventListener('touchend', () => {
      this.isDragging = false;
      touchStartDist = 0;
    });

    // Canvas cursor
    this.canvas.style.cursor = 'grab';
  }

  resize() {
    const parent = this.canvas.parentElement;
    if (!parent) return;
    const rect = parent.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    this.width = rect.width;
    this.height = Math.max(rect.height || 500, 480);

    this.canvas.width = this.width * dpr;
    this.canvas.height = this.height * dpr;
    this.ctx.scale(dpr, dpr);
    this.canvas.style.width = `${this.width}px`;
    this.canvas.style.height = `${this.height}px`;
  }

  zoomAt(x, y, factor) {
    const newScale = Math.min(Math.max(this.scale * factor, this.minScale), this.maxScale);
    if (newScale === this.scale) return;

    // Adjust offset to keep mouse pointer stationary
    this.offsetX = x - (x - this.offsetX) * (newScale / this.scale);
    this.offsetY = y - (y - this.offsetY) * (newScale / this.scale);
    this.scale = newScale;

    this.updateZoomLabel();
    this.render();
  }

  zoom(factor) {
    this.zoomAt(this.width / 2, this.height / 2, factor);
  }

  resetView() {
    this.scale = 1.0;
    if (this.nodes.length > 0) {
      // Center the root
      const root = this.nodes[0];
      this.offsetX = this.width / 2 - root.x;
      this.offsetY = 60;
    } else {
      this.offsetX = 0;
      this.offsetY = 40;
    }
    this.updateZoomLabel();
    this.render();
  }

  fitToView() {
    if (!this.nodes || this.nodes.length === 0) return;

    let minX = Infinity, maxX = -Infinity;
    let minY = Infinity, maxY = -Infinity;

    for (const n of this.nodes) {
      const halfW = n.isLeaf ? 30 : 25;
      const halfH = n.isLeaf ? 26 : 25;
      minX = Math.min(minX, n.x - halfW);
      maxX = Math.max(maxX, n.x + halfW);
      minY = Math.min(minY, n.y - halfH);
      maxY = Math.max(maxY, n.y + halfH);
    }

    const treeW = Math.max(maxX - minX, 80);
    const treeH = Math.max(maxY - minY, 80);

    const pad = 50;
    const scaleX = (this.width - pad * 2) / treeW;
    const scaleY = (this.height - pad * 2) / treeH;

    // Constrain scale between minScale and 1.8 so small trees don't balloon unnaturally
    this.scale = Math.min(Math.max(Math.min(scaleX, scaleY), this.minScale), 1.7);

    const treeMidX = (minX + maxX) / 2;
    const treeMidY = (minY + maxY) / 2;

    this.offsetX = this.width / 2 - treeMidX * this.scale;
    this.offsetY = this.height / 2 - treeMidY * this.scale;

    this.updateZoomLabel();
    this.render();
  }

  selectNodeByByte(byteVal) {
    const node = this.nodes.find(n => n.isLeaf && n.byteVal === byteVal);
    if (node) {
      this.selectedNode = node;
      // Check if node is outside current viewport and pan if needed
      const screenX = node.x * this.scale + this.offsetX;
      const screenY = node.y * this.scale + this.offsetY;
      if (screenX < 60 || screenX > this.width - 60 || screenY < 60 || screenY > this.height - 60) {
        this.offsetX = this.width / 2 - node.x * this.scale;
        this.offsetY = this.height / 2 - node.y * this.scale;
      }
      this.render();
      return node;
    }
    return null;
  }

  updateZoomLabel() {
    const label = document.getElementById('canvasZoomLabel');
    if (label) {
      label.textContent = `Zoom: ${Math.round(this.scale * 100)}%`;
    }
  }

  handleHover(e) {
    const rect = this.canvas.getBoundingClientRect();
    const mouseX = (e.clientX - rect.left - this.offsetX) / this.scale;
    const mouseY = (e.clientY - rect.top - this.offsetY) / this.scale;

    let found = null;
    for (let i = this.nodes.length - 1; i >= 0; i--) {
      const n = this.nodes[i];
      const r = n.isLeaf ? 24 : 20;
      const dist = Math.hypot(mouseX - n.x, mouseY - n.y);
      if (dist <= r) {
        found = n;
        break;
      }
    }

    if (this.hoveredNode !== found) {
      this.hoveredNode = found;
      this.canvas.style.cursor = found ? 'pointer' : (this.isDragging ? 'grabbing' : 'grab');
      this.render();
    }
  }

  /**
   * Set tree hierarchy and calculate layout coordinates
   * treeData format:
   * {
   *   id, freq, symbol, code, isLeaf,
   *   left: { ... },
   *   right: { ... }
   * }
   */
  setTree(treeData) {
    this.treeData = treeData;
    this.nodes = [];
    this.edges = [];

    if (!treeData) {
      this.render();
      return;
    }

    // Assign depths & compute leaf count for spacing
    this.computeLeafIndices(treeData, 0);

    // Compute coordinate positions
    const leafSpacing = 68;
    const levelSpacing = 82;
    this.layoutNode(treeData, 0, leafSpacing, levelSpacing);

    // Flatten to list for rendering & hit testing
    this.flattenTree(treeData);

    // Center root initially
    this.resetView();
  }

  computeLeafIndices(node, depth) {
    if (!node) return 0;
    node.depth = depth;

    if (node.isLeaf || (!node.left && !node.right)) {
      node.isLeaf = true;
      node.leafCount = 1;
      return 1;
    }

    const leftLeaves = node.left ? this.computeLeafIndices(node.left, depth + 1) : 0;
    const rightLeaves = node.right ? this.computeLeafIndices(node.right, depth + 1) : 0;
    node.leafCount = leftLeaves + rightLeaves;
    return node.leafCount;
  }

  layoutNode(node, currentLeafOffset, leafSpacing, levelSpacing) {
    if (!node) return currentLeafOffset;

    const y = node.depth * levelSpacing + 30;

    if (node.isLeaf || (!node.left && !node.right)) {
      const x = (currentLeafOffset + 0.5) * leafSpacing;
      node.x = x;
      node.y = y;
      return currentLeafOffset + 1;
    }

    let nextOffset = currentLeafOffset;
    if (node.left) {
      nextOffset = this.layoutNode(node.left, nextOffset, leafSpacing, levelSpacing);
    }
    if (node.right) {
      nextOffset = this.layoutNode(node.right, nextOffset, leafSpacing, levelSpacing);
    }

    // Parent x is the midpoint of children
    const leftX = node.left ? node.left.x : 0;
    const rightX = node.right ? node.right.x : 0;
    node.x = (leftX + rightX) / 2;
    node.y = y;

    return nextOffset;
  }

  flattenTree(node) {
    if (!node) return;
    this.nodes.push(node);

    if (node.left) {
      this.edges.push({ from: node, to: node.left, bit: '0' });
      this.flattenTree(node.left);
    }
    if (node.right) {
      this.edges.push({ from: node, to: node.right, bit: '1' });
      this.flattenTree(node.right);
    }
  }

  render() {
    if (!this.ctx) return;
    const ctx = this.ctx;
    ctx.clearRect(0, 0, this.width, this.height);

    // Background fill
    ctx.fillStyle = this.colors.bg;
    ctx.fillRect(0, 0, this.width, this.height);

    // Subtle background grid
    ctx.save();
    ctx.strokeStyle = this.colors.grid;
    ctx.lineWidth = 1;
    const gridSize = 32 * this.scale;
    const startX = this.offsetX % gridSize;
    const startY = this.offsetY % gridSize;

    ctx.beginPath();
    for (let x = startX; x < this.width; x += gridSize) {
      ctx.moveTo(x, 0);
      ctx.lineTo(x, this.height);
    }
    for (let y = startY; y < this.height; y += gridSize) {
      ctx.moveTo(0, y);
      ctx.lineTo(this.width, y);
    }
    ctx.stroke();
    ctx.restore();

    // If no tree loaded, show empty state notice
    if (!this.treeData || this.nodes.length === 0) {
      ctx.save();
      ctx.fillStyle = this.colors.textMuted;
      ctx.font = '15px "Consolas", "Courier New", monospace';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText('[ NO TREE GENERATED — ENTER TEXT OR LOAD A FILE TO INSPECT ]', this.width / 2, this.height / 2);
      ctx.restore();
      return;
    }

    // Apply view transform
    ctx.save();
    ctx.translate(this.offsetX, this.offsetY);
    ctx.scale(this.scale, this.scale);

    // 1. Draw Edges
    for (const edge of this.edges) {
      this.drawEdge(ctx, edge);
    }

    // 2. Draw Nodes
    for (const node of this.nodes) {
      this.drawNode(ctx, node);
    }

    ctx.restore();

    // 3. Draw Hover Tooltip on top (unscaled screen space)
    if (this.hoveredNode) {
      this.drawTooltip(ctx, this.hoveredNode);
    }
  }

  drawEdge(ctx, edge) {
    const from = edge.from;
    const to = edge.to;
    const isBit0 = edge.bit === '0';

    ctx.save();
    ctx.beginPath();
    ctx.moveTo(from.x, from.y);
    ctx.lineTo(to.x, to.y);
    ctx.strokeStyle = this.colors.edge;
    ctx.lineWidth = 2;
    ctx.stroke();

    // Edge bit label badge ("0" or "1")
    const midX = (from.x + to.x) / 2;
    const midY = (from.y + to.y) / 2;

    const badgeRadius = 10;
    ctx.beginPath();
    ctx.arc(midX, midY, badgeRadius, 0, Math.PI * 2);
    ctx.fillStyle = isBit0 ? '#261208' : '#180E2B';
    ctx.fill();
    ctx.lineWidth = 1.5;
    ctx.strokeStyle = isBit0 ? this.colors.edgeLabel0 : this.colors.edgeLabel1;
    ctx.stroke();

    // Bit text
    ctx.fillStyle = isBit0 ? this.colors.accentOrange : this.colors.edgeLabel1;
    ctx.font = 'bold 11px "Consolas", monospace';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(edge.bit, midX, midY);

    ctx.restore();
  }

  drawNode(ctx, node) {
    const isHovered = this.hoveredNode === node;
    const isSelected = this.selectedNode === node;

    ctx.save();

    // Draw active selection indicator ring if selected
    if (isSelected) {
      ctx.save();
      ctx.beginPath();
      if (node.isLeaf) {
        ctx.roundRect(node.x - 29, node.y - 26, 58, 52, 9);
      } else {
        ctx.arc(node.x, node.y, 26, 0, Math.PI * 2);
      }
      ctx.lineWidth = 3;
      ctx.strokeStyle = node.isLeaf ? '#FF5A0A' : '#9A6CF9';
      ctx.stroke();
      ctx.restore();
    }

    if (node.isLeaf) {
      // Leaf Node: Orange accent box with symbol and frequency
      const w = 48;
      const h = 42;
      const x = node.x - w / 2;
      const y = node.y - h / 2;
      const r = 6;

      ctx.beginPath();
      ctx.roundRect(x, y, w, h, r);
      ctx.fillStyle = isHovered ? '#2E150B' : this.colors.leafNode;
      ctx.fill();

      ctx.lineWidth = isHovered ? 2.5 : 1.5;
      ctx.strokeStyle = isHovered ? '#FFA366' : this.colors.leafNodeBorder;
      ctx.stroke();

      // Display Symbol
      let dispChar = node.symbol;
      if (node.byteVal === 32 || node.symbol === ' ') dispChar = '␣';
      else if (node.byteVal === 10 || node.symbol === '\n') dispChar = '\\n';
      else if (node.byteVal === 13 || node.symbol === '\r') dispChar = '\\r';
      else if (node.byteVal === 9 || node.symbol === '\t') dispChar = '\\t';
      else if (node.byteVal !== undefined && (node.byteVal < 32 || node.byteVal > 126)) {
        dispChar = '0x' + node.byteVal.toString(16).toUpperCase().padStart(2, '0');
      }

      ctx.fillStyle = this.colors.accentOrange;
      ctx.font = 'bold 14px "Consolas", monospace';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText(dispChar, node.x, node.y - 7);

      // Frequency label
      ctx.fillStyle = this.colors.textSecondary;
      ctx.font = '10px "Consolas", monospace';
      ctx.fillText(`f:${node.freq}`, node.x, node.y + 11);

    } else {
      // Internal Node: Purple circle with combined frequency
      const radius = 20;

      ctx.beginPath();
      ctx.arc(node.x, node.y, radius, 0, Math.PI * 2);
      ctx.fillStyle = isHovered ? '#3D1C8C' : '#22153B';
      ctx.fill();

      ctx.lineWidth = isHovered ? 2.5 : 1.5;
      ctx.strokeStyle = isHovered ? '#BBA0F8' : this.colors.internalNodeBorder;
      ctx.stroke();

      // Combined frequency
      ctx.fillStyle = this.colors.textPrimary;
      ctx.font = 'bold 11px "Consolas", monospace';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText(node.freq, node.x, node.y);
    }

    ctx.restore();
  }

  drawTooltip(ctx, node) {
    // Screen coordinates of node
    const screenX = node.x * this.scale + this.offsetX;
    const screenY = node.y * this.scale + this.offsetY;

    ctx.save();
    const padding = 12;
    const lineHeight = 18;
    const lines = [];

    if (node.isLeaf) {
      let charRep = `'${node.symbol}'`;
      if (node.byteVal === 32) charRep = "' ' (SPACE)";
      else if (node.byteVal === 10) charRep = "\\n (LINE FEED)";
      else if (node.byteVal === 13) charRep = "\\r (CARRIAGE RETURN)";
      else if (node.byteVal === 9) charRep = "\\t (TAB)";
      else if (node.byteVal !== undefined && (node.byteVal < 32 || node.byteVal > 126)) {
        charRep = `0x${node.byteVal.toString(16).toUpperCase().padStart(2, '0')}`;
      }

      lines.push(`LEAF NODE: ${charRep}`);
      lines.push(`Frequency: ${node.freq} occurrence(s)`);
      if (node.code) {
        lines.push(`Huffman Code: ${node.code} (${node.code.length} bits)`);
      }
      lines.push(`Tree Depth: ${node.depth}`);
    } else {
      lines.push(`INTERNAL MERGED NODE`);
      lines.push(`Combined Freq: ${node.freq}`);
      lines.push(`Left Subtree Freq: ${node.left ? node.left.freq : 0}`);
      lines.push(`Right Subtree Freq: ${node.right ? node.right.freq : 0}`);
      lines.push(`Tree Depth: ${node.depth}`);
    }

    ctx.font = '12px "Consolas", monospace';
    let maxW = 0;
    for (const line of lines) {
      maxW = Math.max(maxW, ctx.measureText(line).width);
    }

    const boxW = maxW + padding * 2;
    const boxH = lines.length * lineHeight + padding * 2;

    let tipX = screenX + 25;
    let tipY = screenY - 25;

    // Boundary check
    if (tipX + boxW > this.width - 15) tipX = screenX - boxW - 25;
    if (tipY + boxH > this.height - 15) tipY = this.height - boxH - 15;
    if (tipY < 15) tipY = 15;

    // Tooltip backdrop
    ctx.fillStyle = '#121217';
    ctx.strokeStyle = node.isLeaf ? this.colors.accentOrange : this.colors.accentPurple;
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.roundRect(tipX, tipY, boxW, boxH, 6);
    ctx.fill();
    ctx.stroke();

    // Render lines
    ctx.textAlign = 'left';
    ctx.textBaseline = 'top';
    for (let i = 0; i < lines.length; i++) {
      ctx.fillStyle = (i === 0) 
        ? (node.isLeaf ? this.colors.accentOrange : '#BBA0F8') 
        : this.colors.textSecondary;
      ctx.fillText(lines[i], tipX + padding, tipY + padding + i * lineHeight);
    }

    ctx.restore();
  }
}

// Export to window
window.HuffmanTreeCanvas = HuffmanTreeCanvas;
