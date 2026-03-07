import { useState, useEffect, useRef, useCallback } from 'react';
import { useAppContext } from '../context/AppContext';
import NetworkStats from './NetworkStats';

interface SystemData {
    network: {
        primary: { dl: number; ul: number };
        total: { dl: number; ul: number };
    };
    devices: any[]; // USB devices
    watchedDevices: Array<{ name: string; ip: string; online: boolean; latency?: number }>; // Ping-monitored
    hostname: string;
}

interface NetworkMeasurement {
    dl: number | null;
    ul: number | null;
}

export const useSystemData = () => {
    const { state, updateGlobalState } = useAppContext();
    const [systemData, setSystemData] = useState<SystemData | null>(null);
    const [dlHistory, setDlHistory] = useState<number[]>([]);
    const [ulHistory, setUlHistory] = useState<number[]>([]);
    const [measuredDl, setMeasuredDl] = useState<number>(0);
    const [measuredUl, setMeasuredUl] = useState<number>(0);
    const [measureAvailable, setMeasureAvailable] = useState<boolean>(false);
    const [prevDevices, setPrevDevices] = useState<any>({});
    const hasFetchedIp = useRef(false);
    const prevTraffic = useRef<{ rx: number; tx: number; time: number } | null>(null);


    // Helper functions (moved from main.js)
    const fetchPublicIp = useCallback(async () => {
        try {
            const res = await fetch('https://api.ipify.org?format=json');
            if (!res.ok) return null;
            const j = await res.json();
            return j.ip;
        } catch (e) {
            console.error('Error fetching public IP:', e);
            return null;
        }
    }, []);

    const discoverLocalIps = useCallback(() => {
        return new Promise<string[]>((resolve) => {
            const ips = new Set<string>();
            const pc = new RTCPeerConnection({ iceServers: [] });
            pc.createDataChannel('');
            pc.onicecandidate = (e) => {
                if (!e.candidate) {
                    pc.close();
                    resolve(Array.from(ips));
                    return;
                }
                const parts = e.candidate.candidate.split(' ');
                for (let i = 0; i < parts.length; i++) {
                    const p = parts[i];
                    if (/^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$/.test(p)) {
                        ips.add(p);
                    }
                }
            };
            pc.createOffer().then(o => pc.setLocalDescription(o)).catch(() => resolve([]));
            setTimeout(() => {
                try { pc.close(); } catch (e) { }
                resolve(Array.from(ips));
            }, 2000);
        });
    }, []);

    const measureDownloadSpeedOnce = useCallback(async (): Promise<NetworkMeasurement | null> => {
        const nav: any = (navigator as any).connection || (navigator as any).mozConnection || (navigator as any).webkitConnection;
        if (nav && nav.downlink) {
            return { dl: nav.downlink, ul: null };
        }

        const urls = [
            'https://speed.hetzner.de/1MB.bin',
            'https://download.samplelib.com/mp3/sample-3s.mp3',
            'https://upload.wikimedia.org/wikipedia/commons/3/3f/Fronalpstock_big.jpg'
        ];

        for (const url of urls) {
            try {
                const t0 = performance.now();
                const res = await fetch(url, { method: 'GET', cache: 'no-cache' });
                if (!res.ok) continue;
                const blob = await res.blob();
                const t1 = performance.now();
                const bytes = blob.size;
                const secs = Math.max(0.001, (t1 - t0) / 1000);
                const mbps = (bytes * 8) / (secs * 1000 * 1000);
                if (mbps > 0.1) return { dl: mbps, ul: null };
            } catch (e) {
                // try next
            }
        }
        return null;
    }, []);

    const measureUploadSpeedOnce = useCallback(async (): Promise<number | null> => {
        // Disabled httpbin.org due to CORS issues
        return null;
    }, []);

    const clamp = (v: number, min: number, max: number) => Math.max(min, Math.min(max, v));

    const simulateNetworkStats = useCallback((currentDl: number, currentUl: number) => {
        const newDl = currentDl + (state.targetDl - currentDl) * 0.08 + (Math.random() - 0.5) * 2;
        const newUl = currentUl + (state.targetUl - currentUl) * 0.08 + (Math.random() - 0.5) * 0.6;
        return {
            dl: clamp(newDl, 0, 1000),
            ul: clamp(newUl, 0, 1000),
        };
    }, [state.targetDl, state.targetUl]);


    const refreshNetworkMeasurements = useCallback(async () => {
        try {
            const measured = await measureDownloadSpeedOnce();
            const up = await measureUploadSpeedOnce();

            let currentDlVal = measuredDl;
            let currentUlVal = measuredUl;

            if (measured && measured.dl) {
                setMeasureAvailable(true);
                currentDlVal = currentDlVal === 0 ? measured.dl : currentDlVal * 0.85 + measured.dl * 0.15;
            }
            if (up && up > 0) {
                currentUlVal = currentUlVal === 0 ? up : currentUlVal * 0.85 + up * 0.15;
            } else {
                currentUlVal = currentUlVal === 0 ? Math.max(0.1, state.ulSpeed) : currentUlVal * 0.92 + Math.max(0.1, state.ulSpeed) * 0.08;
            }

            setMeasuredDl(currentDlVal);
            setMeasuredUl(currentUlVal);

            const MAX_HISTORY = 60;
            setDlHistory(prev => {
                const newHistory = [...prev, currentDlVal || state.dlSpeed];
                if (newHistory.length > MAX_HISTORY) newHistory.shift();
                return newHistory;
            });
            setUlHistory(prev => {
                const newHistory = [...prev, currentUlVal || state.ulSpeed];
                if (newHistory.length > MAX_HISTORY) newHistory.shift();
                return newHistory;
            });

            updateGlobalState({
                measuredDl: currentDlVal,
                measuredUl: currentUlVal,
                measureAvailable: measureAvailable,
            });

        } catch (e) {
            console.warn('Network measurement failed', e);
        }
    }, [measureDownloadSpeedOnce, measureUploadSpeedOnce, state.dlSpeed, state.ulSpeed, measuredDl, measuredUl, measureAvailable, updateGlobalState]);

    // Effect for initial IP discovery (runs once)
    useEffect(() => {
        if (hasFetchedIp.current) return;
        hasFetchedIp.current = true;

        (async () => {
            const pub = await fetchPublicIp();
            if (pub) updateGlobalState({ publicIp: pub });
            const locals = await discoverLocalIps();
            if (locals && locals.length) updateGlobalState({ localIp: locals[0] });
        })();
    }, [fetchPublicIp, discoverLocalIps, updateGlobalState]);


    // Main data fetching and network measurement effect
    useEffect(() => {
        let systemFetchInterval: NodeJS.Timeout;
        let networkMeasurementInterval: NodeJS.Timeout;

        const fetchData = async () => {
            const isNative = typeof window !== 'undefined' && (window as any).Capacitor && (window as any).Capacitor.isNative;

            try {
                if (!isNative) {
                    // Try fetching from internal API only if not native
                    const res = await fetch('/api/system');
                    if (res.ok) {
                        const data: SystemData = await res.json();
                        setSystemData(data);

                        updateGlobalState({
                            dlSpeed: data.network.primary.dl,
                            ulSpeed: data.network.primary.ul,
                            localIp: (data as any).localIp || state.localIp
                        });

                        const newPrevDevices: any = {};
                        (data.watchedDevices || []).forEach((dev: any) => {
                            newPrevDevices[dev.name] = { online: dev.online };
                        });
                        setPrevDevices(newPrevDevices);
                        return; // Successfully fetched from API
                    }
                }
                throw new Error('Fallback to Native');
            } catch (err) {
                // FALLBACK: Capacitor / Local Simulation
                try {
                    const { Device } = await import('@capacitor/device');
                    const { Network } = await import('@capacitor/network');

                    // Get Real Device Info via Capacitor
                    const info = await Device.getInfo();
                    const netStatus = await Network.getStatus();

                    const hostname = `${info.model} (${info.operatingSystem})`;

                    let realDl = 0;
                    let realUl = 0;

                    // Use custom plugin for real traffic stats on Android
                    try {
                        const stats = await NetworkStats.getTrafficStats();
                        if (prevTraffic.current) {
                            const dt = (stats.timestamp - prevTraffic.current.time) / 1000; // seconds
                            if (dt > 0) {
                                // (bytes_diff * 8 bits) / (dt * 1,000,000) = Mbps
                                realDl = ((stats.rxBytes - prevTraffic.current.rx) * 8) / (dt * 1000000);
                                realUl = ((stats.txBytes - prevTraffic.current.tx) * 8) / (dt * 1000000);

                                // Clean up spikes and negative values (TrafficStats resets sometimes)
                                if (realDl < 0 || realDl > 2000) realDl = 0;
                                if (realUl < 0 || realUl > 2000) realUl = 0;
                            }
                        }
                        prevTraffic.current = { rx: stats.rxBytes, tx: stats.txBytes, time: stats.timestamp };
                    } catch (e) {
                        // If plugin fails, fall back to simulation based on connection type
                        const baseDl = netStatus.connectionType === 'wifi' ? 150 : 30;
                        const baseUl = netStatus.connectionType === 'wifi' ? 40 : 10;
                        const sim = simulateNetworkStats(state.dlSpeed || baseDl, state.ulSpeed || baseUl);
                        realDl = sim.dl;
                        realUl = sim.ul;
                    }

                    updateGlobalState({
                        dlSpeed: realDl,
                        ulSpeed: realUl,
                    });

                    // Still using devices.json for the list of devices to watch
                    let watched = [];
                    try {
                        const res2 = await fetch('/devices.json');
                        if (res2.ok) {
                            const local = await res2.json();
                            watched = local.watchDevices || local.watchdevices || [];
                        }
                    } catch (e) {
                        console.warn('Could not fetch devices.json, using defaults');
                    }

                    const simulatedWatchedDevices = watched.map((w: any) => ({
                        name: w.name,
                        ip: w.ip,
                        online: Math.random() > 0.1, // Real pinging from Android would need another plugin
                        latency: Math.random() * 50 + 5
                    }));

                    setSystemData({
                        network: {
                            primary: { dl: realDl, ul: realUl },
                            total: { dl: realDl, ul: realUl },
                        },
                        devices: [],
                        watchedDevices: simulatedWatchedDevices,
                        hostname,
                    });

                    const newPrevDevices: any = {};
                    (simulatedWatchedDevices || []).forEach((dev: any) => {
                        newPrevDevices[dev.name] = { online: dev.online };
                    });
                    setPrevDevices(newPrevDevices);
                } catch (err2) {
                    console.error('Capacitor fallback failed:', err2);
                }
            }
        };

        // Initial fetches
        fetchData();
        refreshNetworkMeasurements();

        systemFetchInterval = setInterval(fetchData, 1000);
        networkMeasurementInterval = setInterval(refreshNetworkMeasurements, 15000);

        return () => {
            clearInterval(systemFetchInterval);
            clearInterval(networkMeasurementInterval);
        };
    }, [refreshNetworkMeasurements, simulateNetworkStats, updateGlobalState, state.dlSpeed, state.ulSpeed]); // Refined dependencies

    return { systemData, dlHistory, ulHistory, prevDevices };
};
