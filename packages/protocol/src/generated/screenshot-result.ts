/* Auto-generated from schemas/screenshot-result.json — do not edit */

/**
 * Result of the jig.screenshot command
 */
export interface ScreenshotResult {
/**
 * Base64-encoded image bytes
 */
data: string
/**
 * Actual format used
 */
format: ("png" | "jpeg")
/**
 * Image width in pixels
 */
width: number
/**
 * Image height in pixels
 */
height: number
/**
 * Unix epoch milliseconds when capture occurred
 */
timestamp: number
}

