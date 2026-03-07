'use client';

import React, { useEffect, useState, useRef, useCallback } from 'react';
import { useAppContext } from '../context/AppContext';
import { useSystemData } from '../hooks/useSystemData';
import Clock from './Clock';
import Calendar from './Calendar';
import TimeModule from './TimeModule';
import TodoModule from './TodoModule';
import NetModule from './NetModule';
import DevicesModule from './DevicesModule';
import Settings from './Settings';

export default function AppContent() {
    const { state, updateGlobalState } = useAppContext();
    const lowerHalfRef = useRef<HTMLDivElement>(null);
    const checkerboardRef = useRef<HTMLDivElement>(null);

    const { systemData } = useSystemData(); // This call is now within AppContext.Provider scope

    // Long press logic from main.js
    useEffect(() => {
        const target = lowerHalfRef.current;
        if (!target) return;

        let longPressTimeout: NodeJS.Timeout | null = null;
        let isPressing = false;

        const startPress = (e: Event) => {
            // Don't trigger long-press on interactive settings elements
            if ((e.target as HTMLElement).closest('input, button, label.file-label')) return;

            isPressing = true;

            // Physical feedback (simplified for now)
            if (state.nightMode) {
                target.classList.add('pressed-night');
                setTimeout(() => {
                    target.classList.remove('pressed-night');
                    target.classList.add('pressed-night-recover');
                    setTimeout(() => target.classList.remove('pressed-night-recover'), 80);
                }, 30);
            } else {
                target.classList.add('press-feedback');
                // Noise overlay logic will be applied to a specific element if needed
            }

            longPressTimeout = setTimeout(() => {
                if (isPressing) {
                    updateGlobalState({ settingsOpen: !state.settingsOpen });
                }
            }, 600);
        };

        const endPress = () => {
            isPressing = false;
            if (longPressTimeout) {
                clearTimeout(longPressTimeout);
            }
            target.classList.remove('press-feedback', 'pressed-night', 'pressed-night-recover');
            // Reset noise overlay if applicable
        };

        target.addEventListener('touchstart', startPress, { passive: true });
        target.addEventListener('touchend', endPress);
        target.addEventListener('touchcancel', endPress);
        target.addEventListener('mousedown', startPress);
        target.addEventListener('mouseup', endPress);
        target.addEventListener('mouseleave', endPress);

        return () => {
            target.removeEventListener('touchstart', startPress);
            target.removeEventListener('touchend', endPress);
            target.removeEventListener('touchcancel', endPress);
            target.removeEventListener('mousedown', startPress);
            target.removeEventListener('mouseup', endPress);
            target.removeEventListener('mouseleave', endPress);
            if (longPressTimeout) {
                clearTimeout(longPressTimeout);
            }
        };
    }, [state.nightMode, state.settingsOpen, updateGlobalState]);

    // Burn-in protection (drift) logic
    useEffect(() => {
        let animationFrameId: number;
        let lastDriftUpdate = 0;
        let driftDirY = state.driftDirY; // Use a local variable to update it

        const updateDrift = (timestamp: DOMHighResTimeStamp) => {
            if (!state.burninGuard) {
                if (state.driftX !== 0 || state.driftY !== 0) {
                    updateGlobalState({ driftX: 0, driftY: 0 }); // Update global state
                    if (lowerHalfRef.current) lowerHalfRef.current.style.transform = '';
                }
                animationFrameId = requestAnimationFrame(updateDrift);
                return;
            }

            if (!lastDriftUpdate) lastDriftUpdate = timestamp;
            const dt = (timestamp - lastDriftUpdate) / 1000;
            lastDriftUpdate = timestamp;

            let { driftTimer, unchangedTime, driftX, driftY, driftDirX, driftCycle } = state; // Destructure from global state

            driftTimer += dt;
            unchangedTime += dt;

            const interval = unchangedTime > 21600 ? 30 : 60; // 6 hours vs 1 minute

            if (driftTimer >= interval) {
                driftTimer = 0;
                driftCycle++;

                if (driftCycle % 2 === 1) { // Move X every other cycle
                    driftX += driftDirX;
                    if (Math.abs(driftX) >= 3) driftDirX *= -1;
                } else { // Move Y on the other cycles
                    driftY += driftDirY;
                    if (Math.abs(driftY) >= 3) driftDirY *= -1;
                }
                updateGlobalState({ driftTimer, unchangedTime, driftX, driftY, driftDirX, driftDirY: driftDirY, driftCycle }); // Update global state
            }

            if (lowerHalfRef.current) {
                lowerHalfRef.current.style.transform = `translate(${state.driftX}px, ${state.driftY}px)`;
            }
            if (checkerboardRef.current) {
                checkerboardRef.current.style.transform = `translate(${state.driftX}px, ${state.driftY}px)`;
            }

            animationFrameId = requestAnimationFrame(updateDrift);
        };

        animationFrameId = requestAnimationFrame(updateDrift);

        return () => {
            cancelAnimationFrame(animationFrameId);
            if (lowerHalfRef.current) lowerHalfRef.current.style.transform = '';
        };
    }, [state.burninGuard, state.driftX, state.driftY, state.driftDirX, state.driftDirY, state.driftTimer, state.unchangedTime, state.driftCycle, updateGlobalState]);


    return (
        <>
            <div
                id="app"
                style={{
                    '--font-main': state.fontMain,
                    '--font-ui': state.fontUI,
                    '--font-weight': state.fontWeight,
                    '--mono-w': `${state.monoWidthRatio}em`
                } as any}
            >
                {/* Burn-in Guard Checkerboard Overlay */}
                {state.burninGuard && (
                    <div
                        id="checkerboard-overlay"
                        ref={checkerboardRef}
                        className={state.burninCrtMode ? 'crt-mode' : ''}
                    ></div>
                )}

                {/* Noise overlay */}
                <div id="noise-overlay"></div>

                {/* Burn-in drift layer */}
                <div id="drift-layer">

                    {/* ===== UPPER HALF: Clock + Calendar in ONE panel ===== */}
                    <section id="upper-half">
                        <div id="main-panel" className="plate-concave">
                            {/* Clock */}
                            <Clock />
                            {/* Calendar */}
                            <Calendar />
                        </div>
                    </section>

                    {/* ===== LOWER HALF: 4 modules side by side ===== */}
                    <section id="lower-half" ref={lowerHalfRef}>

                        {/* Module 1: TIME (Day + Remain) */}
                        <TimeModule />

                        {/* Module 2: TODO */}
                        <TodoModule />

                        {/* Module 3: NET */}
                        <NetModule systemData={systemData} /> {/* Pass systemData to NetModule */}

                        {/* Module 4: DEVICES */}
                        <DevicesModule systemData={systemData} /> {/* Pass systemData to DevicesModule */}

                        <Settings />
                    </section>

                    {/* Logo */}
                    <div id="logo">DP-05</div>

                </div>{/* /drift-layer */}
            </div>
        </>
    );
}
