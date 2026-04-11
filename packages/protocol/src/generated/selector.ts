/* Auto-generated from schemas/selector.json — do not edit */

/**
 * Element selector for jig.findElement and jig.findElements
 */
export interface Selector {
/**
 * Match by testID (accessibilityIdentifier on iOS, contentDescription on Android)
 */
testID?: string
/**
 * Match by visible text content
 */
text?: string
/**
 * Match by accessibility role
 */
role?: string
/**
 * When multiple elements match, select the Nth (0-based)
 */
index?: number
within?: Selector1
selector?: Selector2
}
/**
 * Scope search to descendants of the element matching this selector
 */
export interface Selector1 {
/**
 * Match by testID (accessibilityIdentifier on iOS, contentDescription on Android)
 */
testID?: string
/**
 * Match by visible text content
 */
text?: string
/**
 * Match by accessibility role
 */
role?: string
/**
 * When multiple elements match, select the Nth (0-based)
 */
index?: number
within?: Selector1
selector?: Selector2
}
/**
 * Inner selector to match within the scoped container
 */
export interface Selector2 {
/**
 * Match by testID (accessibilityIdentifier on iOS, contentDescription on Android)
 */
testID?: string
/**
 * Match by visible text content
 */
text?: string
/**
 * Match by accessibility role
 */
role?: string
/**
 * When multiple elements match, select the Nth (0-based)
 */
index?: number
within?: Selector1
selector?: Selector2
}

