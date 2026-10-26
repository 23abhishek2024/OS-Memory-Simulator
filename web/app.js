// Tab switching
function switchTab(tabId) {
  document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
  document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));
  
  const activeBtn = Array.from(document.querySelectorAll('.tab-btn')).find(b => b.getAttribute('onclick').includes(tabId));
  if (activeBtn) activeBtn.classList.add('active');
  
  document.getElementById('tab-' + tabId).classList.add('active');
  if (tabId === 'belady') renderBeladyChart();
  if (tabId === 'benchmark') renderBenchmarkComparison();
  if (tabId === 'simulator') drawDonutChart();
}

// Presets
function loadPreset() {
  const p = document.getElementById('sim-preset').value;
  const r = document.getElementById('sim-refs');
  if (p === 'textbook') r.value = '7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1';
  else if (p === 'belady') r.value = '1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5';
  else if (p === 'locality') r.value = '1, 2, 1, 3, 1, 2, 4, 1, 2, 5, 1, 2';
  else if (p === 'cyclic') r.value = '1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4';
  resetSimulation();
}

// ─────────────────────────────────────────────────────────────
// Simulation core: the same replacement rules as the C++ engine
// (src/PageReplacement.cpp), so dashboard numbers match the CLI.
// ─────────────────────────────────────────────────────────────
const ALGORITHMS = ['OPT', 'LRU', 'LFU', 'FIFO', 'LIFO', 'MFU'];

const ALGO_INFO = {
  FIFO: { name: 'FIFO (First-In, First-Out)', desc: 'Evicts the page loaded earliest. Simple queue; can show Bélády’s anomaly.' },
  LIFO: { name: 'LIFO (Last-In, First-Out)', desc: 'Evicts the page loaded most recently. Simple stack; the first pages loaded never leave.' },
  LRU: { name: 'LRU (Least Recently Used)', desc: 'Evicts the page unused for the longest time. O(1) with a doubly linked list + hash map.' },
  LFU: { name: 'LFU (Least Frequently Used)', desc: 'Evicts the page with the fewest references. O(1) with frequency buckets.' },
  MFU: { name: 'MFU (Most Frequently Used)', desc: 'Evicts the page with the most references. O(F) scan of resident pages.' },
  OPT: { name: 'Optimal (OPT)', desc: 'Evicts the page used farthest in the future. Theoretical minimum; needs future knowledge.' }
};

function createAlgoState(algo, frameCount) {
  return {
    algo: algo,
    frames: new Array(frameCount).fill(-1),
    loadOrder: [], // oldest load first (FIFO victim = first, LIFO victim = last)
    recency: [],   // most recently used first (LRU victim = last)
    freq: {},      // page -> reference count while resident (LFU / MFU)
    freqSince: {}, // page -> time it reached its current count (LFU tie-break, like the C++ buckets)
    lastUsed: {},  // page -> last access time (MFU tie-break)
    clock: 0
  };
}

function chooseVictim(st, refs, index) {
  const resident = st.frames.filter(p => p !== -1);

  if (st.algo === 'FIFO') return st.loadOrder[0];
  if (st.algo === 'LIFO') return st.loadOrder[st.loadOrder.length - 1];
  if (st.algo === 'LRU') return st.recency[st.recency.length - 1];

  if (st.algo === 'LFU') {
    // Lowest count; tie -> the page that reached that count first
    let victim = resident[0];
    for (const p of resident) {
      if (st.freq[p] < st.freq[victim] ||
          (st.freq[p] === st.freq[victim] && st.freqSince[p] < st.freqSince[victim])) {
        victim = p;
      }
    }
    return victim;
  }

  if (st.algo === 'MFU') {
    // Highest count; tie -> least recently used
    let victim = resident[0];
    for (const p of resident) {
      if (st.freq[p] > st.freq[victim] ||
          (st.freq[p] === st.freq[victim] && st.lastUsed[p] < st.lastUsed[victim])) {
        victim = p;
      }
    }
    return victim;
  }

  // OPT: the page whose next use is farthest away (or never used again)
  let victim = resident[0];
  let farthest = -1;
  for (const p of resident) {
    let nextUse = Infinity;
    for (let i = index + 1; i < refs.length; ++i) {
      if (refs[i] === p) { nextUse = i; break; }
    }
    if (nextUse > farthest) { farthest = nextUse; victim = p; }
  }
  return victim;
}

