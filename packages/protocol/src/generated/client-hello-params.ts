/* Auto-generated from schemas/client-hello-params.json — do not edit */

/**
 * Params for the client.hello request declaring protocol version and domain subscriptions
 */
export interface ClientHelloParams {
protocol: {
version: number
}
client: {
name: string
version: string
}
enable?: string[]
subscribe?: {
[k: string]: {
[k: string]: unknown
}
}
}

