#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#include <types.h>

/*
 * Core exception handling functions
 */
void exception_handler_c(uint32_t type);
void install_exception_vectors(void);

/*
 * Exception information display
 */
void dump_exception_info(uint32_t type, uint64_t esr, uint64_t elr, 
                         uint64_t spsr, uint64_t far);

/*
 * Helper functions
 */
const char *exception_type_string(uint32_t type);

#endif /* _EXCEPTION_H */