// Process one page reference. Updates the state and returns { hit, victim }.
function accessPage(st, page, refs, index) {
  st.clock++;

  if (st.frames.includes(page)) {
    st.recency = [page, ...st.recency.filter(p => p !== page)];
    st.freq[page]++;
    st.freqSince[page] = st.clock;
    st.lastUsed[page] = st.clock;
    return { hit: true, victim: null };
  }

  let target = st.frames.indexOf(-1);
  let victim = null;
  if (target === -1) {
    victim = chooseVictim(st, refs, index);
    target = st.frames.indexOf(victim);
    st.loadOrder = st.loadOrder.filter(p => p !== victim);
    st.recency = st.recency.filter(p => p !== victim);
    delete st.freq[victim];
    delete st.freqSince[victim];
    delete st.lastUsed[victim];
  }

  st.frames[target] = page;
  st.loadOrder.push(page);
  st.recency.unshift(page);
  st.freq[page] = 1;
  st.freqSince[page] = st.clock;
  st.lastUsed[page] = st.clock;
  return { hit: false, victim: victim };
}

// Run a whole reference string (used by the Benchmark and Bélády tabs)
function simulate(algo, refs, frameCount) {
  const st = createAlgoState(algo, frameCount);
  let hits = 0, faults = 0, evictions = 0;
  refs.forEach((page, i) => {
    const r = accessPage(st, page, refs, i);
    if (r.hit) hits++; else faults++;
    if (r.victim !== null) evictions++;
  });
  return { hits, faults, evictions };
}

// Effective access time in ms, same model as the C++ metrics:
// hit = 110 ns (10 ns TLB + 100 ns RAM), fault = 8 ms disk + 110 ns
function effectiveAccessTimeMs(hits, faults) {
  const total = hits + faults;
  if (total === 0) return 0;
  return (hits * 110 + faults * 8000110) / total / 1e6;
}

// ─────────────────────────────────────────────────────────────
// Interactive simulator (Tab 1)
// ─────────────────────────────────────────────────────────────
let simState = null;

function parseReferences() {
  const raw = document.getElementById('sim-refs').value;
  return raw.split(/[\s,]+/).filter(x => x.trim() !== '').map(Number);
}

function resetSimulation() {
  if (simState && simState.autoRunInterval) {
    clearInterval(simState.autoRunInterval);
    document.getElementById('btn-autorun').innerText = 'Auto-Run Simulation';
  }

  const refs = parseReferences();
  const frameCount = parseInt(document.getElementById('sim-frames').value, 10) || 3;
  const algo = document.getElementById('sim-algo').value;

  simState = {
    refs: refs,
    currentIndex: 0,
    hits: 0,
    faults: 0,
    evictions: 0,
    algo: algo,
    frameCount: frameCount,
    history: [],
    autoRunInterval: null,
    core: createAlgoState(algo, frameCount)
  };
  simState.frames = simState.core.frames; // same array, updated by accessPage

  updateUI();
  renderTape();
  drawDonutChart();
}

function stepSimulation() {
  if (simState.currentIndex >= simState.refs.length) {
    if (simState.autoRunInterval) {
      clearInterval(simState.autoRunInterval);
      simState.autoRunInterval = null;
      document.getElementById('btn-autorun').innerText = 'Auto-Run Simulation';
    }
    return;
  }

  const page = simState.refs[simState.currentIndex];
  const result = accessPage(simState.core, page, simState.refs, simState.currentIndex);

  if (result.hit) simState.hits++; else simState.faults++;
  if (result.victim !== null) simState.evictions++;

  simState.history.push({
    step: simState.currentIndex + 1,
    page: page,
    hit: result.hit,
    victim: result.victim,
    frames: [...simState.frames]
  });

  simState.currentIndex++;
  updateUI(result.hit, page, result.victim);
  drawDonutChart();
}

function toggleAutoRun() {
  if (simState.autoRunInterval) {
    clearInterval(simState.autoRunInterval);
    simState.autoRunInterval = null;
    document.getElementById('btn-autorun').innerText = 'Auto-Run Simulation';
  } else {
    document.getElementById('btn-autorun').innerText = 'Pause Simulation';
    simState.autoRunInterval = setInterval(() => {
      if (simState.currentIndex < simState.refs.length) {
        stepSimulation();
      } else {
        clearInterval(simState.autoRunInterval);
        simState.autoRunInterval = null;
        document.getElementById('btn-autorun').innerText = 'Auto-Run Simulation';
      }
    }, 500);
  }
}

