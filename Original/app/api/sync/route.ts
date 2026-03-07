import { NextRequest } from 'next/server';
import { syncEmitter, incrementClientCount, decrementClientCount } from '@/lib/sync';

export const dynamic = 'force-dynamic';

export async function GET(req: NextRequest) {
    const encoder = new TextEncoder();

    const stream = new ReadableStream({
        start(controller) {
            incrementClientCount();

            const onSync = (data: any) => {
                try {
                    const message = `data: ${JSON.stringify({ type: 'sync', data })}\n\n`;
                    controller.enqueue(encoder.encode(message));
                } catch (e) {
                    cleanup();
                }
            };

            const heartbeatInterval = setInterval(() => {
                try {
                    controller.enqueue(encoder.encode(': heartbeat\n\n'));
                } catch (e) {
                    cleanup();
                }
            }, 10000);

            const cleanup = () => {
                clearInterval(heartbeatInterval);
                syncEmitter.off('sync', onSync);
                try { controller.close(); } catch (e) { }
            };

            syncEmitter.on('sync', onSync);

            // Initial connection confirmation
            controller.enqueue(encoder.encode('data: {"type": "connected"}\n\n'));

            req.signal.addEventListener('abort', () => {
                decrementClientCount();
                cleanup();
            });
        },
    });

    return new Response(stream, {
        headers: {
            'Content-Type': 'text/event-stream',
            'Cache-Control': 'no-cache, no-transform, no-store',
            'Connection': 'keep-alive',
            'X-Accel-Buffering': 'no',
        },
    });
}
