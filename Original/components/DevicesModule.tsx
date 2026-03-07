'use client';

import React, { useEffect, useRef, useState } from 'react'; // Import useState
import { useAppContext } from '../context/AppContext';
import { useSystemData } from '../hooks/useSystemData';

interface DevicesModuleProps {
    systemData: any; // SystemData type from useSystemData hook
}

export default function DevicesModule({ systemData }: DevicesModuleProps) {
    const { state } = useAppContext();
    const { prevDevices } = useSystemData(); // Get prevDevices for animation
    const devicesListRef = useRef<HTMLDivElement>(null);
    const [clientHostname, setClientHostname] = useState<string>('LOCAL'); // New state for client-side hostname

    // Effect to get hostname on client side
    useEffect(() => {
        if (typeof window !== 'undefined' && window.location && window.location.hostname) {
            setClientHostname(window.location.hostname);
        }
    }, []);

    // Effect to handle flash animation
    useEffect(() => {
        if (devicesListRef.current) {
            const flashes = devicesListRef.current.querySelectorAll('.device-item.flash');
            flashes.forEach(f => {
                f.addEventListener('animationend', () => f.classList.remove('flash'), { once: true });
            });
        }
    }, [systemData && systemData.watchedDevices, prevDevices]); // Re-run when watched devices or prevDevices change

    let html = '';

    // Use systemData.hostname if available, otherwise fallback to clientHostname
    const displayHostname = (systemData && systemData.hostname) || clientHostname || 'LOCAL';
    html += `<div class="device-item current-host">
        <span class="device-dot online"></span>
        <span class="device-name">${displayHostname}</span>
    </div>`;

    // Ping-monitored devices (user specified)
    if (systemData && systemData.watchedDevices && systemData.watchedDevices.length > 0) {
        systemData.watchedDevices.forEach((dev: any) => {
            const dotClass = dev.online ? 'online' : 'offline';
            const latencyStr = dev.online && dev.latency != null
                ? `<span class="device-latency">${Number(dev.latency).toFixed(0)}ms</span>`
                : '';

            // Detect state change for animation
            const prev = prevDevices[dev.name];
            const became = prev == null ? false : (prev.online !== dev.online);
            const flashClass = became ? 'flash' : '';

            html += `<div class="device-item ${flashClass}">
                <span class="device-dot ${dotClass}"></span>
                <span class="device-name">${dev.name}</span>
                ${latencyStr}
            </div>`;
        });
    }

    // USB-connected devices
    if (systemData && systemData.devices && systemData.devices.length > 0) {
        systemData.devices.forEach((dev: any) => {
            html += `<div class="device-item">
                <span class="device-dot online"></span>
                <span class="device-name">${dev.name}</span>
            </div>`;
        });
    }

    // Fallback if no devices are found and no system data yet
    if (!systemData && !html) {
        html = '<div class="device-item fading"><span class="device-name">INITIALIZING...</span></div>';
    }


    return (
        <div className="module" id="mod-devices">
            <div className="module-inner" id="devices-content">
                <div className="section-label">DEVICES</div>
                <div id="devices-list" ref={devicesListRef} dangerouslySetInnerHTML={{ __html: html }}></div>
            </div>
            {/* Settings replacement - will be handled by the main Settings component */}
            <div className="module-settings hidden" id="settings-devices">
                {/* ... settings content ... */}
            </div>
        </div>
    );
}
