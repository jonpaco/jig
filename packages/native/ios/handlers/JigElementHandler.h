#ifndef JIG_ELEMENT_HANDLER_H
#define JIG_ELEMENT_HANDLER_H

#include "../../core/jig_handler.h"

jig_handler *jig_find_element_handler_create(void);
jig_handler *jig_find_elements_handler_create(void);
void jig_find_element_handler_destroy(jig_handler *handler);
void jig_find_elements_handler_destroy(jig_handler *handler);

#endif
