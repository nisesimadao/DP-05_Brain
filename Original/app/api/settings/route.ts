import { NextResponse } from 'next/server';
import fs from 'fs/promises';
import path from 'path';
import { broadcastSync } from '@/lib/sync';

export const dynamic = 'force-dynamic';

const SETTINGS_PATH = path.join(process.cwd(), 'settings.json');

async function getSettings() {
    try {
        const data = await fs.readFile(SETTINGS_PATH, 'utf-8');
        return JSON.parse(data);
    } catch (e) {
        return {
            nightMode: false,
            glowColor: 'green',
            burninGuard: false,
            burninCrtMode: false,
            fontWeight: 400,
            secondsScale: 82,
            thinOverride: false,
            monoEnforce: true,
            fontMain: 'var(--font-jetbrains-mono)',
            fontUI: 'var(--font-geist-sans)',
            reloadCounter: 0
        };
    }
}

async function saveSettings(settings: any) {
    await fs.writeFile(SETTINGS_PATH, JSON.stringify(settings, null, 2));
}

export async function GET() {
    const settings = await getSettings();
    return NextResponse.json(settings, {
        headers: {
            'Cache-Control': 'no-store, max-age=0, must-revalidate',
            'Pragma': 'no-cache',
            'Expires': '0',
        }
    });
}

export async function POST(request: Request) {
    const body = await request.json();
    const settings = await getSettings();

    const updatedSettings = { ...settings, ...body };
    await saveSettings(updatedSettings);

    // Broadcast the update to all SSE clients
    broadcastSync(updatedSettings);

    return NextResponse.json(updatedSettings);
}