function drawDonutChart() {
  const canvas = document.getElementById('donutChart');
  if (!canvas) return;
  const ctx = canvas.getContext('2d');
  const total = simState.hits + simState.faults;
  const hitPct = total > 0 ? (simState.hits / total) : 0;
  
  document.getElementById('donut-pct').innerText = total > 0 ? `${(hitPct * 100).toFixed(0)}%` : '0%';

  ctx.clearRect(0, 0, canvas.width, canvas.height);
  const centerX = canvas.width / 2;
  const centerY = canvas.height / 2;
  const radius = 54;
  const lineWidth = 14;

  // Draw background track
  ctx.beginPath();
  ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI);
  ctx.strokeStyle = 'rgba(255, 255, 255, 0.08)';
  ctx.lineWidth = lineWidth;
  ctx.stroke();

  if (total === 0) return;

  const hitAngle = hitPct * 2 * Math.PI;

  // Draw Fault Arc (Amber)
  ctx.beginPath();
  ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI);
  ctx.strokeStyle = '#f59e0b';
  ctx.lineWidth = lineWidth;
  ctx.stroke();

  // Draw Hit Arc (Green)
  if (hitPct > 0) {
    ctx.beginPath();
    ctx.arc(centerX, centerY, radius, -Math.PI / 2, (-Math.PI / 2) + hitAngle);
    ctx.strokeStyle = '#10b981';
    ctx.lineWidth = lineWidth;
    ctx.lineCap = 'round';
    ctx.stroke();
  }
}

function updateUI(lastHit = null, accessedPage = null, victim = null) {
  // Stats
  const total = simState.currentIndex;
  document.getElementById('stat-hits').innerText = simState.hits;
  document.getElementById('stat-faults').innerText = simState.faults;
  document.getElementById('stat-evictions').innerText = simState.evictions;

  const hitPct = total > 0 ? ((simState.hits / total) * 100).toFixed(1) : '0.0';
  const faultPct = total > 0 ? ((simState.faults / total) * 100).toFixed(1) : '0.0';
  document.getElementById('stat-hit-ratio').innerText = `Hit Ratio: ${hitPct}%`;
  document.getElementById('stat-fault-ratio').innerText = `Fault Ratio: ${faultPct}%`;

  // EAT in ms
  const eatMs = effectiveAccessTimeMs(simState.hits, simState.faults).toFixed(2);
  document.getElementById('stat-eat').innerHTML = `${eatMs} <span style="font-size: 1rem;">ms</span>`;

  document.getElementById('step-indicator').innerText = `Step ${simState.currentIndex} of ${simState.refs.length}`;

  if (victim !== null) {
    document.getElementById('victim-status').innerHTML = `<span style="color: var(--accent-red); font-weight: 700;">Evicted Page ${victim}</span> to make room for Page ${accessedPage}`;
  } else if (lastHit === true) {
    document.getElementById('victim-status').innerHTML = `<span style="color: var(--accent-green); font-weight: 700;">Page ${accessedPage} HIT in memory!</span>`;
  } else if (lastHit === false) {
    document.getElementById('victim-status').innerHTML = `<span style="color: var(--accent-amber); font-weight: 700;">Page ${accessedPage} FAULT -> Loaded into Free Frame</span>`;
  }

  // Render Frames
  const framesGrid = document.getElementById('frames-grid');
  framesGrid.innerHTML = '';
  for (let f = 0; f < simState.frameCount; ++f) {
    const val = simState.frames[f];
    const isOccupied = val !== -1;
    const isAccessed = isOccupied && val === accessedPage;
    
    let extraClass = '';
    if (isAccessed) {
      extraClass = lastHit ? 'just-accessed-hit' : 'just-accessed-fault';
    }

    const div = document.createElement('div');
    div.className = `frame-slot ${isOccupied ? 'occupied' : ''} ${extraClass}`;
    div.innerHTML = `
      <div class="frame-header">Physical Frame ${f}</div>
      <div class="frame-page">${isOccupied ? val : '[Empty]'}</div>
      <div class="frame-meta">${isOccupied ? 'Valid · 4 KB Frame' : 'Available'}</div>
    `;
    framesGrid.appendChild(div);
  }

  // Render Table Head and Rows
  const headRow = document.getElementById('table-head-row');
  headRow.innerHTML = '<th>Step</th><th>Page Ref</th><th>Status</th><th>Victim Evicted</th>';
  for (let f = 0; f < simState.frameCount; ++f) {
    headRow.innerHTML += `<th>Frame ${f}</th>`;
  }

  const tbody = document.getElementById('table-body');
  tbody.innerHTML = '';
  for (let log of simState.history) {
    const tr = document.createElement('tr');
    const badge = log.hit ? '<span class="badge-hit">[HIT]</span>' : '<span class="badge-fault">[FAULT]</span>';
    const vic = log.victim !== null ? `Page ${log.victim}` : '-';

    let framesHtml = '';
    for (let f = 0; f < simState.frameCount; ++f) {
      const p = log.frames[f];
      framesHtml += `<td>${p !== -1 ? p : '-'}</td>`;
    }

    tr.innerHTML = `
      <td>${log.step}</td>
      <td style="font-weight: 700; color: #fff;">${log.page}</td>
      <td>${badge}</td>
      <td>${vic}</td>
      ${framesHtml}
    `;
    tbody.appendChild(tr);
  }

  // Highlight Tape
  const tape = document.getElementById('stream-tape');
  Array.from(tape.children).forEach((node, idx) => {
    node.classList.remove('current', 'hit', 'fault');
    if (idx === simState.currentIndex) {
      node.classList.add('current');
      node.scrollIntoView({ behavior: 'smooth', block: 'nearest', inline: 'center' });
    } else if (idx < simState.currentIndex) {
      const hist = simState.history[idx];
      if (hist) {
        node.classList.add(hist.hit ? 'hit' : 'fault');
      }
    }
  });
}

