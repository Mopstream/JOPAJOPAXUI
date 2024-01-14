#ifndef LLP_LAB1_QUERY_H_
#define LLP_LAB1_QUERY_H_

#include "schema.h"
#include "node.h"

typedef enum {
    ADD,
    SELECT,
    DELETE,
    SET
} query_type;

typedef enum {
    Q_INDEX,
    Q_NODE,
    Q_LINK
} query_target;

//typedef enum {
//    GREATER,
//    LOWER,
//    EQUAL
//} cmp_t;
//
//typedef struct cond {
//    char attr_name[16];
//    cmp_t cmp;
//    value_t val;
//} cond_t;
//
//typedef struct{
//    uint32_t cond_cnt;
//    cond_t * conditionals;
//} select_q;

typedef union {
    node_t *node;
    link_t *link;
} add_q;

typedef union {
    char name[16];
    uint32_t id;
} delete_q;

typedef struct {
    uint32_t node_id;
    char attr_name[16];
    value_t new_value;
} set_q;

typedef struct {
    char filename[16];
    query_type q_type;
    query_target target;
    index_t *index;
    union {
        add_q add;
        select_q * sel;
        delete_q del;
        set_q set;
    } query_body;
} query_t;

char * exec(query_t * q);

#endif
