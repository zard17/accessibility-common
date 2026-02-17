#ifndef ACCESSIBILITY_TOOLS_INSPECTOR_WEB_INSPECTOR_RESOURCES_H
#define ACCESSIBILITY_TOOLS_INSPECTOR_WEB_INSPECTOR_RESOURCES_H

/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

namespace WebInspectorResources
{
/**
 * @brief Embedded HTML/CSS/JS for the web-based accessibility inspector.
 */
inline const char* HTML = R"HTMLPAGE(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Tizen Accessibility Inspector</title>
<style>
  :root {
    --bg-base: #1e1e2e;
    --bg-surface: #181825;
    --bg-overlay: #313244;
    --text-main: #cdd6f4;
    --text-sub: #a6adc8;
    --text-dim: #6c7086;
    --accent: #89b4fa;
    --accent-hover: #74c7ec;
    --green: #a6e3a1;
    --red: #f38ba8;
    --yellow: #f9e2af;
    --mauve: #cba6f7;
    --border: #45475a;
    --radius: 6px;
  }

  * { margin: 0; padding: 0; box-sizing: border-box; }

  body {
    font-family: 'SF Mono', 'Cascadia Code', 'Fira Code', monospace;
    background: var(--bg-base);
    color: var(--text-main);
    height: 100vh;
    display: flex;
    flex-direction: column;
  }

  header {
    background: var(--bg-surface);
    border-bottom: 1px solid var(--border);
    padding: 12px 20px;
    display: flex;
    align-items: center;
    gap: 16px;
    flex-shrink: 0;
  }

  header h1 {
    font-size: 16px;
    font-weight: 600;
    color: var(--accent);
    white-space: nowrap;
  }

  .nav-buttons {
    display: flex;
    gap: 6px;
  }

  .nav-buttons button {
    background: var(--bg-overlay);
    color: var(--text-main);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 6px 12px;
    font-family: inherit;
    font-size: 12px;
    cursor: pointer;
    transition: background 0.15s, border-color 0.15s;
  }

  .nav-buttons button:hover {
    background: var(--border);
    border-color: var(--accent);
  }

  .nav-buttons button:active {
    background: var(--accent);
    color: var(--bg-base);
  }

  .nav-buttons .kbd {
    font-size: 10px;
    color: var(--text-dim);
    margin-left: 4px;
  }

  .main-content {
    display: grid;
    grid-template-columns: 1fr 1fr;
    flex: 1;
    overflow: hidden;
  }

  .panel {
    display: flex;
    flex-direction: column;
    overflow: hidden;
  }

  .panel-header {
    background: var(--bg-surface);
    border-bottom: 1px solid var(--border);
    padding: 8px 16px;
    font-size: 12px;
    font-weight: 600;
    color: var(--text-sub);
    text-transform: uppercase;
    letter-spacing: 0.5px;
    flex-shrink: 0;
  }

  .panel-body {
    overflow-y: auto;
    padding: 8px;
    flex: 1;
  }

  .tree-panel {
    border-right: 1px solid var(--border);
  }

  /* Tree items */
  .tree-item {
    display: flex;
    align-items: center;
    padding: 4px 8px;
    border-radius: var(--radius);
    cursor: pointer;
    font-size: 13px;
    line-height: 1.6;
    transition: background 0.1s;
  }

  .tree-item:hover {
    background: var(--bg-overlay);
  }

  .tree-item.focused {
    background: rgba(137, 180, 250, 0.15);
    outline: 1px solid var(--accent);
  }

  .tree-toggle {
    width: 16px;
    height: 16px;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    font-size: 10px;
    color: var(--text-dim);
    cursor: pointer;
    flex-shrink: 0;
    user-select: none;
    transition: transform 0.15s;
  }

  .tree-toggle.collapsed {
    transform: rotate(-90deg);
  }

  .tree-toggle.leaf {
    visibility: hidden;
  }

  .tree-role {
    color: var(--mauve);
    font-weight: 600;
    margin-right: 6px;
  }

  .tree-name {
    color: var(--green);
  }

  .tree-children {
    padding-left: 20px;
  }

  .tree-children.collapsed {
    display: none;
  }

  /* Detail panel */
  .detail-section {
    margin-bottom: 16px;
  }

  .detail-section h3 {
    font-size: 12px;
    color: var(--text-sub);
    text-transform: uppercase;
    letter-spacing: 0.5px;
    margin-bottom: 8px;
    padding-bottom: 4px;
    border-bottom: 1px solid var(--border);
  }

  .detail-row {
    display: flex;
    padding: 4px 0;
    font-size: 13px;
  }

