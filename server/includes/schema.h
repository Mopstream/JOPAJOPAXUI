#ifndef LLP_LAB1_SCHEMA_H_
#define LLP_LAB1_SCHEMA_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    INT,
    DOUBLE,
    STRING,
    BOOL
} attr_val_type_t;

typedef struct {
    char type_name[16];
    attr_val_type_t type;
} attr_type_t;

typedef struct {
    uint32_t type_id;
    uint32_t attr_count;
    attr_type_t *attr_types;
} node_type_t;

typedef struct link_type {
    uint32_t type_id;
    char type_name[16];
} link_type_t;


typedef struct {
    uint32_t fd;
    uint32_t first_index;
    uint32_t free_page;
    uint32_t cnt_free;
    uint32_t last_page_num;
} schema_t;

struct page;
struct find_context {
    struct page *page;
    uint32_t i;
    void *thing;
};

#include "index.h"
#include "node.h"

typedef struct {
    uint32_t size;
    char *data;
} chunk_t;

schema_t *init_db(char *filename);

chunk_t *encode_index(index_t *index);

index_t *decode_index(char *data);

void add_index(index_t *index, schema_t *schema);

void delete_index_by_name(schema_t *schema, char name[16]);

void add_node(schema_t *schema, index_t *index, node_t *node);

void add_link(schema_t *schema, index_t *index, link_t *link);

void free_index(index_t *index);

void delete_link_by_id(schema_t *schema, index_t * index, uint32_t link_id);

void delete_node_by_id(schema_t *schema, index_t *index, uint32_t node_id);

void set_node_attribute(schema_t *schema, index_t *index, uint32_t node_id, char attr_name[16], value_t new_value);

void index_enumerate(schema_t * schema);

typedef enum {
    GREATER,
    LOWER,
    EQUAL
} cmp_t;

typedef struct cond {
    char attr_name[16];
    cmp_t cmp;
    value_t val;
} cond_t;

typedef struct{
    uint32_t cond_cnt;
    cond_t * conditionals;
} select_q;
void node_enumerate(schema_t *schema, index_t *index, select_q * select);
void link_enumerate(schema_t *schema, index_t *index);
index_t * get_first_index(schema_t *schema, char name[16]);
#endif
