#include "printer.h"
#include "ast.h"
#include "types.h"
#include "stdio.h"

char *node_types_p[] = {
        [FILENAME_N] = "filename",
        [INSERT_N] = "insert",
        [SELECT_N] = "select",
        [UPDATE_N] = "update",
        [DELETE_N] = "delete",
        [INDEX_N] = "index",
        [NODE_N] = "node",
        [LINK_N] = "link",
        [CONDITIONAL_N] = "conditional",
        [FILTER_N] = "filter",
        [SET_N] = "set",
        [VAL_N] = "val",
        [NAME_N] = "name",
        [LIST_N] = "list",
};

char *insert_targets_p[] = {
        [I_INDEX] = "index",
        [I_LINK] = "link",
        [I_NODE] = "node",
};

char *logical_ops_p[] = {
        [AND] = "and",
        [OR] = "or",
        [NOT] = "not",
};

char *comparators[] = {
        [GT] = "greater",
        [LT] = "less",
        [EQ] = "equal",
        [GT_EQ] = "greater or equal",
        [LT_EQ] = "less or equal",
        [SUBSTR] = "contains as substring",
};

char *value_types_p[] = {
        [NONE] = "none",
        [NAME] = "name",
        [INSERT_TARGET] = "insert_target",
        [SET] = "set",
        [ATTR] = "attr",
        [LOGICAL_OP] = "logical_op",
        [CMP] = "cmp",
        [LINK] = "link",
};

void print_attr(attr_t *attr, int32_t nesting_level) {
    switch (attr->type) {
        case INT: {
            printf("%*svalue = %d\n", nesting_level, "", attr->val.i);
            break;
        }
        case DOUBLE: {
            printf("%*svalue = %f\n", nesting_level, "", attr->val.d);
            break;
        }
        case BOOL: {
            if (attr->val.b) {
                printf("%*svalue = TRUE\n", nesting_level, "");
            } else {
                printf("%*svalue = FALSE\n", nesting_level, "");
            }
        }
        case STRING: {
            printf("%*svalue = %s\n", nesting_level, "", attr->val.str.str);
            break;
        }
    }
}

void print_value(ast_node *node, int32_t nesting_level) {
    switch (node->type) {
        case FILENAME_N: {
            printf("%*s%s\n", nesting_level, "", (char *) node->value);
            break;
        }
        case INSERT_N: {
            printf("%*s%s\n", nesting_level, "", insert_targets_p[*(insert_target_t *) (node->value)]);
            break;
        }
        case UPDATE_N: {
            set_t *s = node->value;
            printf("%*snode to update id = %d\n", nesting_level, "", s->node_id);
            printf("%*sattr to update name = %s\n", nesting_level, "", s->attr_name);
            print_attr(&s->new_value, nesting_level);
        }
        case INDEX_N: {
            if (node->value) {
                printf("%*sname: %s\n", nesting_level, "", (char *) (node->value));
                break;
            }
        }
        case FILTER_N: {
            printf("%*slogical operator: %s\n", nesting_level, "", logical_ops_p[*(logical_op *) (node->value)]);
            break;
        }
        case CONDITIONAL_N: {
            printf("%*scomparator: %s\n", nesting_level, "", comparators[*(cmp_t *) (node->value)]);
            break;
        }
        case LINK_N: {
            link_t *l = node->value;
            printf("%*snode_from_type_id = %d, node_from_id = %d\n", nesting_level, "", l->node_from_type_id,
                   l->node_from_id);
            printf("%*snode_to_type_id = %d, node_to_id = %d\n", nesting_level, "", l->node_to_type_id, l->node_to_id);
            break;
        }
        case VAL_N: {
            print_attr(node->value, nesting_level);
            break;
        }
        case NAME_N: {
            printf("%*s%s\n", nesting_level, "", (char *) node->value);
            break;
        }
        default:
            printf("%*s%s\n", nesting_level, "", "none");
            break;
    }
}

