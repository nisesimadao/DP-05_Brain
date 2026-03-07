import { NextResponse } from 'next/server';
import si from 'systeminformation';

export const dynamic = 'force-dynamic';

export async function GET() {
  try {
    const networkStats = await si.networkStats();
    const networkInterfaces = await si.networkInterfaces();
    const osInfo = await si.osInfo();
    const time = si.time();

    // Find the primary interface (the one with an IP or at the top)
    const primaryNet = networkStats[0] || { rx_sec: 0, tx_sec: 0 };

    // Find local IP from network interfaces
    const localIpInterface = (networkInterfaces as any[]).find(iface => !iface.internal && iface.ip4);
    const localIp = localIpInterface ? localIpInterface.ip4 : '0.0.0.0';

    const dummyWatchedDevices = [
      { name: 'Router', ip: '192.168.1.1', online: true, latency: 5 },
      { name: 'Google DNS', ip: '8.8.8.8', online: true, latency: 15 },
    ];

    const usb = await si.usb();
    const usbDevices = usb.map(d => ({
      name: d.name || d.device,
      vendor: d.vendor,
      id: (d as any).id || Math.random().toString(36).substr(2, 5)
    }));

    return NextResponse.json({
      network: {
        primary: {
          dl: (primaryNet.rx_sec || 0) / 1024 / 1024 * 8, // Mbps
          ul: (primaryNet.tx_sec || 0) / 1024 / 1024 * 8  // Mbps
        },
        total: { dl: 0, ul: 0 },
      },
      devices: usbDevices,
      watchedDevices: dummyWatchedDevices,
      hostname: osInfo.hostname || 'NextJS-Server',
      localIp: localIp,
      system: {
        uptime: time.uptime,
        platform: osInfo.platform,
        release: osInfo.release,
        arch: osInfo.arch
      }
    });
  } catch (error) {
    console.error('Error fetching system data:', error);
    return NextResponse.json({ error: 'Failed to fetch system data' }, { status: 500 });
  }
}
