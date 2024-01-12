#ifndef LLP_LAB1_NODE_H_
#define LLP_LAB1_NODE_H_

#include <stdint.h>
#include <stdbool.h>

//#include "schema.h"


typedef struct {
    uint32_t link_id;
    uint32_t node_from_type_id;
    uint32_t node_from_id;
    uint32_t node_to_type_id;
    uint32_t node_to_id;
} link_t;

typedef struct {
    uint32_t link_type_id;
    uint32_t link_id;
} link_ref_t;

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

typedef struct {
    uint32_t id;
    value_t * attrs;

    uint32_t out_cnt;
    link_ref_t *links_out;
    uint32_t in_cnt;
    link_ref_t *links_in;
} node_t;

#endif
