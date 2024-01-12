#include "query_constructor.h"
#include <stdlib.h>
#include "stdio.h"
#include "schema.h"

attr_val_type_t attr_types[] = {
        [VAL_TYPE__INT] = INT,
        [VAL_TYPE__DOUBLE] = DOUBLE,
        [VAL_TYPE__STRING] = STRING,
        [VAL_TYPE__BOOL] = BOOL,
};

query_target targets[] = {
        [INSERT_TARGET__I_NODE] = Q_NODE,
        [INSERT_TARGET__I_LINK] = Q_LINK,
        [INSERT_TARGET__I_INDEX] = Q_INDEX,
};

query_t *construct(Ast *ast) {
    query_t *q = malloc(sizeof(query_t));
    q->filename = ast->val->name;
    Ast *query_ast = ast->left;
    Ast *index_ast = query_ast->left;
    if (!query_ast->right) {
        char *index_name = (char *) index_ast->val;
        Ast *attrs_list = index_ast->left;
        uint32_t cnt = attrs_list->val->cnt;
        attr_type_t *attrs = malloc(cnt * sizeof(attr_type_t));
        for (uint32_t i = 0; i < cnt; ++i) {
            Ast *attr = attrs_list->left;
            AttrType *curr_attr = attr->val->attr_type;
            attrs[i].type_name = curr_attr->name;
            attrs[i].type = attr_types[curr_attr->val];
            attrs_list = attrs_list->right;
        }

        q->index = create_index(index_name, attrs, cnt);
        q->q_type = ADD;
        q->target = Q_INDEX;
        q->query_body = 0;
        return q;
    }
    schema_t *schema = init_db(q->filename);
    q->index = get_first_index(schema, index_ast->val->name);
    close(schema->fd);
    free(schema);
    if (query_ast->type == AST_NODE_TYPE__UPDATE_N) {
        q->q_type = SET;
        q->target = Q_NODE;
        Set *curr_set = query_ast->val->set;
        Attr *curr_attr = curr_set->new_value;
        q->query_body.set = (set_q) {.node_id = curr_set->node_id,
                .attr_name = curr_set->attr_name};
        switch (curr_attr->type) {
            case VAL_TYPE__INT: {
                q->query_body.set.new_value.i = curr_attr->i;
                break;
            }
            case VAL_TYPE__DOUBLE: {
                q->query_body.set.new_value.d = curr_attr->d;
                break;
            }
            case VAL_TYPE__STRING: {
                q->query_body.set.new_value.str = (string_t) {.size = curr_attr->str->size, .str = curr_attr->str->str};
                break;
            }
            case VAL_TYPE__BOOL: {
                q->query_body.set.new_value.b = curr_attr->b;
                break;
            }
            default: {
                fprintf(stderr, "PROGRAMMAN TUPOY!!!!\n");
            }
        }
        return q;
    }
    Ast *body = query_ast->right;
    if (query_ast->type == AST_NODE_TYPE__DELETE_N) {
        q->q_type = DELETE;
        if (!body) {
            q->target = Q_INDEX;
        } else {
            q->target = Q_NODE;
            Attr *id = body->val->attr;
            q->query_body.del.id = id->i;
        }
        return q;
    }
    switch (query_ast->type) {
        case AST_NODE_TYPE__INSERT_N: {
            q->q_type = ADD;
            switch (body->val->target) {
                case INSERT_TARGET__I_NODE: {
                    q->target = Q_NODE;
                    node_t *node = malloc(sizeof(node_t));
                    q->query_body.add.node = node;

                    break;
                }
                case INSERT_TARGET__I_LINK: {
                    q->target = Q_LINK;
                    link_t *link = malloc(sizeof(link_t));
                    q->query_body.add.link = link;
                    Link *curr_link = body->val->link;
                    link->node_from_id = curr_link->node_from_id;
                    link->node_from_type_id = curr_link->node_from_type_id;
                    link->node_to_id = curr_link->node_to_id;
                    link->node_to_type_id = curr_link->node_to_type_id;
                    break;
                }
                case
                default: {
                    fprintf(stderr, "PROGRAMMAN TUPOY!!!!\n");
                }

            }
            return q;
        }
        case AST_NODE_TYPE__SELECT_N: {
            q->q_type = SELECT;
            break;
        }
        default: {
            fprintf(stderr, "PROGRAMMAN TUPOY!!!!\n");
        }
    }
    return q;
}
