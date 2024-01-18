#ifndef LLP_LAB1_SCHEMA_H_
#define LLP_LAB1_SCHEMA_H_

#include <stdint.h>
#include <stdbool.h>
#include "../../spec.pb-c.h"

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
    uint32_t attr_count;
    attr_type_t *attr_types;
} node_type_t;

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

Response *add_index(index_t *index, schema_t *schema);

Response *delete_index_by_name(schema_t *schema, char name[16]);

Response *add_node(schema_t *schema, index_t *index, node_t *node);

Response *add_link(schema_t *schema, index_t *index, link_t *link);

void free_index(index_t *index);

Response *delete_link_by_id(schema_t *schema, index_t * index, uint32_t link_id);

Response *delete_node_by_id(schema_t *schema, index_t *index, uint32_t node_id);

Response *set_node_attribute(schema_t *schema, index_t *index, uint32_t node_id, char attr_name[16], value_t new_value);

Response * index_enumerate(schema_t * schema);

typedef enum {
    GREATER,
    LOWER,
    EQUAL,
    GREATER_EQUAL,
    LOWER_EQUAL,
    SUBSTR,
} cmp_t;

typedef struct cond {
    char *attr_name;
    cmp_t cmp;
    value_t val;
} cond_t;

typedef struct{
    uint32_t cond_cnt;
    cond_t * conditionals;
} select_q;
Response * node_enumerate(schema_t *schema, index_t *index, select_q * select);
Response *link_enumerate(schema_t *schema, index_t *index);
index_t * get_first_index(schema_t *schema, char name[16]);
index_t *create_index(char name[16], attr_type_t * attrs, uint32_t cnt, element_kind_t kind);
node_t *create_node(value_t*values, uint32_t cnt);
link_t *create_link(uint32_t node_from_id, uint32_t node_from_type_id, uint32_t node_to_id, uint32_t node_to_type_id);
Index *construct_index(index_t *index);
NodeType *convert_node_description(node_type_t type);
AttrType *convert_attr_type(attr_type_t type);
Node * construct_node(node_t* node, index_t * index);
LinkRef *convert_link_ref(link_ref_t link);
Link *convert_link(link_t * link);
#endif
