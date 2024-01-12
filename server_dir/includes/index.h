#ifndef LLP_LAB1_INDEX_H
#define LLP_LAB1_INDEX_H

#include <stdint.h>


typedef enum {
    I_NODE,
    I_LINK
} element_kind_t;

typedef struct {
    uint32_t size;
    char type_name[16];
    element_kind_t kind;
    union {
        node_type_t node;
        link_type_t link;
    } description;
} element_type_t;

typedef struct index {
    element_type_t type;
    uint32_t count;
    uint32_t first_page_num;
} index_t;

#endif //LLP_LAB1_INDEX_H
