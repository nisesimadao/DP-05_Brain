'use client';

import React, { useEffect, useState, useCallback } from 'react';
import { AppContext } from '../context/AppContext';
import AppContent from './AppContent'; // Import the new AppContent component
import { AppState } from '../context/AppContext'; // Import AppState type

// Initial state based on main.js
const initialState: AppState = {
    customFontFamily: null,
    fontWeight: 400,
    secondsScale: 82,
    thinOverride: false,
    monoEnforce: true,
    fontMain: 'var(--font-jetbrains-mono)',
    fontUI: 'var(--font-geist-sans)',
    nightMode: false,
    glowColor: 'green',
    burninGuard: false,
    burninCrtMode: false,
    reloadCounter: 0,
    isFullscreen: false,
    settingsOpen: false,
    colonPhase: 0,
    lastColonToggle: 0,
    lastSecond: -1,
    lastRemainSecond: -1,
    monoWidthRatio: 0.6,
    driftX: 0,
    driftY: 0,
    driftDirX: 1,
    driftDirY: 1,
    driftTimer: 0,
    driftCycle: 0,
    lastDriftTime: 0,
    unchangedTime: 0,
    dlSpeed: 0,
    ulSpeed: 0,
    targetDl: 85,
    targetUl: 22,
    dlHistory: [],
    ulHistory: [],
    measuredDl: 0,
    measuredUl: 0,
    measureAvailable: false,
    longPressTimer: null,
    isPressing: false,
    publicIp: '--',
    localIp: '--',
    todos: [],
};

export default function Layout() {
    const [state, setState] = useState(initialState);

    // Effect for handling global theme and glow color changes
    useEffect(() => {
        document.documentElement.dataset.theme = state.nightMode ? 'night' : 'light';
        document.documentElement.dataset.glowColor = state.glowColor;
    }, [state.nightMode, state.glowColor]);

    // Function to update global state and persist to server
    const updateGlobalState = useCallback(async (newValues: Partial<AppState>) => {
        setState(prevState => ({ ...prevState, ...newValues }));

        // Persist relevant settings to server
        const settingsKeys = ['nightMode', 'glowColor', 'burninGuard', 'burninCrtMode', 'fontWeight', 'secondsScale', 'thinOverride', 'monoEnforce', 'fontMain', 'fontUI'];
        const settingsToPersist = Object.keys(newValues)
            .filter(key => settingsKeys.includes(key))
            .reduce((obj, key) => {
                (obj as any)[key] = (newValues as any)[key];
                return obj;
            }, {});

        if (Object.keys(settingsToPersist).length > 0) {
            try {
                await fetch('/api/settings', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(settingsToPersist)
                });
            } catch (e) {
                console.error('Failed to persist settings', e);
            }
        }
    }, []);

    // SSE Connection for real-time sync
    useEffect(() => {
        let eventSource: EventSource | null = null;

        const connect = () => {
            if (eventSource) eventSource.close();

            eventSource = new EventSource('/api/sync');

            eventSource.onopen = () => {
                console.log('[SSE] Stream established');
            };

            eventSource.onmessage = (event) => {
                try {
                    const { type, data } = JSON.parse(event.data);
                    console.log(`[SSE] Received event: ${type}`, data);
                    if (type === 'sync') {
                        setState(prevState => {
                            const hasChanges =
                                data.nightMode !== prevState.nightMode ||
                                data.glowColor !== prevState.glowColor ||
                                data.burninGuard !== prevState.burninGuard ||
                                data.burninCrtMode !== prevState.burninCrtMode ||
                                data.fontWeight !== prevState.fontWeight ||
                                data.secondsScale !== prevState.secondsScale ||
                                data.thinOverride !== prevState.thinOverride ||
                                data.monoEnforce !== prevState.monoEnforce ||
                                data.fontMain !== prevState.fontMain ||
                                data.fontUI !== prevState.fontUI;

                            if (data.reloadCounter > prevState.reloadCounter) {
                                window.location.reload();
                            }

                            if (hasChanges) {
                                return { ...prevState, ...data };
                            }
                            return prevState;
                        });
                    }
                } catch (e) {
                    console.error('Failed to parse SSE message', e);
                }
            };

            eventSource.onerror = () => {
                console.warn('SSE disconnected. Retrying in 3s...');
                eventSource?.close();
                setTimeout(connect, 3000);
            };
        };

        connect();

        // Fallback polling (every 5s) in case SSE is unstable
        const fallbackInterval = setInterval(async () => {
            try {
                const res = await fetch(`/api/settings?t=${Date.now()}`);
                if (res.ok) {
                    const data = await res.json();
                    setState(prev => ({ ...prev, ...data }));
                }
            } catch (e) { }
        }, 5000);

        return () => {
            eventSource?.close();
            clearInterval(fallbackInterval);
        };
    }, []);

    // Initial fetch only
    useEffect(() => {
        const fetchInitialSettings = async () => {
            try {
                const res = await fetch(`/api/settings?t=${Date.now()}`);
                if (res.ok) {
                    const remoteSettings = await res.json();
                    setState(prevState => ({ ...prevState, ...remoteSettings }));
                }
            } catch (e) {
                console.error('Failed to fetch initial settings', e);
            }
        };
        fetchInitialSettings();
    }, []);

    return (
        <AppContext.Provider value={{ state, updateGlobalState }}>
            <AppContent />
        </AppContext.Provider>
    );
}
