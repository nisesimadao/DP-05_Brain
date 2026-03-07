import { registerPlugin } from '@capacitor/core';

export interface NetworkStatsPlugin {
  getTrafficStats(): Promise<{ rxBytes: number; txBytes: number; timestamp: number }>;
}

const NetworkStats = registerPlugin<NetworkStatsPlugin>('NetworkStats');

export default NetworkStats;
