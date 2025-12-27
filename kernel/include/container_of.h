#ifndef _CONTAINER_OF_H
#define _CONTAINER_OF_H

#include <stddef.h>
#define __same_type(a, b)	__builtin_types_compatible_p(typeof(a), typeof(b))
/*
 * This macro takes a pointer to a structure member and returns a pointer
 * to the structure that contains that member.
 *
 * Type safety:
 * A compile-time check is performed to ensure that the type of the object
 * pointed to by @ptr matches the declared type of @member within @type.
 *
 * HOW THE TYPE CHECK WORKS:
 * The static_assert inside this macro performs a compile-time verification
 * that @ptr points to the correct member type.
 *
 *   (type *)0
 *     - This creates a NULL pointer to @type.
 *     - No memory is accessed or dereferenced.
 *     - The pointer exists only so the compiler can reason about structure
 *       layout and member types.
 *
 *   ((type *)0)->member
 *     - This asks the compiler: "If a @type object started at address 0,
 *       what would the type of @member be?"
 *     - The expression is never evaluated at runtime.
 *     - It is used purely to extract the declared type of @member.
 *
 *   *(ptr)
 *     - Dereferencing @ptr yields the type of the object it points to.
 *     - This does NOT generate a memory access in the static_assert;
 *       it is used only for type inspection.
 *
 *   __same_type(*(ptr), ((type *)0)->member)
 *     - Verifies that @ptr points to the same type as @member.
 *
 *   __same_type(*(ptr), void)
 *     - Allows @ptr to be a void pointer (void *) as an explicit opt out.
 *
 * HOW THE ADDRESS IS COMPUTED:
 *
 * The container address is calculated as:
 *
 *   container_address = member_address - offsetof(type, member)
 *
 * offsetof(type, member) is a compile-time constant representing the byte
 * offset of @member within @type.
 *
 * No memory access occurs during this computation;
*/
#define container_of(ptr, type, member) ({                              \
    void *__mptr = (void *)(ptr);                                       \
    static_assert(__same_type(*(ptr), ((type *)0)->member) ||           \
                  __same_type(*(ptr), void),                            \
                  "container_of(): pointer type mismatch");             \
    (type *)(__mptr - offsetof(type, member));                          \
})

#endif	/* _CONTAINER_OF_H */