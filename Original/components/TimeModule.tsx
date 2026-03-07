'use client';

import React, { useEffect, useState, useCallback } from 'react';
import { useAppContext } from '../context/AppContext';

function pad(n: number): string {
    return n.toString().padStart(2, '0');
}

export default function TimeModule() {
    const { state } = useAppContext();
    const [remainHours, setRemainHours] = useState('00');
    const [remainMinutes, setRemainMinutes] = useState('00');
    const [remainSeconds, setRemainSeconds] = useState('00');
    const [dayPercentage, setDayPercentage] = useState(0);

    const updateTime = useCallback(() => {
        const now = new Date();
        const totalSeconds = 24 * 60 * 60;
        const elapsed = now.getHours() * 3600 + now.getMinutes() * 60 + now.getSeconds();

        // Remain logic
        const remain = totalSeconds - elapsed;
        const h = Math.floor(remain / 3600);
        const m = Math.floor((remain % 3600) / 60);
        const s = remain % 60;

        setRemainHours(pad(h));
        setRemainMinutes(pad(m));
        setRemainSeconds(pad(s));

        // Day logic
        const pct = (elapsed / totalSeconds) * 100;
        setDayPercentage(pct);
    }, []);

    useEffect(() => {
        updateTime();
        const interval = setInterval(updateTime, 1000);
        return () => clearInterval(interval);
    }, [updateTime]);

    const renderDigits = (value: string) => {
        if (state.monoEnforce && state.monoWidthRatio > 0) {
            return value.split('').map((char, i) => (
                <span key={i} className="mono-cell">{char}</span>
            ));
        }
        return value;
    };

    return (
        <div className="module" id="mod-time">
            <div className="module-inner" id="time-content">
                <div className="section-label">TIME REMAINING</div>
                <div id="remain-display" style={{ paddingTop: '5px', marginBottom: '10px' }}>
                    <span className="remain-digit">{renderDigits(remainHours)}</span>
                    <span className="remain-colon">:</span>
                    <span className="remain-digit">{renderDigits(remainMinutes)}</span>
                    <span className="remain-colon">:</span>
                    <span className="remain-digit remain-seconds">{renderDigits(remainSeconds)}</span>
                </div>

                <div id="day-bar-container" className="plate-concave">
                    <div id="day-bar-fill" className="plate-convex" style={{ width: `${dayPercentage}%` }}></div>
                </div>
                <div id="day-labels">
                    <span>00:00</span>
                    <span className="section-label">PROGRESS</span>
                    <span>24:00</span>
                </div>
            </div>
        </div>
    );
}