  .detail-label {
    color: var(--text-dim);
    width: 100px;
    flex-shrink: 0;
  }

  .detail-value {
    color: var(--text-main);
    word-break: break-word;
  }

  .state-badge {
    display: inline-block;
    background: var(--bg-overlay);
    border: 1px solid var(--border);
    border-radius: 3px;
    padding: 1px 6px;
    font-size: 11px;
    margin: 1px 3px 1px 0;
    color: var(--yellow);
  }

  .detail-empty {
    color: var(--text-dim);
    font-style: italic;
    padding: 20px;
    text-align: center;
  }

  #speak-btn {
    background: var(--accent);
    color: var(--bg-base);
    border: none;
    border-radius: var(--radius);
    padding: 8px 16px;
    font-family: inherit;
    font-size: 12px;
    font-weight: 600;
    cursor: pointer;
    margin-top: 8px;
    transition: background 0.15s;
  }

  #speak-btn:hover {
    background: var(--accent-hover);
  }

  .loading {
    color: var(--text-dim);
    padding: 20px;
    text-align: center;
  }

  @media (max-width: 768px) {
    .main-content {
      grid-template-columns: 1fr;
      grid-template-rows: 1fr 1fr;
    }
    .tree-panel {
      border-right: none;
      border-bottom: 1px solid var(--border);
    }
  }
</style>
</head>
<body>
  <header>
    <h1>Tizen Accessibility Inspector</h1>
    <div class="nav-buttons">
      <button onclick="navigate('prev')">Prev <span class="kbd">Shift+Tab</span></button>
      <button onclick="navigate('next')">Next <span class="kbd">Tab</span></button>
      <button onclick="navigate('child')">Child <span class="kbd">Enter</span></button>
      <button onclick="navigate('parent')">Parent <span class="kbd">Backspace</span></button>
      <button onclick="loadTree()">Refresh <span class="kbd">R</span></button>
    </div>
  </header>
  <div class="main-content">
    <div class="panel tree-panel">
      <div class="panel-header">Accessibility Tree</div>
      <div class="panel-body" id="tree-root">
        <div class="loading">Loading tree...</div>
      </div>
    </div>
    <div class="panel detail-panel">
      <div class="panel-header">Element Details</div>
      <div class="panel-body" id="detail-root">
        <div class="detail-empty">Select an element to view details</div>
      </div>
    </div>
  </div>

<script>
const API = {
  async getTree() {
    const r = await fetch('/api/tree');
    return r.json();
  },
  async getElement(id) {
    const r = await fetch('/api/element/' + id);
    return r.json();
  },
  async navigate(direction) {
    const r = await fetch('/api/navigate', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({direction: direction})
    });
    return r.json();
  }
};

let currentFocusedId = 0;

function renderTree(node, container) {
  const item = document.createElement('div');
  item.className = 'tree-item' + (node.id === currentFocusedId ? ' focused' : '');
  item.dataset.id = node.id;

  const hasChildren = node.children && node.children.length > 0;

  const toggle = document.createElement('span');
  toggle.className = 'tree-toggle' + (hasChildren ? '' : ' leaf');
  toggle.textContent = '\u25BC';

  const role = document.createElement('span');
  role.className = 'tree-role';
  role.textContent = '[' + node.role + ']';

  const name = document.createElement('span');
  name.className = 'tree-name';
  name.textContent = '"' + node.name + '"';

  item.appendChild(toggle);
  item.appendChild(role);
  item.appendChild(name);

  item.addEventListener('click', function(e) {
    if (e.target === toggle && hasChildren) {
      e.stopPropagation();
      const childContainer = item.nextElementSibling;
      if (childContainer && childContainer.classList.contains('tree-children')) {
        childContainer.classList.toggle('collapsed');
        toggle.classList.toggle('collapsed');
      }
      return;
    }
    selectElement(node.id);
  });

  container.appendChild(item);

  if (hasChildren) {
    const childContainer = document.createElement('div');
    childContainer.className = 'tree-children';
    node.children.forEach(function(child) {
      renderTree(child, childContainer);
    });
    container.appendChild(childContainer);
  }
}