function renderTape() {
  const tape = document.getElementById('stream-tape');
  tape.innerHTML = '';
  simState.refs.forEach((p, idx) => {
    const div = document.createElement('div');
    div.className = `stream-node ${idx === 0 ? 'current' : ''}`;
    div.innerText = p;
    tape.appendChild(div);
  });
}

// ─────────────────────────────────────────────────────────────
// Bélády's anomaly (Tab 2): computed live with the simulator
// ─────────────────────────────────────────────────────────────
const BELADY_STRING = [1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5];

function renderBeladyChart() {
  const container = document.getElementById('belady-chart-container');
  container.innerHTML = '';

  const data = [];
  for (let frames = 2; frames <= 6; ++frames) {
    data.push({
      frames: frames,
      fifo: simulate('FIFO', BELADY_STRING, frames).faults,
      lru: simulate('LRU', BELADY_STRING, frames).faults,
      opt: simulate('OPT', BELADY_STRING, frames).faults
    });
  }
  const maxFaults = Math.max(...data.map(d => Math.max(d.fifo, d.lru, d.opt)));

  data.forEach((d, i) => {
    const increase = i > 0 ? d.fifo - data[i - 1].fifo : 0;
    const isAnomaly = increase > 0; // more frames but MORE faults
    const row = document.createElement('div');
    row.className = 'chart-row';
    row.innerHTML = `
      <div class="chart-label">
        ${d.frames} Frames
        ${isAnomaly ? '<br><span style="color: var(--accent-red); font-size: 0.72rem; font-weight: 800;">* ANOMALY *</span>' : ''}
      </div>
      <div class="bar-group">
        <div class="bar-line bar-fifo" style="width: ${(d.fifo / maxFaults) * 100}%;">FIFO: ${d.fifo} Faults ${isAnomaly ? `▲ (+${increase})` : ''}</div>
        <div class="bar-line bar-lru" style="width: ${(d.lru / maxFaults) * 100}%;">LRU: ${d.lru} Faults</div>
        <div class="bar-line bar-opt" style="width: ${(d.opt / maxFaults) * 100}%;">OPT: ${d.opt} Faults</div>
      </div>
    `;
    container.appendChild(row);
  });
}

// ─────────────────────────────────────────────────────────────
// Benchmark (Tab 3): all 6 algorithms, computed live on the
// reference string and frame count from the Simulator tab
// ─────────────────────────────────────────────────────────────
function renderBenchmarkComparison() {
  const container = document.getElementById('benchmark-cards');
  container.innerHTML = '';

  const refs = parseReferences();
  const frameCount = parseInt(document.getElementById('sim-frames').value, 10) || 3;

  const subtitle = document.getElementById('benchmark-subtitle');
  if (subtitle) {
    subtitle.textContent = `All 6 algorithms on your current reference string (${refs.length} references) with ${frameCount} frames, sorted by fewest page faults.`;
  }

  if (refs.length === 0) {
    container.innerHTML = '<p style="color: var(--text-muted);">Enter a reference string in the Simulator tab first.</p>';
    return;
  }

  const results = ALGORITHMS
    .map(algo => Object.assign({ algo: algo }, simulate(algo, refs, frameCount)))
    .sort((a, b) => a.faults - b.faults);

  results.forEach(r => {
    const info = ALGO_INFO[r.algo];
    const hitPct = ((r.hits / refs.length) * 100).toFixed(1);
    const eat = effectiveAccessTimeMs(r.hits, r.faults).toFixed(2);
    const card = document.createElement('div');
    card.className = 'algo-summary-card';
    card.innerHTML = `
      <h3>${info.name}</h3>
      <p class="algo-desc">${info.desc}</p>
      <div class="algo-stat-row">
        <span>Hit Ratio</span>
        <span style="color: var(--accent-green);">${hitPct}%</span>
      </div>
      <div class="algo-stat-row">
        <span>Total Faults</span>
        <span style="color: var(--accent-amber);">${r.faults}</span>
      </div>
      <div class="algo-stat-row">
        <span>Effective Access Time</span>
        <span style="color: var(--accent-cyan);">${eat} ms</span>
      </div>
    `;
    container.appendChild(card);
  });
}

// Initialize on load
window.addEventListener('DOMContentLoaded', () => {
  resetSimulation();
  renderBeladyChart();
  renderBenchmarkComparison();
  drawDonutChart();
});
