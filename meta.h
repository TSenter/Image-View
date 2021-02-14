#ifndef __IV_META_H
#define __IV_META_H
#include "utils/types.h"

typedef struct meta {
    char key[80];
    IVValue val;
} meta_t;
#endif