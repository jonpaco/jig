/* Auto-generated from schemas/element-not-found-error.json — do not edit */

/**
 * Error data for ELEMENT_NOT_FOUND (-32002)
 */
export interface ElementNotFoundError {
/**
 * The selector that failed to match
 */
selector: {

}
/**
 * Fuzzy-matched testID/text/component values sorted by edit distance
 */
suggestions: string[]
/**
 * Summary of currently visible elements for debugging
 */
visibleElements: {
testID?: string
component?: string
}[]
}

