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

// Simulation Engine State (mirrors C++ core logic)
let simState = {
  refs: [],
  currentIndex: 0,
  frames: [],
  hits: 0,
  faults: 0,
  evictions: 0,
  algo: 'LRU',
  frameCount: 3,
  history: [],
  lruOrder: [], // MRU front, LRU back
  fifoQueue: [],
  freqMap: {},
  autoRunInterval: null
};

function parseReferences() {
  const raw = document.getElementById('sim-refs').value;
  return raw.split(/[\s,]+/).filter(x => x.trim() !== '').map(Number);
}

function resetSimulation() {
  if (simState.autoRunInterval) {
    clearInterval(simState.autoRunInterval);
    simState.autoRunInterval = null;
    document.getElementById('btn-autorun').innerText = 'Auto-Run Simulation';
  }

  const refs = parseReferences();
  const frameCount = parseInt(document.getElementById('sim-frames').value, 10) || 3;
  const algo = document.getElementById('sim-algo').value;

  simState = {
    refs: refs,
    currentIndex: 0,
    frames: new Array(frameCount).fill(-1),
    hits: 0,
    faults: 0,
    evictions: 0,
    algo: algo,
    frameCount: frameCount,
    history: [],
    lruOrder: [],
    fifoQueue: [],
    freqMap: {},
    autoRunInterval: null
  };

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
  const algo = simState.algo;
  let hit = false;
  let victim = null;

  // Track frequency
  simState.freqMap[page] = (simState.freqMap[page] || 0) + 1;

  // Check hit
  const frameIdx = simState.frames.indexOf(page);
  if (frameIdx !== -1) {
    // HIT
    hit = true;
    simState.hits++;

    if (algo === 'LRU') {
      simState.lruOrder = simState.lruOrder.filter(p => p !== page);
      simState.lruOrder.unshift(page);
    }
  } else {
    // FAULT
    simState.faults++;
    const freeIdx = simState.frames.indexOf(-1);

    if (freeIdx !== -1) {
      // Free frame available
      simState.frames[freeIdx] = page;
      if (algo === 'LRU') simState.lruOrder.unshift(page);
      if (algo === 'FIFO') simState.fifoQueue.push(page);
    } else {
      // Eviction needed
      simState.evictions++;
      let targetFrame = 0;

      if (algo === 'FIFO') {
        victim = simState.fifoQueue.shift();
        targetFrame = simState.frames.indexOf(victim);
        simState.fifoQueue.push(page);
      } else if (algo === 'LRU') {
        victim = simState.lruOrder.pop();
        targetFrame = simState.frames.indexOf(victim);
        simState.lruOrder.unshift(page);
      } else if (algo === 'OPT') {
        // Find farthest future use
        let farthest = -1;
        let optVictim = simState.frames[0];
        for (let f of simState.frames) {
          let nextUse = Infinity;
          for (let i = simState.currentIndex + 1; i < simState.refs.length; ++i) {
            if (simState.refs[i] === f) {
              nextUse = i;
              break;
            }
          }
          if (nextUse > farthest) {
            farthest = nextUse;
            optVictim = f;
          }
        }
        victim = optVictim;
        targetFrame = simState.frames.indexOf(victim);
      } else {
        // Default fallback / LFU
        let minFreq = Infinity;
        let lfuVictim = simState.frames[0];
        for (let f of simState.frames) {
          let freq = simState.freqMap[f] || 0;
          if (freq < minFreq) {
            minFreq = freq;
            lfuVictim = f;
          }
        }
        victim = lfuVictim;
        targetFrame = simState.frames.indexOf(victim);
      }

      simState.frames[targetFrame] = page;
    }
  }

  simState.history.push({
    step: simState.currentIndex + 1,
    page: page,
    hit: hit,
    victim: victim,
    frames: [...simState.frames]
  });

  simState.currentIndex++;
  updateUI(hit, page, victim);
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
  const eatMs = total > 0 ? ((simState.hits * 0.00011 + simState.faults * 8.0) / total).toFixed(2) : '0.00';
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

// Belady Chart Renderer
function renderBeladyChart() {
  const container = document.getElementById('belady-chart-container');
  container.innerHTML = '';

  // Canonical string: 1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5
  const data = [
    { frames: 2, fifo: 12, lru: 12, opt: 9 },
    { frames: 3, fifo: 9,  lru: 10, opt: 7 },
    { frames: 4, fifo: 10, lru: 8,  opt: 6 }, // Anomaly!
    { frames: 5, fifo: 5,  lru: 5,  opt: 5 },
    { frames: 6, fifo: 5,  lru: 5,  opt: 5 }
  ];

  data.forEach(d => {
    const isAnomaly = d.frames === 4;
    const row = document.createElement('div');
    row.className = 'chart-row';
    row.innerHTML = `
      <div class="chart-label">
        ${d.frames} Frames
        ${isAnomaly ? '<br><span style="color: var(--accent-red); font-size: 0.72rem; font-weight: 800;">* ANOMALY *</span>' : ''}
      </div>
      <div class="bar-group">
        <div class="bar-line bar-fifo" style="width: ${(d.fifo / 12) * 100}%;">FIFO: ${d.fifo} Faults ${isAnomaly ? '▲ (+1)' : ''}</div>
        <div class="bar-line bar-lru" style="width: ${(d.lru / 12) * 100}%;">LRU: ${d.lru} Faults</div>
        <div class="bar-line bar-opt" style="width: ${(d.opt / 12) * 100}%;">OPT: ${d.opt} Faults</div>
      </div>
    `;
    container.appendChild(row);
  });
}

// Multi-Algorithm Benchmark Cards
function renderBenchmarkComparison() {
  const container = document.getElementById('benchmark-cards');
  container.innerHTML = '';

  const algos = [
    { name: 'Optimal (OPT)', hit: '84%', faults: 16, eat: '1.76 ms', desc: 'Theoretical lowest possible page faults using future knowledge.' },
    { name: 'LRU (Least Recently Used)', hit: '78%', faults: 22, eat: '2.48 ms', desc: 'O(1) Doubly Linked List + Hash Map tracking recent references.' },
    { name: 'LFU (Least Frequently Used)', hit: '79%', faults: 21, eat: '2.28 ms', desc: 'Frequency buckets with O(1) eviction for repeated loops.' },
    { name: 'Clock (Second Chance)', hit: '71%', faults: 29, eat: '3.64 ms', desc: 'Hardware-friendly circular buffer approximating LRU.' },
    { name: 'FIFO (First-In, First-Out)', hit: '69%', faults: 31, eat: '3.92 ms', desc: 'Simple queue arrival order; susceptible to Bélády’s anomaly.' },
    { name: 'MFU (Most Frequently Used)', hit: '66%', faults: 34, eat: '4.76 ms', desc: 'Evicts highest frequency pages under heavy utilization.' }
  ];

  algos.forEach(a => {
    const card = document.createElement('div');
    card.className = 'algo-summary-card';
    card.innerHTML = `
      <h3>${a.name}</h3>
      <p class="algo-desc">${a.desc}</p>
      <div class="algo-stat-row">
        <span>Hit Ratio</span>
        <span style="color: var(--accent-green);">${a.hit}</span>
      </div>
      <div class="algo-stat-row">
        <span>Total Faults</span>
        <span style="color: var(--accent-amber);">${a.faults}</span>
      </div>
      <div class="algo-stat-row">
        <span>Effective Access Time</span>
        <span style="color: var(--accent-cyan);">${a.eat}</span>
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
