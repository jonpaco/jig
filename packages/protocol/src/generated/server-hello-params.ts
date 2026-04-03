/* Auto-generated from schemas/server-hello-params.json — do not edit */

/**
 * Params for the server.hello notification sent immediately on WebSocket connect
 */
export interface ServerHelloParams {
protocol: {
name: string
version: number
min: number
max: number
}
/**
 * Jig server version (semver)
 */
server: string
app: {
name: string
bundleId: string
platform: ("ios" | "android")
rn: string
expo?: string
}
commands: string[]
domains: string[]
}

