#ifndef LLP_LAB2_TYPES_H
#define LLP_LAB2_TYPES_H

#include "stdint.h"
#include "stdbool.h"

typedef struct {
    uint32_t size;
    char* str;
} string_t;

typedef union {
    int32_t i;
    double d;
    string_t str;
    bool b;
} value_t;

typedef enum {
    INT = 0,
    DOUBLE = 1,
    STRING = 2,
    BOOL = 3,
} val_type_t;

typedef struct {
    char* name;
    val_type_t type;
} attr_desc_t;

typedef struct {
    val_type_t type;
    value_t val;
} attr_t;

typedef enum {
    GT = 0,
    LT = 1,
    EQ = 2,
    GT_EQ = 3,
    LT_EQ = 4,
    SUBSTR = 5,
} cmp_t;

#endif
