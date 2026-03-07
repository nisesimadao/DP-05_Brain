'use client';

import React, { useEffect, useRef, useCallback } from 'react';
import { useAppContext } from '../context/AppContext';
import { useSystemData } from '../hooks/useSystemData';

interface NetModuleProps {
    systemData: any; // SystemData type from useSystemData hook
}

// Helper functions (moved from main.js)
function getGlowColorRgba(glowColor: string, alpha: number) {
    const colors: { [key: string]: string } = {
        green: `rgba(46, 219, 106, ${alpha})`,
        red: `rgba(176, 18, 18, ${alpha})`,
        amber: `rgba(255, 179, 71, ${alpha})`,
    };
    return colors[glowColor] || colors.green;
}

function drawLine(ctx: CanvasRenderingContext2D, data: number[], w: number, h: number, minVal: number, range: number, color: string, lineWidth: number) {
    if (data.length < 2) return;
    const MAX_HISTORY = 60;
    const step = w / (MAX_HISTORY - 1);

    ctx.beginPath();
    ctx.strokeStyle = color;
    ctx.lineWidth = lineWidth;
    ctx.lineJoin = 'round';
    ctx.lineCap = 'round';

    for (let i = 0; i < data.length; i++) {
        const x = (MAX_HISTORY - data.length + i) * step;
        const y = h - ((data[i] - minVal) / range) * (h - 4) - 2;
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
    }
    ctx.stroke();
}


export default function NetModule({ systemData }: NetModuleProps) {
    const { state } = useAppContext();
    const { dlHistory, ulHistory } = useSystemData(); // Get histories from the hook
    const netGraphRef = useRef<HTMLCanvasElement>(null);

    const drawSparkline = useCallback(() => {
        const canvas = netGraphRef.current;
        if (!canvas) return;

        const dpr = window.devicePixelRatio || 1;
        const rect = canvas.getBoundingClientRect();
        if (rect.width === 0 || rect.height === 0) return;

        canvas.width = rect.width * dpr;
        canvas.height = rect.height * dpr;
        const ctx = canvas.getContext('2d');
        if (!ctx) return;
        ctx.scale(dpr, dpr);

        const w = rect.width;
        const h = rect.height;

        ctx.clearRect(0, 0, w, h);

        const allValues = [...dlHistory, ...ulHistory];
        if (allValues.length < 2) return;
        const maxVal = Math.max(...allValues, 1);
        const minVal = Math.min(...allValues, 0);
        const range = maxVal - minVal || 1;

        const isNight = state.nightMode;

        drawLine(ctx, dlHistory, w, h, minVal, range,
            isNight ? getGlowColorRgba(state.glowColor, 0.7) : 'rgba(17,17,17,0.5)', 1.5);

        drawLine(ctx, ulHistory, w, h, minVal, range,
            isNight ? getGlowColorRgba(state.glowColor, 0.3) : 'rgba(17,17,17,0.18)', 1);
    }, [dlHistory, ulHistory, state.nightMode, state.glowColor]);

    useEffect(() => {
        drawSparkline();
    }, [drawSparkline, dlHistory, ulHistory]); // Redraw when histories change

    const dl = state.measuredDl || state.dlSpeed;
    const ul = state.measuredUl || state.ulSpeed;
    const total = dl + ul;

    return (
        <div className="module" id="mod-net">
            <div className="module-inner" id="net-content">
                <div className="section-label">NET</div>
                <div id="net-stats">
                    <div className="net-row">
                        <span className="net-direction">DL</span>
                        <span id="net-dl-value" className="net-value">{dl.toFixed(1)}</span>
                        <span className="net-unit">Mbps</span>
                    </div>
                    <div className="net-row">
                        <span className="net-direction">UL</span>
                        <span id="net-ul-value" className="net-value">{ul.toFixed(1)}</span>
                        <span className="net-unit">Mbps</span>
                    </div>
                    <div className="net-row net-total">
                        <span className="net-direction">ALL</span>
                        <span id="net-total-value" className="net-value">{total.toFixed(1)}</span>
                        <span className="net-unit">Mbps</span>
                    </div>
                    <div className="net-row">
                        <span className="net-direction">PUB</span>
                        <span id="net-public-ip" className="net-value">{state.publicIp}</span>
                        <span className="net-unit">IP</span>
                    </div>
                    <div className="net-row">
                        <span className="net-direction">MGMT</span>
                        <span id="net-local-ip" className="net-value">
                            <a href="/dashboard" style={{ color: 'inherit', textDecoration: 'none' }}>{state.localIp}</a>
                        </span>
                        <span className="net-unit">URL</span>
                    </div>
                </div>
                <canvas id="net-graph" ref={netGraphRef}></canvas>
            </div>
            {/* Settings replacement - will be handled by the main Settings component */}
            <div className="module-settings hidden" id="settings-net">
                {/* ... settings content ... */}
            </div>
        </div>
    );
}