void print_node(ast_node *node, int32_t nesting_level) {
    if (node) {
        printf("%*sNode: %s\n", nesting_level, "", node_types_p[node->type]);
        printf("%*sValue Type: %s\n", nesting_level, "", value_types_p[node->v_type]);
        printf("%*sValue:\n", nesting_level, "");
        print_value(node, nesting_level + 2);
        if (node->left) {
            printf("%*sLeft:\n", nesting_level, "");
            print_node(node->left, nesting_level + 4);
        }
        if (node->right) {
            printf("%*sRight:\n", nesting_level, "");
            print_node(node->right, nesting_level + 4);
        }
    } else {
        printf("%*sNone\n", nesting_level, "");
    }
}

void print_response(Response *res, int32_t nesting_level) {
    if (res) {
        print_response(res->response, nesting_level);
        switch (res->type) {
            case RESPONSE_TYPE__STATUS_R: {
                printf("%*sMessage: %s\n", nesting_level, "", res->status);
                break;
            }
            case RESPONSE_TYPE__NODE_R: {
                Index *index = res->index;
                Node *node = res->node;
                printf("%*s===== NODE =====\n", nesting_level, "");
                printf("%*stype name: %s, type_id: %d\n", nesting_level, "", index->type_name, index->type_id);
                printf("%*sid: %d\n", nesting_level, "", node->id);
                printf("%*sattributes:\n", nesting_level, "");
                for (uint32_t i = 0; i < node->n_attrs; ++i) {
                    printf("%*s%s = ", nesting_level + 2, "", index->description->attrs[i]->name);
                    if (node->attrs[i]->type == VAL_TYPE__INT) printf("%d\n", node->attrs[i]->i);
                    if (node->attrs[i]->type == VAL_TYPE__DOUBLE) printf("%f\n", node->attrs[i]->d);
                    if (node->attrs[i]->type == VAL_TYPE__STRING) printf("%s\n", node->attrs[i]->str->str);
                    if (node->attrs[i]->type == VAL_TYPE__BOOL)
                        printf((node->attrs[i]->b == true) ? "true\n" : "false\n");
                }
                printf("%*slinks out:\n", nesting_level, "");
                for (uint32_t i = 0; i < node->n_links_out; ++i) {
                    printf("%*s%d. link type id = %d, link id = %d\n", nesting_level + 2, "", i + 1,
                           node->links_out[i]->link_type_id,
                           node->links_out[i]->link_id);
                }

                printf("%*slinks in:\n", nesting_level, "");
                for (uint32_t i = 0; i < node->n_links_in; ++i) {
                    printf("%*s%d. link type id = %d, link id = %d\n", nesting_level + 2, "", i + 1,
                           node->links_in[i]->link_type_id,
                           node->links_in[i]->link_id);
                }
                printf("%*s===========\n\n", nesting_level, "");
                break;
            }
            case RESPONSE_TYPE__LINK_R: {
                Link *link = res->link;
                printf("%*s==== LINK =====\n", nesting_level, "");
                printf("%*slink id: %d\n", nesting_level, "", link->link_id);
                printf("%*snode from type id: %d\n", nesting_level, "", link->node_from_type_id);
                printf("%*snode_from_id: %d\n", nesting_level, "", link->node_from_id);
                printf("%*snode_to_type_id: %d\n", nesting_level, "", link->node_to_type_id);
                printf("%*snode_to_id: %d\n", nesting_level, "", link->node_to_id);
                printf("%*s==========\n\n", nesting_level, "");
                break;
            }
            case RESPONSE_TYPE__INDEX_R: {
                Index *index = res->index;
                printf("%*s===== INDEX ====\n", nesting_level, "");
                printf("%*sname: %s\n", nesting_level, "", index->type_name);
                printf("%*skind: %s\n", nesting_level, "", index->kind == ELEMENT_KIND__NODE_I ? "node" : "link");
                printf("%*selements count: %d\n", nesting_level, "", index->count);
                printf("%*stype id: %d\n", nesting_level, "", index->type_id);
                if (index->kind == ELEMENT_KIND__NODE_I) {
                    printf("%*sattributes count: %lu\n", nesting_level, "", index->description->n_attrs);
                    for (uint32_t i = 0; i < index->description->n_attrs; ++i) {
                        printf("%*s%s\n", nesting_level, "", index->description->attrs[i]->name);
                    }
                }
                printf("%*s==============\n\n", nesting_level, "");
                break;
            }
        }
        print_response(res->inner, nesting_level + 8);
    }
}