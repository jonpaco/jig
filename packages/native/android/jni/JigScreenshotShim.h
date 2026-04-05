#ifndef JIG_SCREENSHOT_SHIM_H
#define JIG_SCREENSHOT_SHIM_H

#include "../../core/jig_handler.h"

/* Create the Android screenshot handler (JIG_THREAD_MAIN).
 * Caller owns the returned handler. */
jig_handler *jig_screenshot_handler_create(void);

#endif /* JIG_SCREENSHOT_SHIM_H */
