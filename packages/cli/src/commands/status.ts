import { connect, type ConnectOptions } from '@jig/sdk';

export async function status(options?: ConnectOptions): Promise<string> {
  const session = await connect(options);
  const { app, protocol, server } = session.serverHello;

  const lines = [
    `✓ Connected to ${app.name} (${app.bundleId})`,
    `  Platform:  ${app.platform}`,
    `  RN:        ${app.rn}`,
    ...(app.expo ? [`  Expo:      ${app.expo}`] : []),
    `  Jig:       ${server}`,
    `  Protocol:  ${protocol.name}/${protocol.version}`,
    `  Session:   ${session.sessionId}`,
  ];

  session.disconnect();
  return lines.join('\n');
}
