import { connect, type ConnectionOptions } from '../client/connection';

export async function status(options?: ConnectionOptions): Promise<string> {
  const result = await connect(options);
  const { serverHello, session } = result;
  const { app, protocol, server } = serverHello;

  const lines = [
    `✓ Connected to ${app.name} (${app.bundleId})`,
    `  Platform:  ${app.platform}`,
    `  RN:        ${app.rn}`,
    ...(app.expo ? [`  Expo:      ${app.expo}`] : []),
    `  Jig:       ${server}`,
    `  Protocol:  ${protocol.name}/${protocol.version}`,
    `  Session:   ${session.sessionId}`,
  ];

  return lines.join('\n');
}
