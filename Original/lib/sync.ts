import { EventEmitter } from 'events';

// Singleton for the server-side event emitter
// Using globalThis to persist across HMR in dev mode
const globalSync = globalThis as unknown as {
    syncEmitter?: EventEmitter;
    activeClients: number;
};

if (!globalSync.syncEmitter) {
    globalSync.syncEmitter = new EventEmitter();
    globalSync.syncEmitter.setMaxListeners(100);
    globalSync.activeClients = 0;
}

export const syncEmitter = globalSync.syncEmitter;

export function getActiveClientCount() {
    return globalSync.activeClients;
}

export function incrementClientCount() {
    globalSync.activeClients++;
}

export function decrementClientCount() {
    globalSync.activeClients = Math.max(0, globalSync.activeClients - 1);
}

export function broadcastSync(data: any) {
    console.log(`[SyncLib] Broadcasting to active clients (Count: ${globalSync.activeClients})`);
    syncEmitter.emit('sync', data);
}
