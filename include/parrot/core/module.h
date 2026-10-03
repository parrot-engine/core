#ifndef PARROT_CORE_MODULE_H_
#define PARROT_CORE_MODULE_H_

#include "parrot/core/reflect.h"

static const ParrotReflectDescription Parrot_core_collection[] = {
    PARROT_REFLECT_COLLECTION_HEADER(),

#ifdef PARROT_CORE_MATH_H_
    PARROT_REFLECT_COLLECTION_DESCRIPTION(Parrot_core_math_collection),
#endif
#ifdef PARROT_CORE_BINARY_H_
    PARROT_REFLECT_COLLECTION_DESCRIPTION(Parrot_core_binary_collection),
#endif
#ifdef PARROT_CORE_MAIN_LOOP_
    PARROT_REFLECT_COLLECTION_DESCRIPTION(Parrot_core_main_loop_collection),
#endif

    PARROT_REFLECT_END(),
};

#endif // PARROT_CORE_MODULE_H_
