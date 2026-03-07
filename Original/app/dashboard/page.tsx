'use client';

import React, { useState, useEffect, useCallback } from 'react';

interface TodoItem {
    id: string;
    text: string;
    completed: boolean;
}

interface SystemInfo {
    hostname: string;
    localIp: string;
    system: {
        uptime: number;
        platform: string;
        release: string;
        arch: string;
    };
}

export default function Dashboard() {
    const [todos, setTodos] = useState<TodoItem[]>([]);
    const [newTodo, setNewTodo] = useState('');
    const [systemInfo, setSystemInfo] = useState<SystemInfo | null>(null);
    const [settings, setSettings] = useState({
        nightMode: false,
        glowColor: 'green',
        burninGuard: false,
        burninCrtMode: false,
        fontWeight: 400,
        secondsScale: 82,
        thinOverride: false,
        monoEnforce: true,
        fontMain: 'var(--font-jetbrains-mono)',
        fontUI: 'var(--font-geist-sans)',
        reloadCounter: 0
    });

    const fetchData = useCallback(async () => {
        try {
            const t = Date.now();
            const [todoRes, settingsRes, systemRes] = await Promise.all([
                fetch(`/api/todos?t=${t}`),
                fetch(`/api/settings?t=${t}`),
                fetch(`/api/system?t=${t}`)
            ]);

            if (todoRes.ok) setTodos(await todoRes.json());
            if (systemRes.ok) setSystemInfo(await systemRes.json());

            if (settingsRes.ok) {
                const s = await settingsRes.json();
                setSettings(prev => ({ ...prev, ...s }));
                document.documentElement.dataset.theme = s.nightMode ? 'night' : 'light';
                document.documentElement.dataset.glowColor = s.glowColor;
            }
        } catch (e) {
            console.error('Failed to fetch dashboard data', e);
        }
    }, []);

    // SSE for settings, Poll for system info
    useEffect(() => {
        fetchData(); // Initial load

        let eventSource: EventSource | null = new EventSource('/api/sync');

        eventSource.onmessage = (event) => {
            try {
                const { type, data } = JSON.parse(event.data);
                if (type === 'sync') {
                    setSettings(prev => ({ ...prev, ...data }));
                    document.documentElement.dataset.theme = data.nightMode ? 'night' : 'light';
                    document.documentElement.dataset.glowColor = data.glowColor;
                }
            } catch (e) { }
        };

        const interval = setInterval(async () => {
            try {
                const res = await fetch(`/api/system?t=${Date.now()}`);
                if (res.ok) setSystemInfo(await res.json());
                const todoRes = await fetch(`/api/todos?t=${Date.now()}`);
                if (todoRes.ok) {
                    const todoData = await todoRes.json();
                    setTodos(todoData);
                }
            } catch (e) {
                console.error('Dashboard interval fetch failed', e);
            }
        }, 2000);

        return () => {
            eventSource?.close();
            clearInterval(interval);
        };
    }, [fetchData]);

    const addTodo = async (e: React.FormEvent) => {
        e.preventDefault();
        if (!newTodo.trim()) return;
        const res = await fetch('/api/todos', {
            method: 'POST',
            body: JSON.stringify({ action: 'add', text: newTodo })
        });
        if (res.ok) {
            setNewTodo('');
            setTodos(await res.json());
        }
    };

    const toggleTodo = async (id: string) => {
        const res = await fetch('/api/todos', {
            method: 'POST',
            body: JSON.stringify({ action: 'toggle', id })
        });
        if (res.ok) setTodos(await res.json());
    };

    const deleteTodo = async (id: string) => {
        const res = await fetch('/api/todos', {
            method: 'POST',
            body: JSON.stringify({ action: 'delete', id })
        });
        if (res.ok) setTodos(await res.json());
    };

    const updateSetting = async (key: string, value: any) => {
        console.log(`[Dashboard] Updating ${key} to ${value}`);
        setSettings(prev => ({ ...prev, [key]: value }));

        if (key === 'nightMode') document.documentElement.dataset.theme = value ? 'night' : 'light';
        if (key === 'glowColor') document.documentElement.dataset.glowColor = value;

        try {
            const res = await fetch('/api/settings', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ [key]: value })
            });
            if (!res.ok) {
                const errorText = await res.text();
                throw new Error(`Sync failure: ${res.status} ${errorText}`);
            }
            console.log('[Dashboard] Server sync successful');
        } catch (e) {
            console.error('[Dashboard] Setting update failed:', e);
        }
    };

    const formatUptime = (seconds: number) => {
        const h = Math.floor(seconds / 3600);
        const m = Math.floor((seconds % 3600) / 60);
        return `${h}h ${m}m`;
    };

    return (
        <div
            id="app"
            className="dashboard-root"
            style={{
                '--font-main': settings.fontMain,
                '--font-ui': settings.fontUI
            } as any}
        >
            <div className="dashboard-container">

                {/* Header Section */}
                <header className="dashboard-header">
                    <div className="section-label" style={{ fontSize: '18px' }}>DP-05 MGMT INTERFACE</div>
                    <div className="header-meta">
                        <span className="sync-status" style={{
                            color: 'var(--glow-green)',
                            marginRight: '12px',
                            opacity: settings.reloadCounter % 2 === 0 ? 1 : 0.7
                        }}>● LIVE_SYNC</span>
                        VER 2.5 / {systemInfo?.system.platform.toUpperCase() || '--'}
                    </div>
                </header>

                {/* System Status Row */}
                <div className="status-grid">
                    <div className="plate-concave status-card">
                        <div className="section-label">SERVER STATUS</div>
                        <div className="status-value">{systemInfo?.hostname || 'FETCHING...'}</div>
                        <div className="status-sub">IP: {systemInfo?.localIp || '--'}</div>
                    </div>
                    <div className="plate-concave status-card">
                        <div className="section-label">UPTIME</div>
                        <div className="status-value">
                            {systemInfo ? formatUptime(systemInfo.system.uptime) : '--'}
                        </div>
                        <div className="status-sub">ARCH: {systemInfo?.system.arch || '--'}</div>
                    </div>
                </div>

                {/* Controls Section */}
                <section className="controls-grid">

                    {/* VISUAL & THEME */}
                    <div className="plate-concave control-panel">
                        <div className="section-label">VISUAL & THEME</div>

                        <div className="control-row">
                            <span className="control-text">NIGHT VISION</span>
                            <button
                                className={`plate-convex action-button ${settings.nightMode ? 'pressed' : ''}`}
                                onClick={() => updateSetting('nightMode', !settings.nightMode)}
                            >
                                {settings.nightMode ? 'ACTIVE' : 'STANDBY'}
                            </button>
                        </div>

                        <div className={`glow-selector ${settings.nightMode ? 'active' : 'inactive'}`}>
                            <div className="section-label glow-label">GLOW SPECTRUM</div>
                            <div className="glow-grid">
                                {(['green', 'red', 'amber'] as const).map(color => (
                                    <button
                                        key={color}
                                        className={`plate-convex glow-option ${settings.glowColor === color ? 'pressed glow-btn-active' : ''}`}
                                        onClick={() => updateSetting('glowColor', color)}
                                        style={{ '--gc': `var(--glow-${color})` } as any}
                                    >
                                        {color.toUpperCase()}
                                    </button>
                                ))}
                            </div>
                        </div>

                        <div className="section-label enhancement-label">DISPLAY ENHANCEMENTS</div>
                        <div className="enhancement-row">
                            <button
                                className={`plate-convex action-button ${settings.burninGuard ? 'pressed' : ''}`}
                                onClick={() => updateSetting('burninGuard', !settings.burninGuard)}
                            >
                                GUARD: {settings.burninGuard ? 'ON' : 'OFF'}
                            </button>
                            <button
                                className={`plate-convex action-button ${settings.burninCrtMode ? 'pressed' : ''}`}
                                onClick={() => updateSetting('burninCrtMode', !settings.burninCrtMode)}
                                disabled={!settings.burninGuard}
                                style={{ opacity: settings.burninGuard ? 1 : 0.4 }}
                            >
                                CRT: {settings.burninCrtMode ? 'ON' : 'OFF'}
                            </button>
                        </div>
                    </div>

                    {/* FONT & TYPOGRAPHY */}
                    <div className="plate-concave control-panel">
                        <div className="section-label">FONT & TYPOGRAPHY</div>

                        <div className="slider-group">
                            <div className="slider-header">
                                <label className="section-label slider-label">WEIGHT</label>
                                <span className="slider-value">{settings.fontWeight}</span>
                            </div>
                            <input
                                type="range" min="100" max="900" step="100"
                                value={settings.fontWeight}
                                onChange={(e) => updateSetting('fontWeight', parseInt(e.target.value))}
                                className="dashboard-slider"
                            />
                        </div>

                        <div className="slider-group">
                            <div className="slider-header">
                                <label className="section-label slider-label">SECONDS SCALE</label>
                                <span className="slider-value">{settings.secondsScale}%</span>
                            </div>
                            <input
                                type="range" min="50" max="100" step="1"
                                value={settings.secondsScale}
                                onChange={(e) => updateSetting('secondsScale', parseInt(e.target.value))}
                                className="dashboard-slider"
                            />
                        </div>

                        <div className="slider-group">
                            <div className="slider-header">
                                <label className="section-label slider-label">MAIN FONT (MONO)</label>
                            </div>
                            <select
                                className="dashboard-input"
                                value={settings.fontMain}
                                onChange={(e) => updateSetting('fontMain', e.target.value)}
                                style={{ width: '100%', padding: '8px', fontSize: '11px', marginBottom: '8px' }}
                            >
                                <option value="var(--font-jetbrains-mono)">JETBRAINS MONO</option>
                                <option value="var(--font-geist-mono)">GEIST MONO</option>
                                <option value="var(--font-ibm-plex-mono)">IBM PLEX MONO</option>
                                <option value="TenoText">TENOTYPE (8x10)</option>
                                <option value="custom">-- CUSTOM / EXTERNAL --</option>
                            </select>
                            <input
                                type="text"
                                className="dashboard-input"
                                placeholder="ENTER CSS FONT-FAMILY..."
                                value={settings.fontMain === 'var(--font-jetbrains-mono)' || settings.fontMain === 'var(--font-geist-mono)' || settings.fontMain === 'var(--font-ibm-plex-mono)' || settings.fontMain === 'TenoText' ? '' : settings.fontMain}
                                onChange={(e) => updateSetting('fontMain', e.target.value)}
                                style={{ width: '100%', padding: '8px', fontSize: '10px' }}
                            />
                        </div>

                        <div className="slider-group">
                            <div className="slider-header">
                                <label className="section-label slider-label">UI FONT (SANS)</label>
                            </div>
                            <select
                                className="dashboard-input"
                                value={settings.fontUI}
                                onChange={(e) => updateSetting('fontUI', e.target.value)}
                                style={{ width: '100%', padding: '8px', fontSize: '11px', marginBottom: '8px' }}
                            >
                                <option value="var(--font-geist-sans)">GEIST SANS</option>
                                <option value="var(--font-inter)">INTER UI</option>
                                <option value="TenoText">TENOTYPE (8x10)</option>
                                <option value="system-ui">SYSTEM DEFAULT</option>
                                <option value="custom">-- CUSTOM / EXTERNAL --</option>
                            </select>
                            <input
                                type="text"
                                className="dashboard-input"
                                placeholder="ENTER CSS FONT-FAMILY..."
                                value={settings.fontUI === 'var(--font-geist-sans)' || settings.fontUI === 'var(--font-inter)' || settings.fontUI === 'TenoText' || settings.fontUI === 'system-ui' ? '' : settings.fontUI}
                                onChange={(e) => updateSetting('fontUI', e.target.value)}
                                style={{ width: '100%', padding: '8px', fontSize: '10px' }}
                            />
                        </div>

                        <div className="toggle-row">
                            <button
                                className={`plate-convex action-button ${settings.thinOverride ? 'pressed' : ''}`}
                                onClick={() => updateSetting('thinOverride', !settings.thinOverride)}
                            >
                                THIN: {settings.thinOverride ? 'ON' : 'OFF'}
                            </button>
                            <button
                                className={`plate-convex action-button ${settings.monoEnforce ? 'pressed' : ''}`}
                                onClick={() => updateSetting('monoEnforce', !settings.monoEnforce)}
                            >
                                MONO: {settings.monoEnforce ? 'ON' : 'OFF'}
                            </button>
                        </div>

                        <div className="reload-row">
                            <button
                                className="plate-convex reload-button"
                                onClick={() => updateSetting('reloadCounter', settings.reloadCounter + 1)}
                            >
                                RELOAD ALL CLIENTS
                            </button>
                        </div>
                    </div>

                </section>

                {/* Todo Management */}
                <section className="plate-concave todo-section">
                    <div className="section-label todo-header">TASKPersistence INTERFACE</div>

                    <form onSubmit={addTodo} className="todo-form">
                        <input
                            type="text"
                            value={newTodo}
                            onChange={(e) => setNewTodo(e.target.value)}
                            placeholder="INITIALIZE NEW TASK..."
                            className="dashboard-input todo-input"
                        />
                        <button type="submit" className="plate-convex todo-add-btn">
                            ADD
                        </button>
                    </form>

                    <div className="todo-list">
                        {todos.length === 0 ? (
                            <div className="todo-empty">-- NO ACTIVE TASKS --</div>
                        ) : (
                            todos.map(todo => (
                                <div key={todo.id} className="plate-convex todo-item">
                                    <input
                                        type="checkbox"
                                        checked={todo.completed}
                                        onChange={() => toggleTodo(todo.id)}
                                        className="todo-checkbox"
                                    />
                                    <span className={`todo-text ${todo.completed ? 'completed' : ''}`}>
                                        {todo.text}
                                    </span>
                                    <button
                                        onClick={() => deleteTodo(todo.id)}
                                        className="todo-delete-btn"
                                    >
                                        &times;
                                    </button>
                                </div>
                            ))
                        )}
                    </div>
                </section>

                <footer className="dashboard-footer">
                    DP-05 MGMT CONSOLE / TEENAGE ENGINEERING PHILOSOPHY ENABLED
                </footer>
            </div>

            <style jsx>{`
                .dashboard-root {
                    min-height: 100vh;
                    overflow-y: auto;
                    padding: 40px 24px;
                    -webkit-overflow-scrolling: touch;
                    background: var(--bg);
                }
                .dashboard-container {
                    max-width: 800px;
                    margin: 0 auto;
                    display: flex;
                    flex-direction: column;
                    gap: 32px;
                }
                .dashboard-root :global(#app) {
                    background: none !important;
                    height: auto !important;
                    overflow: visible !important;
                }
                .dashboard-header {
                    display: flex;
                    justify-content: space-between;
                    align-items: baseline;
                    border-bottom: 0.5px solid var(--grid-line);
                    padding-bottom: 12px;
                }
                .header-meta {
                    font-family: var(--font-main);
                    font-size: 10px;
                    opacity: 0.6;
                }
                .status-grid {
                    display: grid;
                    grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
                    gap: 12px;
                }
                .status-card {
                    padding: 16px;
                }
                .status-value {
                    font-family: var(--font-main);
                    font-size: 14px;
                    margin-top: 8px;
                }
                .status-sub {
                    font-size: 10px;
                    opacity: 0.6;
                    margin-top: 4px;
                }
                .controls-grid {
                    display: grid;
                    grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
                    gap: 20px;
                }
                .control-panel {
                    padding: 20px;
                    display: flex;
                    flex-direction: column;
                    gap: 20px;
                }
                .control-row {
                    display: flex;
                    align-items: center;
                    justify-content: space-between;
                }
                .control-text {
                    fontSize: 12px;
                    fontWeight: 500;
                }
                .action-button {
                    padding: 8px 16px;
                    font-size: 10px;
                    font-weight: 600;
                    border: none;
                    cursor: pointer;
                    minWidth: 80px;
                    transition: all 0.1s var(--ease-mechanical);
                }
                .glow-selector {
                    transition: opacity 0.2s var(--ease-mechanical);
                }
                .glow-selector.inactive {
                    opacity: 0.2;
                    pointer-events: none;
                    filter: grayscale(1);
                }
                .glow-label {
                    font-size: 8px;
                    margin-bottom: 8px;
                }
                .glow-grid {
                    display: flex;
                    gap: 8px;
                }
                .glow-option {
                    flex: 1;
                    padding: 10px;
                    font-size: 9px;
                    font-weight: 700;
                    border: none;
                    cursor: pointer;
                }
                .enhancement-label {
                    font-size: 8px;
                }
                .enhancement-row {
                    display: flex;
                    gap: 10px;
                }
                .slider-group {
                    padding: 4px 0;
                }
                .slider-header {
                    display: flex;
                    justify-content: space-between;
                    margin-bottom: 8px;
                }
                .slider-label {
                    font-size: 9px;
                }
                .slider-value {
                    font-size: 10px;
                    font-family: var(--font-main);
                }
                .toggle-row {
                    display: flex;
                    gap: 10px;
                    margin-top: 4px;
                }
                .reload-row {
                    margin-top: 4px;
                }
                .reload-button {
                    width: 100%;
                    padding: 12px;
                    font-size: 10px;
                    font-weight: 600;
                    border: none;
                    cursor: pointer;
                    color: #ff4444;
                }
                .todo-section {
                    padding: 24px;
                }
                .todo-header {
                    margin-bottom: 20px;
                }
                .todo-form {
                    display: flex;
                    gap: 8px;
                    margin-bottom: 24px;
                }
                .todo-input {
                    flex: 1;
                    padding: 14px;
                    border-radius: 6px;
                    font-size: 12px;
                }
                .todo-add-btn {
                    padding: 0 24px;
                    font-weight: 700;
                    border: none;
                    cursor: pointer;
                }
                .todo-list {
                    display: flex;
                    flex-direction: column;
                    gap: 10px;
                    max-height: 400px;
                    overflow-y: auto;
                    padding-right: 8px;
                }
                .todo-empty {
                    text-align: center;
                    font-size: 10px;
                    opacity: 0.4;
                    padding: 20px;
                }
                .todo-item {
                    display: flex;
                    align-items: center;
                    gap: 12px;
                    padding: 12px 16px;
                }
                .todo-checkbox {
                    width: 18px;
                    height: 18px;
                    cursor: pointer;
                    accent-color: var(--text);
                }
                .todo-text {
                    flex: 1;
                    font-size: 13px;
                    font-family: var(--font-ui);
                }
                .todo-text.completed {
                    text-decoration: line-through;
                    opacity: 0.4;
                }
                .todo-delete-btn {
                    background: none;
                    border: none;
                    color: #ff4444;
                    cursor: pointer;
                    font-size: 20px;
                    padding: 0 8px;
                }
                .dashboard-footer {
                    text-align: center;
                    padding: 20px;
                    font-size: 9px;
                    opacity: 0.4;
                    letter-spacing: 0.1em;
                }
            `}</style>
        </div>
    );
}
