#include "converter.h"
#include "types.h"
#include <stdlib.h>
#include <stdio.h>

enum _AstNodeType node_types[] = {
        [FILENAME_N] = AST_NODE_TYPE__FILENAME_N,
        [INSERT_N] = AST_NODE_TYPE__INSERT_N,
        [SELECT_N] = AST_NODE_TYPE__SELECT_N,
        [UPDATE_N] = AST_NODE_TYPE__UPDATE_N,
        [DELETE_N] = AST_NODE_TYPE__DELETE_N,
        [INDEX_N] = AST_NODE_TYPE__INDEX_N,
        [NODE_N] = AST_NODE_TYPE__NODE_N,
        [LINK_N] = AST_NODE_TYPE__LINK_N,
        [CONDITIONAL_N] = AST_NODE_TYPE__CONDITIONAL_N,
        [FILTER_N] = AST_NODE_TYPE__FILTER_N,
        [SET_N] = AST_NODE_TYPE__SET_N,
        [VAL_N] = AST_NODE_TYPE__VAL_N,
        [NAME_N] = AST_NODE_TYPE__NAME_N,
        [LIST_N] = AST_NODE_TYPE__LIST_N,
};

enum _InsertTarget insert_targets[] = {
        [I_NODE] = INSERT_TARGET__I_NODE,
        [I_LINK] = INSERT_TARGET__I_LINK,
        [I_INDEX] = INSERT_TARGET__I_INDEX,
};

enum _LogicalOp logical_ops[] = {
        [AND] = LOGICAL_OP__AND,
        [OR] = LOGICAL_OP__OR,
        [NOT] = LOGICAL_OP__NOT,
};

enum _Cmp cmps[] = {
        [GT] = CMP__GT,
        [LT] = CMP__LT,
        [EQ] = CMP__EQ,
        [GT_EQ] = CMP__GT_EQ,
        [LT_EQ] = CMP__LT_EQ,
        [SUBSTR] = CMP__SUBSTR,
};

Attr *convert_attr(attr_t attr) {
    Attr *a = malloc(sizeof(Attr));
    attr__init(a);
    switch (attr.type) {
        case INT: {
            a->type = VAL_TYPE__INT;
            a->i = attr.val.i;
            break;
        }
        case DOUBLE: {
            a->type = VAL_TYPE__DOUBLE;
            a->d = attr.val.d;
            break;
        }
        case STRING: {
            a->type = VAL_TYPE__STRING;
            a->str = malloc(sizeof(String));
            string__init(a->str);
            a->str->size = attr.val.str.size;
            a->str->str = attr.val.str.str;
            break;
        }
        case BOOL: {
            a->type = VAL_TYPE__BOOL;
            a->b = attr.val.b;
            break;
        }
    }
    return a;
}

Ast *convert(ast_node *node) {
    Ast *ast = malloc(sizeof(Ast));
    ast__init(ast);
    if (node->left) ast->left = convert(node->left);
    if (node->right) ast->right = convert(node->right);
    ast->type = node_types[node->type];

    Value *v = malloc(sizeof(Value));
    value__init(v);
    ast->val = v;
    switch (node->v_type) {
        case NONE:{
            ast->v_type = VALUE_TYPE__NONE;
            break;
        }
        case NAME: {
            ast->v_type = VALUE_TYPE__NAME;
            v->name = node->value;
            break;
        }
        case INSERT_TARGET: {
            ast->v_type = VALUE_TYPE__INSERT_TARGET;
            v->target = insert_targets[*(insert_target_t *) node->value];
            break;
        }
        case SET: {
            ast->v_type = VALUE_TYPE__SET;
            set_t *set = (set_t *) node->value;
            Set *s = malloc(sizeof(Set));
            v->set = s;
            set__init(s);
            s->node_id = set->node_id;
            s->attr_name = set->attr_name;
            s->new_value = convert_attr(set->new_value);
            break;
        }
        case ATTR: {
            ast->v_type = VALUE_TYPE__ATTR;
            v->attr = convert_attr(*(attr_t *) node->value);
            break;
        }
        case LOGICAL_OP: {
            ast->v_type = VALUE_TYPE__LOGICAL_OP;
            v->l_op = logical_ops[*(logical_op *) node->value];
            break;
        }
        case CMP: {
            ast->v_type = VALUE_TYPE__CMP;
            v->cmp = cmps[*(cmp_t *) node->value];
            break;
        }
        case LINK: {
            ast->v_type = VALUE_TYPE__LINK;
            Link *l = malloc(sizeof(Link));
            v->link = l;
            link__init(l);
            link_t *link = node->value;
            l->link_id = link->link_id;
            l->node_from_type_id = link->node_from_type_id;
            l->node_from_id = link->node_from_id;
            l->node_to_type_id = link->node_to_type_id;
            l->node_to_id = link->node_to_id;
            break;
        }
    }
    return ast;
}