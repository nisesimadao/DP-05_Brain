'use client';

import React, { useEffect, useRef, useState } from 'react';
import { useAppContext } from '../context/AppContext';

// Helper function from main.js
function pad(n: number): string {
    return n.toString().padStart(2, '0');
}

// Function to trigger glow pulse
function triggerGlowPulse(clockDisplayEl: HTMLElement | null) {
    if (!clockDisplayEl) return;
    clockDisplayEl.classList.add('glow-pulse');
    setTimeout(() => clockDisplayEl.classList.remove('glow-pulse'), 60);
}

export default function Clock() {
    const { state, updateGlobalState } = useAppContext();
    const [currentTime, setCurrentTime] = useState(new Date());
    const [mounted, setMounted] = useState(false);
    const clockDisplayRef = useRef<HTMLDivElement>(null);

    useEffect(() => {
        setMounted(true);
    }, []);

    // Clock update and colon blinking logic
    useEffect(() => {
        let animationFrameId: number;

        const update = () => {
            const now = new Date();
            setCurrentTime(now);

            // Colon blink logic (using simple timestamp)
            const timestamp = performance.now();
            if (timestamp - state.lastColonToggle >= 500) {
                updateGlobalState({
                    colonPhase: (state.colonPhase + 1) % 2,
                    lastColonToggle: timestamp,
                });
            }

            // Night mode glow pulse on second change
            const currentSecond = now.getSeconds();
            if (currentSecond !== state.lastSecond) {
                updateGlobalState({ lastSecond: currentSecond });
                if (state.nightMode && currentSecond !== 0) {
                    triggerGlowPulse(clockDisplayRef.current);
                }
            }

            animationFrameId = requestAnimationFrame(update);
        };

        animationFrameId = requestAnimationFrame(update);

        return () => {
            cancelAnimationFrame(animationFrameId);
        };
    }, [state.colonPhase, state.lastColonToggle, state.lastSecond, state.nightMode, updateGlobalState]);

    const renderDigits = (value: string) => {
        if (state.monoEnforce) {
            return value.split('').map((char, i) => (
                <span key={i} className="mono-cell">{char}</span>
            ));
        }
        return value;
    };

    const h = mounted ? pad(currentTime.getHours()) : '00';
    const m = mounted ? pad(currentTime.getMinutes()) : '00';
    const s = mounted ? pad(currentTime.getSeconds()) : '00';

    const colonDim = mounted ? state.colonPhase === 1 : false;

    return (
        <div id="clock-area">
            <div id="clock-display" ref={clockDisplayRef}>
                <span id="clock-hours" className="clock-digit">{renderDigits(h)}</span>
                <span id="clock-colon-1" className={`clock-colon ${colonDim ? 'dim' : ''}`}>:</span>
                <span id="clock-minutes" className="clock-digit">{renderDigits(m)}</span>
                <span id="clock-colon-2" className={`clock-colon ${colonDim ? 'dim' : ''}`}>:</span>
                <span id="clock-seconds" className="clock-digit clock-seconds">{renderDigits(s)}</span>
            </div>
        </div>
    );
}
