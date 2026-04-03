/* Auto-generated from schemas/session-ready-result.json — do not edit */

/**
 * Result payload for the session.ready response confirming the handshake
 */
export interface SessionReadyResult {
sessionId: string
negotiated: {
protocol: number
}
enabled: string[]
rejected: string[]
}

