#ifndef LLP_LAB1_INDEX_H
#define LLP_LAB1_INDEX_H

#include <stdint.h>


typedef enum {
    I_NODE,
    I_LINK
} element_kind_t;

typedef struct index {
    uint32_t size;
    char type_name[16];
    element_kind_t kind;
    uint32_t type_id;
    node_type_t description;

    uint32_t count;
    uint32_t first_page_num;
} index_t;

#endif
