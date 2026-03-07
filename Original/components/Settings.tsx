'use client';

import React, { useCallback, useEffect, useRef } from 'react';
import { useAppContext } from '../context/AppContext';

export default function Settings() {
    const { state, updateGlobalState } = useAppContext();
    const settingsPanelRef = useRef<HTMLDivElement>(null);

    // Font system functions (from main.js)
    const loadCustomFont = useCallback((file: File) => {
        const reader = new FileReader();
        reader.onload = (e) => {
            const fontName = 'DP05-Custom';
            const result = e.target && e.target.result;
            if (result) {
                const fontFace = new FontFace(fontName, `url(${result})`);

                fontFace.load().then((loaded) => {
                    document.fonts.add(loaded);
                    updateGlobalState({ customFontFamily: fontName });
                    measureGlyphs();
                }).catch((err) => {
                    console.error('Font load error:', err);
                });
            }
        };
        reader.readAsDataURL(file);
    }, [updateGlobalState]);

    const removeCustomFont = useCallback(() => {
        updateGlobalState({ customFontFamily: null });
        measureGlyphs();
    }, [updateGlobalState]);

    const applyFontSettings = useCallback(() => {
        const fontFamily = state.customFontFamily
            ? `'${state.customFontFamily}', 'JetBrains Mono', monospace`
            : "'JetBrains Mono', 'Inter', monospace";

        document.documentElement.style.setProperty('--font-main', fontFamily);
        document.documentElement.style.setProperty('--font-weight', state.fontWeight.toString());
        document.documentElement.style.setProperty('--seconds-scale', (state.secondsScale / 100).toString());

        if (state.thinOverride) {
            document.documentElement.style.setProperty('--font-weight',
                Math.max(100, state.fontWeight - 200).toString());
        } else {
            document.documentElement.style.setProperty('--font-weight', state.fontWeight.toString());
        }
    }, [state.customFontFamily, state.fontWeight, state.secondsScale, state.thinOverride]);

    const measureGlyphs = useCallback(() => {
        if (!state.monoEnforce) {
            updateGlobalState({ monoWidthRatio: 0 });
            document.documentElement.style.removeProperty('--mono-w');
            return;
        }

        const canvas = document.createElement('canvas');
        const ctx = canvas.getContext('2d');
        if (!ctx) return;
        const fontSize = 100;
        const fontFamily = state.customFontFamily
            ? `'${state.customFontFamily}'`
            : "'JetBrains Mono'";
        ctx.font = `${state.fontWeight} ${fontSize}px ${fontFamily}`;

        let maxW = 0;
        for (let d = 0; d <= 9; d++) {
            const m = ctx.measureText(d.toString());
            maxW = Math.max(maxW, m.width);
        }

        const ratio = (maxW / fontSize) * 1.1;
        const monoWidthRatio = Math.min(Math.max(ratio, 0.5), 1.2);
        updateGlobalState({ monoWidthRatio });

        document.documentElement.style.setProperty('--mono-w', `${monoWidthRatio}em`);

        try {
            const m = ctx.measureText('0');
            if (m && typeof m.actualBoundingBoxAscent === 'number') {
                const ascent = m.actualBoundingBoxAscent;
                const descent = m.actualBoundingBoxDescent || 0;
                const offsetRatio = (ascent - descent) / (-2 * fontSize);
                document.documentElement.style.setProperty('--digit-offset', `${offsetRatio.toFixed(3)}em`);

                const mc = ctx.measureText(':');
                const ascC = mc.actualBoundingBoxAscent;
                const desC = mc.actualBoundingBoxDescent || 0;
                const offCRatio = (ascC - desC) / (-2 * fontSize);
                document.documentElement.style.setProperty('--colon-offset', `${offCRatio.toFixed(3)}em`);
            } else {
                document.documentElement.style.removeProperty('--digit-offset');
                document.documentElement.style.removeProperty('--colon-offset');
            }
        } catch (e) {
            document.documentElement.style.removeProperty('--digit-offset');
        }
    }, [state.customFontFamily, state.fontWeight, state.monoEnforce, updateGlobalState]);

    useEffect(() => {
        applyFontSettings();
        measureGlyphs();
    }, [applyFontSettings, measureGlyphs]);

    // Handle full screen state
    useEffect(() => {
        const handleFullscreenChange = () => {
            updateGlobalState({ isFullscreen: !!document.fullscreenElement });
        };
        document.addEventListener('fullscreenchange', handleFullscreenChange);
        return () => document.removeEventListener('fullscreenchange', handleFullscreenChange);
    }, [updateGlobalState]);

    const handleFontUpload = useCallback((e: React.ChangeEvent<HTMLInputElement>) => {
        if (e.target.files && e.target.files[0]) {
            loadCustomFont(e.target.files[0]);
        }
    }, [loadCustomFont]);

    const handleWeightChange = useCallback((e: React.ChangeEvent<HTMLInputElement>) => {
        updateGlobalState({ fontWeight: parseInt(e.target.value) });
    }, [updateGlobalState]);

    const handleSecondsScaleChange = useCallback((e: React.ChangeEvent<HTMLInputElement>) => {
        updateGlobalState({ secondsScale: parseInt(e.target.value) });
    }, [updateGlobalState]);

    const handleThinOverrideToggle = useCallback(() => {
        updateGlobalState({ thinOverride: !state.thinOverride });
    }, [state.thinOverride, updateGlobalState]);

    const handleMonoEnforceToggle = useCallback(() => {
        updateGlobalState({ monoEnforce: !state.monoEnforce });
    }, [state.monoEnforce, updateGlobalState]);

    const handleNightModeToggle = useCallback(() => {
        updateGlobalState({ nightMode: !state.nightMode });
    }, [state.nightMode, updateGlobalState]);

    const handleGlowColorChange = useCallback((color: 'green' | 'red' | 'amber') => {
        updateGlobalState({ glowColor: color });
    }, [updateGlobalState]);

    const handleBurninToggle = useCallback(() => {
        updateGlobalState({ burninGuard: !state.burninGuard });
    }, [state.burninGuard, updateGlobalState]);

    const handleBurninCrtToggle = useCallback(() => {
        if (!state.burninGuard) return; // Prevent toggle if Burn-In Guard is disabled
        updateGlobalState({ burninCrtMode: !state.burninCrtMode });
    }, [state.burninGuard, state.burninCrtMode, updateGlobalState]);

    const handleFullscreenToggle = useCallback(() => {
        if (!document.fullscreenElement) {
            document.documentElement.requestFullscreen().catch((err) => {
                console.warn(`Fullscreen error: ${err.message}`);
            });
        } else {
            if (document.exitFullscreen) {
                document.exitFullscreen();
            }
        }
    }, []);

    return (
        <div className={`module-settings-overlay ${state.settingsOpen ? 'visible' : ''}`}>
            <div className="module-settings">
                {/* Module 1: DAY Settings */}
                <div className="setting-section">
                    <div className="section-label">DAY SETTINGS</div>
                    <div className="setting-item">
                        <label className="setting-label">FONT SOURCE</label>
                        <div className="setting-control">
                            <input type="file" id="font-upload" accept=".ttf,.otf,.woff,.woff2" onChange={handleFontUpload} />
                            <label htmlFor="font-upload" className="file-label plate-convex">SELECT</label>
                        </div>
                    </div>
                    <div className="setting-item">
                        <label className="setting-label">WEIGHT</label>
                        <div className="setting-control">
                            <input type="range" id="font-weight" min="100" max="900" step="100" value={state.fontWeight} onChange={handleWeightChange} />
                            <span id="font-weight-value" className="setting-value">{state.fontWeight}</span>
                        </div>
                    </div>
                    <div className="setting-item">
                        <label className="setting-label">REMOVE FONT</label>
                        <div className="setting-control">
                            <button id="remove-font" className="action-btn plate-convex" onClick={removeCustomFont}>RESET</button>
                        </div>
                    </div>
                </div>

                {/* Module 2: REMAIN Settings */}
                <div className="setting-section">
                    <div className="section-label">REMAIN SETTINGS</div>
                    <div className="setting-item">
                        <label className="setting-label">SECONDS SCALE</label>
                        <div className="setting-control">
                            <input type="range" id="seconds-scale" min="50" max="100" step="1" value={state.secondsScale} onChange={handleSecondsScaleChange} />
                            <span id="seconds-scale-value" className="setting-value">{state.secondsScale}%</span>
                        </div>
                    </div>
                    <div className="setting-item">
                        <label className="setting-label">THIN OVERRIDE</label>
                        <div className="setting-control">
                            <button id="thin-override" className="toggle-btn plate-convex" data-state={state.thinOverride ? 'on' : 'off'} onClick={handleThinOverrideToggle}>
                                {state.thinOverride ? 'ON' : 'OFF'}
                            </button>
                        </div>
                    </div>
                    <div className="setting-item">
                        <label className="setting-label">MONOSPACE</label>
                        <div className="setting-control">
                            <button id="mono-enforce" className="toggle-btn plate-convex" data-state={state.monoEnforce ? 'on' : 'off'} onClick={handleMonoEnforceToggle}>
                                {state.monoEnforce ? 'ON' : 'OFF'}
                            </button>
                        </div>
                    </div>
                </div>

                {/* Module 3: NET Settings */}
                <div className="setting-section">
                    <div className="section-label">NET SETTINGS</div>
                    <div className="setting-item">
                        <label className="setting-label">NIGHT MODE</label>
                        <div className="setting-control">
                            <button id="night-toggle" className="toggle-btn plate-convex" data-state={state.nightMode ? 'on' : 'off'} onClick={handleNightModeToggle}>
                                {state.nightMode ? 'ON' : 'OFF'}
                            </button>
                        </div>
                    </div>
                    <div className="setting-item">
                        <label className="setting-label">GLOW COLOR</label>
                        <div className="setting-control glow-colors">
                            <button className={`glow-btn ${state.glowColor === 'green' ? 'active' : ''}`} data-color="green" style={{ '--gc': '#2EDB6A' } as React.CSSProperties} onClick={() => handleGlowColorChange('green')}>●</button>
                            <button className={`glow-btn ${state.glowColor === 'red' ? 'active' : ''}`} data-color="red" style={{ '--gc': '#B01212' } as React.CSSProperties} onClick={() => handleGlowColorChange('red')}>●</button>
                            <button className={`glow-btn ${state.glowColor === 'amber' ? 'active' : ''}`} data-color="amber" style={{ '--gc': '#FFB347' } as React.CSSProperties} onClick={() => handleGlowColorChange('amber')}>●</button>
                        </div>
                    </div>
                </div>

                {/* Module 4: DEVICES Settings */}
                <div className="setting-section">
                    <div className="section-label">DEVICES SETTINGS</div>
                    <div className="setting-item">
                        <label className="setting-label">BURN-IN GUARD</label>
                        <div className="setting-control">
                            <button id="burnin-toggle" className="toggle-btn plate-convex" data-state={state.burninGuard ? 'on' : 'off'} onClick={handleBurninToggle}>
                                {state.burninGuard ? 'ON' : 'OFF'}
                            </button>
                        </div>
                    </div>
                    <div className={`setting-item ${!state.burninGuard ? 'disabled' : ''}`}>
                        <label className="setting-label">CRT OVERLAY</label>
                        <div className="setting-control">
                            <button id="burnin-crt-toggle" className="toggle-btn plate-convex" data-state={state.burninCrtMode ? 'on' : 'off'} onClick={handleBurninCrtToggle} disabled={!state.burninGuard}>
                                {state.burninCrtMode ? 'ON' : 'OFF'}
                            </button>
                        </div>
                    </div>
                    <div className="setting-item">
                        <label className="setting-label">FULLSCREEN</label>
                        <div className="setting-control">
                            <button id="fullscreen-toggle" className="toggle-btn plate-convex" data-state={state.isFullscreen ? 'on' : 'off'} onClick={handleFullscreenToggle}>
                                {state.isFullscreen ? 'ON' : 'OFF'}
                            </button>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
}