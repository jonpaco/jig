/* Auto-generated from schemas/element.json — do not edit */

/**
 * An element found on screen
 */
export interface Element {
/**
 * React Native internal view identifier
 */
reactTag: number
/**
 * Developer-assigned test identifier
 */
testID?: string
/**
 * Visible text content
 */
text?: string
/**
 * Accessibility role
 */
role?: string
/**
 * Screen coordinates and dimensions
 */
frame: {
x: number
y: number
width: number
height: number
}
/**
 * Whether the element is visible on screen
 */
visible: boolean
}