function renderDetail(info) {
  const root = document.getElementById('detail-root');
  root.innerHTML = '';

  // Identity section
  const identSec = createSection('Identity');
  addRow(identSec, 'ID', info.id);
  addRow(identSec, 'Name', info.name);
  addRow(identSec, 'Role', info.role);
  if (info.description) {
    addRow(identSec, 'Description', info.description);
  }
  root.appendChild(identSec);

  // States section
  const statesSec = createSection('States');
  const statesDiv = document.createElement('div');
  statesDiv.style.padding = '4px 0';
  const stateList = info.states.split(', ');
  stateList.forEach(function(s) {
    if (s && s !== '(none)') {
      const badge = document.createElement('span');
      badge.className = 'state-badge';
      badge.textContent = s;
      statesDiv.appendChild(badge);
    }
  });
  if (statesDiv.children.length === 0) {
    statesDiv.textContent = '(none)';
    statesDiv.style.color = 'var(--text-dim)';
  }
  statesSec.appendChild(statesDiv);
  root.appendChild(statesSec);

  // Bounds section
  const boundsSec = createSection('Bounds');
  addRow(boundsSec, 'Position', info.boundsX + ', ' + info.boundsY);
  addRow(boundsSec, 'Size', info.boundsWidth + ' x ' + info.boundsHeight);
  root.appendChild(boundsSec);

  // Tree section
  const treeSec = createSection('Tree');
  addRow(treeSec, 'Children', info.childCount);
  addRow(treeSec, 'Parent ID', info.parentId || '(root)');
  root.appendChild(treeSec);

  // Speak button
  const btn = document.createElement('button');
  btn.id = 'speak-btn';
  btn.textContent = 'Speak (S)';
  btn.onclick = function() { speak(info.role + '. ' + info.name); };
  root.appendChild(btn);
}

function createSection(title) {
  const sec = document.createElement('div');
  sec.className = 'detail-section';
  const h = document.createElement('h3');
  h.textContent = title;
  sec.appendChild(h);
  return sec;
}

function addRow(container, label, value) {
  const row = document.createElement('div');
  row.className = 'detail-row';
  const l = document.createElement('span');
  l.className = 'detail-label';
  l.textContent = label;
  const v = document.createElement('span');
  v.className = 'detail-value';
  v.textContent = value;
  row.appendChild(l);
  row.appendChild(v);
  container.appendChild(row);
}

function speak(text) {
  if ('speechSynthesis' in window) {
    window.speechSynthesis.cancel();
    const u = new SpeechSynthesisUtterance(text);
    window.speechSynthesis.speak(u);
  }
}

async function selectElement(id) {
  currentFocusedId = id;
  // Update focus highlight in tree
  document.querySelectorAll('.tree-item').forEach(function(el) {
    el.classList.toggle('focused', parseInt(el.dataset.id) === id);
  });
  // Fetch and show details
  const info = await API.getElement(id);
  renderDetail(info);
}

async function navigate(direction) {
  const result = await API.navigate(direction);
  if (result.focusedId) {
    currentFocusedId = result.focusedId;
    // Update tree highlight
    document.querySelectorAll('.tree-item').forEach(function(el) {
      el.classList.toggle('focused', parseInt(el.dataset.id) === currentFocusedId);
    });
    // Scroll focused element into view
    const focused = document.querySelector('.tree-item.focused');
    if (focused) focused.scrollIntoView({block: 'nearest', behavior: 'smooth'});
    // Show element details
    if (result.element) {
      renderDetail(result.element);
    }
  }
}

async function loadTree() {
  const treeRoot = document.getElementById('tree-root');
  treeRoot.innerHTML = '<div class="loading">Loading tree...</div>';
  try {
    const data = await API.getTree();
    currentFocusedId = data.focusedId;
    treeRoot.innerHTML = '';
    renderTree(data.tree, treeRoot);
    // Load details for focused element
    if (currentFocusedId) {
      const info = await API.getElement(currentFocusedId);
      renderDetail(info);
    }
  } catch(e) {
    treeRoot.innerHTML = '<div class="loading">Error loading tree: ' + e.message + '</div>';
  }
}

// Keyboard shortcuts
document.addEventListener('keydown', function(e) {
  // Skip if user is typing in an input
  if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') return;

  if (e.key === 'Tab') {
    e.preventDefault();
    navigate(e.shiftKey ? 'prev' : 'next');
  } else if (e.key === 'Enter') {
    e.preventDefault();
    navigate('child');
  } else if (e.key === 'Backspace') {
    e.preventDefault();
    navigate('parent');
  } else if (e.key === 's' || e.key === 'S') {
    const btn = document.getElementById('speak-btn');
    if (btn) btn.click();
  } else if (e.key === 'r' || e.key === 'R') {
    loadTree();
  }
});

// Initialize
loadTree();
</script>
</body>
</html>
)HTMLPAGE";

} // namespace WebInspectorResources

#endif // ACCESSIBILITY_TOOLS_INSPECTOR_WEB_INSPECTOR_RESOURCES_H
