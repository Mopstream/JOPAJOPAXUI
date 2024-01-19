#include "../includes/schema.h"
#include "../includes/page.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

schema_t *init_db(char *filename) {

    int fd = open(filename, O_CREAT | O_RDWR, 0644);
    if (fd < 0) return 0;

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return 0;
    }
    uint64_t file_size = st.st_size;

    if (file_size == -1L) {
        close(fd);
        return 0;
    }

    schema_t *schema = malloc(sizeof(schema_t));
    if (!schema) {
        close(fd);
        return 0;
    }
    if (file_size == 0) {
        schema->first_index = 0;
        schema->free_page = 0;
        schema->last_page_num = 0;
        schema->cnt_free = 0;
        schema->fd = fd;
        save(schema);
    } else {
        page_t *p = read_page(fd, 0);
        memcpy(schema, p->chunks[0].data, p->chunks[0].size);
        destroy_page(p);
        schema->fd = fd;

    }
    return schema;

}

chunk_t *encode_index(index_t *index) {
    chunk_t *index_chunk = malloc(sizeof(chunk_t));
    char *data = malloc(index->size);
    index_chunk->size = index->size;
    char *ptr = data;
    memcpy(ptr, &index->size, 4);
    ptr += 4;
    memcpy(ptr, index->type_name, 16);
    ptr += 16;
    memcpy(ptr, &index->kind, sizeof(element_kind_t));
    ptr += sizeof(element_kind_t);
    memcpy(ptr, &index->type_id, 4);
    ptr += 4;
    if (index->kind == I_NODE) {
        memcpy(ptr, &index->description.attr_count, 4);
        ptr += 4;
        memcpy(ptr, index->description.attr_types,
               index->description.attr_count * sizeof(attr_type_t));
        ptr += index->description.attr_count * sizeof(attr_type_t);
    }
    memcpy(ptr, &index->count, 4);
    ptr += 4;
    memcpy(ptr, &index->first_page_num, 4);
    index_chunk->data = data;
    return index_chunk;
}

index_t *decode_index(char *data) {
    char *ptr = data;
    index_t *index = malloc(sizeof(index_t));

    index->size = *((uint32_t *) ptr);
    ptr += 4;
    memcpy(index->type_name, ptr, 16);
    ptr += 16;
    index->kind = *((element_kind_t *) ptr);
    ptr += sizeof(element_kind_t);
    index->type_id = *((uint32_t *) ptr);
    ptr += 4;
    if (index->kind == I_NODE) {
        index->description.attr_count = *((uint32_t *) ptr);
        ptr += 4;
        uint32_t cnt = index->description.attr_count;
        index->description.attr_types = malloc(cnt * sizeof(attr_type_t));
        memcpy(index->description.attr_types, ptr, cnt * sizeof(attr_type_t));
        ptr += cnt * sizeof(attr_type_t);
    }
    index->count = *((uint32_t *) ptr);
    ptr += 4;
    index->first_page_num = *((uint32_t *) ptr);
    ptr += 4;
    return index;
}

chunk_t *encode_node(node_t *node, index_t *index) {
    chunk_t *node_chunk = malloc(sizeof(chunk_t));
    uint32_t size_attrs = 0;
    node_type_t type = index->description;
    for (uint32_t i = 0; i < type.attr_count; ++i) {
        if (type.attr_types[i].type == INT) size_attrs += 4;
        else if (type.attr_types[i].type == DOUBLE) size_attrs += 8;
        else if (type.attr_types[i].type == STRING) size_attrs += 4 + node->attrs[i].str.size;
        else size_attrs += 1;
    }
    char *data = malloc(4 + size_attrs + 4 + node->out_cnt * 8 + 4 + node->in_cnt * 8);
    node_chunk->size = 4 + size_attrs + 4 + node->out_cnt * 8 + 4 + node->in_cnt * 8;
    char *ptr = data;
    memcpy(ptr, &node->id, 4);
    ptr += 4;
    for (uint32_t i = 0; i < type.attr_count; ++i) {
        if (type.attr_types[i].type == INT) {
            memcpy(ptr, &node->attrs[i].i, 4);
            ptr += 4;
        } else if (type.attr_types[i].type == DOUBLE) {
            memcpy(ptr, &node->attrs[i].d, 8);
            ptr += 8;
        } else if (type.attr_types[i].type == STRING) {
            memcpy(ptr, &node->attrs[i].str.size, 4);
            ptr += 4;
            memcpy(ptr, node->attrs[i].str.str, node->attrs[i].str.size);
            ptr += node->attrs[i].str.size;
        } else {
            memcpy(ptr, &node->attrs[i].b, 1);
            ptr += 1;
        }
    }
    memcpy(ptr, &node->out_cnt, 4);
    ptr += 4;
    memcpy(ptr, node->links_out, node->out_cnt * 8);
    ptr += node->out_cnt * 8;
    memcpy(ptr, &node->in_cnt, 4);
    ptr += 4;
    memcpy(ptr, node->links_in, node->in_cnt * 8);
    ptr += node->in_cnt * 4;
    node_chunk->data = data;
    return node_chunk;
}

node_t *decode_node(char *data, index_t *index) {
    char *ptr = data;
    node_t *node = malloc(sizeof(node_t));
    node->id = *((uint32_t *) ptr);
    ptr += 4;
    node->attrs = malloc(index->description.attr_count * sizeof(value_t));
    for (uint32_t i = 0; i < index->description.attr_count; ++i) {
        if (index->description.attr_types[i].type == STRING) {
            string_t s;
            s.size = *((uint32_t *) ptr);
            ptr += 4;
            s.str = malloc(s.size);
            memcpy(s.str, ptr, s.size);
            ptr += s.size;
            node->attrs[i].str = s;
        } else if (index->description.attr_types[i].type == INT) {
            node->attrs[i].i = *((int32_t *) ptr);
            ptr += 4;
        } else if (index->description.attr_types[i].type == DOUBLE) {
            node->attrs[i].d = *((double *) ptr);
            ptr += 8;
        } else {
            node->attrs[i].b = *((bool *) ptr);
            ptr += 1;
        }
    }
    node->out_cnt = *((uint32_t *) ptr);
    ptr += 4;
    node->links_out = malloc(node->out_cnt * 8);
    memcpy(node->links_out, ptr, node->out_cnt * 8);
    ptr += node->out_cnt * 8;
    node->in_cnt = *((uint32_t *) ptr);
    ptr += 4;
    node->links_in = malloc(node->in_cnt * 8);
    memcpy(node->links_in, ptr, node->in_cnt * 8);
    ptr += node->in_cnt * 8;
    return node;
}

void free_node(index_t *index, node_t *node) {
    for (uint32_t i = 0; i < index->description.attr_count; ++i) {
        if (index->description.attr_types[i].type == STRING) {
            free(node->attrs[i].str.str);
        }
    }
    free(node->attrs);
    free(node->links_out);
    free(node->links_in);
    free(node);
}

chunk_t *encode_link(link_t *link) {
    chunk_t *link_chunk = malloc(sizeof(chunk_t));
    link_chunk->data = malloc(sizeof(link_t));
    memcpy(link_chunk->data, link, sizeof(link_t));
    link_chunk->size = sizeof(link_t);
    return link_chunk;
}

link_t *decode_link(char *data) {
    link_t *link = malloc(sizeof(link_t));
    memcpy(link, data, sizeof(link_t));
    return link;
}

void free_link(link_t *link) {
    free(link);
}

struct find_context
find(schema_t *schema, uint32_t first_page, struct find_context (*action)(struct find_context, void *), void *extra) {
    struct find_context a;
    page_t *page = read_page(schema->fd, first_page);
    while (page->header->this_page != 0) {
        for (uint32_t i = 0; i < page->header->chunks_count; ++i) {
            a.i = i;
            a.thing = page->chunks[i].data;
            a.page = page;
            a = action(a, extra);
            if (a.thing != 0) {
                return a;
            }
        }
        page_t *new_page = read_page(schema->fd, page->header->next_page);
        destroy_page(page);
        page = new_page;
    }
    return (struct find_context) {0};
}

struct find_context save_index_action(struct find_context context, void *index) {
    index_t *cur_index = decode_index(context.thing);
    if (strcmp(cur_index->type_name, ((index_t *) index)->type_name) == 0) {
        free_index(cur_index);
        return context;
    }
    free_index(cur_index);
    return (struct find_context) {0};
}

void save_index(schema_t *schema, index_t *index) {
    chunk_t *serialized = encode_index(index);
    struct find_context context = find(schema, schema->first_index, save_index_action, index);

    if (context.page->header->free_space + context.page->chunks[context.i].size - serialized->size >= 0) {
        context.page->header->free_space += context.page->chunks[context.i].size - serialized->size;
        free(context.page->chunks[context.i].data);
        context.page->chunks[context.i] = *serialized;
        write_page(schema->fd, context.page);
    } else {
        delete_chunk(schema, context.page, context.i);
        add_index(index, schema);
    }
    free(serialized);

    destroy_page(context.page);
}

struct find_context index_enumerating_action(struct find_context context, void *extra) {
    Response **res = (Response **) extra;
    index_t *index = decode_index(context.thing);
    Response *new_res = malloc(sizeof(Response));
    response__init(new_res);
    new_res->type = RESPONSE_TYPE__INDEX_R;
    new_res->index = construct_index(index);
    new_res->response = *res;
    *res = new_res;
    free_index(index);
    return (struct find_context) {0};
}

struct find_context node_enumerating_action(struct find_context context, void *extra) {
    struct {
        index_t *index;
        select_q *conditionals;
        Response **res;
    } *cond = extra;
    node_t *node = decode_node(context.thing, cond->index);
    bool flag = true;
    for (uint32_t i = 0; i < cond->conditionals->cond_cnt; ++i) {
        cond_t cur_cond = cond->conditionals->conditionals[i];
        for (uint32_t j = 0; j < cond->index->description.attr_count; ++j) {
            attr_type_t curr_attr_type = cond->index->description.attr_types[j];
            value_t curr_val = node->attrs[j];
            if (strcmp(cur_cond.attr_name, curr_attr_type.type_name) == 0) {
                if (curr_attr_type.type == STRING) {
                    if (cur_cond.cmp == EQUAL && strcmp(curr_val.str.str, cur_cond.val.str.str) != 0)
                        flag = false;
                }
                if (curr_attr_type.type == INT) {
                    switch (cur_cond.cmp) {
                        case GREATER: {
                            if (curr_val.i <= cur_cond.val.i) {
                                flag = false;
                            }
                            break;
                        }
                        case LOWER: {
                            if (curr_val.i >= cur_cond.val.i) {
                                flag = false;
                            }
                            break;
                        }
                        case EQUAL: {
                            if (curr_val.i != cur_cond.val.i) {
                                flag = false;
                            }
                            break;
                        }
                    }
                    break;
                }
                if (curr_attr_type.type == DOUBLE) {
                    switch (cur_cond.cmp) {
                        case GREATER: {
                            if (curr_val.d <= cur_cond.val.d) {
                                flag = false;
                            }
                            break;
                        }
                        case LOWER: {
                            if (curr_val.d >= cur_cond.val.d) {
                                flag = false;
                            }
                            break;
                        }
                        case EQUAL: {
                            if (curr_val.d != cur_cond.val.d) {
                                flag = false;
                            }
                            break;
                        }
                    }
                    break;
                }
                if (curr_attr_type.type == BOOL) {
                    switch (cur_cond.cmp) {
                        case GREATER: {
                            if (curr_val.b <= cur_cond.val.b) {
                                flag = false;
                            }
                            break;
                        }
                        case LOWER: {
                            if (curr_val.b >= cur_cond.val.b) {
                                flag = false;
                            }
                            break;
                        }
                        case EQUAL: {
                            if (curr_val.b != cur_cond.val.b) {
                                flag = false;
                            }
                            break;
                        }
                    }
                    break;
                }
            }
        }
        if (!flag) break;
    }
    if (flag) {
        Response *new_res = malloc(sizeof(Response));
        response__init(new_res);
        new_res->type = RESPONSE_TYPE__NODE_R;
        new_res->index = construct_index(cond->index);
        new_res->node = construct_node(node, cond->index);
        new_res->response = *(cond->res);
        *(cond->res) = new_res;
    }

    free_node(cond->index, node);
    return (struct find_context) {0};
}

struct find_context link_enumerating_action(struct find_context context, void *extra) {
    struct {
        schema_t * schema;
        Response **res;
    } *e = extra;
    Response **res = e->res;
    link_t *link = decode_link(context.thing);
    Response *new_res = malloc(sizeof(Response));
    response__init(new_res);
    new_res->type = RESPONSE_TYPE__LINK_R;
    new_res->link = convert_link(link);
    new_res->response = *res;
    *res = new_res;

    struct find_context ind_context = find_index_by_id(e->schema, link->node_from_type_id);
    destroy_page(ind_context.page);
    index_t *ind = ind_context.thing;

    struct find_context node_context = find_node_by_id(e->schema, ind, link->node_from_id);
    destroy_page(node_context.page);
    node_t *node = node_context.thing;
    Response *from = malloc(sizeof(Response));
    response__init(from);
    from->type = RESPONSE_TYPE__NODE_R;
    from->index = construct_index(ind);
    from->node = construct_node(node, ind);
    free_node(ind, node);
    free_index(ind);


    ind_context = find_index_by_id(e->schema, link->node_to_type_id);
    destroy_page(ind_context.page);
    ind = ind_context.thing;
    node_context = find_node_by_id(e->schema, ind, link->node_to_id);
    destroy_page(node_context.page);
    node = node_context.thing;

    Response *to = malloc(sizeof(Response));
    response__init(to);
    to->type = RESPONSE_TYPE__NODE_R;
    to->index = construct_index(ind);
    to->node = construct_node(node, ind);
    to->response = from;
    new_res->inner = to;
    free_node(ind, node);
    free_index(ind);

    free_link(link);

    return (struct find_context) {0};
}

struct find_context find_index_by_name_action(struct find_context context, void *name) {
    index_t *index = decode_index(context.thing);
    if (strcmp(index->type_name, name) == 0) {
        context.thing = index;
        return context;
    }
    free_index(index);
    return (struct find_context) {0};
}

struct find_context find_index_by_type_id_action(struct find_context context, void *extra) {
    uint32_t *id = extra;
    index_t *index = decode_index(context.thing);
    if (index->type_id == *id) {
        context.thing = index;
        return context;
    }
    free_index(index);
    return (struct find_context) {0};
}

struct find_context find_node_by_id_action(struct find_context context, void *extra) {
    struct {
        index_t *index;
        uint32_t id;
    } *params = extra;

    node_t *node = decode_node(context.thing, params->index);
    if (node->id == params->id) {
        context.thing = node;
        return context;
    }
    free_node(params->index, node);
    return (struct find_context) {0};
}

Response *index_enumerate(schema_t *schema) {
    Response *res = 0;
    find(schema, schema->first_index, index_enumerating_action, &res);
    return res;
}

Response *node_enumerate(schema_t *schema, index_t *index, select_q *conditionals) {
    Response *res = 0;
    struct {
        index_t *index;
        select_q *conditionals;
        Response **res;
    } *extra = malloc(sizeof(struct {
        index_t *index;
        select_q *conditionals;
        Response **res;
    }));
    extra->index = index;
    extra->conditionals = conditionals;
    extra->res = &res;
    find(schema, index->first_page_num, node_enumerating_action, extra);
    return res;
}

Response *link_enumerate(schema_t *schema, index_t *index) {
    Response *res = 0;
    struct {
        schema_t * schema;
        Response **res;
    } *extra = malloc(sizeof(struct {
        schema_t * schema;
        Response **res;
    }));

    extra->res = &res;
    extra->schema = schema;
    find(schema, index->first_page_num, link_enumerating_action, extra);
    return res;
}

Response *delete_node_by_id(schema_t *schema, index_t *index, uint32_t node_id) {
    Response *res = malloc(sizeof(res));
    response__init(res);
    struct {
        index_t *index;
        uint32_t id;
    } *extra = malloc(sizeof(struct {
        index_t *index;
        uint32_t id;
    }));
    extra->id = node_id;
    extra->index = index;

    struct find_context context = find(schema, index->first_page_num, find_node_by_id_action, extra);
    if (context.page->header->chunks_count == 1) {
        if (index->first_page_num == context.page->header->this_page) {
            index->first_page_num = context.page->header->next_page;
        }
    }
    --index->count;
    save_index(schema, index);
    delete_chunk(schema, context.page, context.i);
    free_node(index, context.thing);
    destroy_page(context.page);
    res->type = RESPONSE_TYPE__STATUS_R;
    res->status = "node successfully deleted\n";
    return res;
}

Response *delete_link_by_id(schema_t *schema, index_t *index, uint32_t link_id) {
    Response *res = malloc(sizeof(res));
    response__init(res);
    res->type = RESPONSE_TYPE__STATUS_R;
    res->status = "link successfully deleted\n";
    return res;
}

Response *delete_index_by_name(schema_t *schema, char name[16]) {
    Response *res = malloc(sizeof(Response));
    response__init(res);
    struct find_context context = find(schema, schema->first_index, find_index_by_name_action, name);
    if (((index_t *) context.thing)->count == 0) {
        delete_chunk(schema, context.page, context.i);
    }
    free_index(context.thing);
    destroy_page(context.page);
    res->type = RESPONSE_TYPE__STATUS_R;
    res->status = "index successfully deleted\n";
    return res;
}

Response *add_node(schema_t *schema, index_t *index, node_t *node) {
    Response *res = malloc(sizeof(Response));
    response__init(res);
    page_t *p;
    chunk_t *node_chunk = encode_node(node, index);

    if (index->first_page_num != 0) {
        p = read_page(schema->fd, index->first_page_num);
        if (p->header->free_space < node_chunk->size + sizeof(uint32_t)) {
            page_t *new_p = get_free_page(schema);
            new_p->header->page_type = NODE;
            new_p->header->next_page = p->header->this_page;
            new_p->header->prev_page = 0;

            p->header->prev_page = new_p->header->this_page;
            write_header(schema->fd, p->header);
            destroy_page(p);

            index->first_page_num = new_p->header->this_page;
            p = new_p;
        }
    } else {
        p = get_free_page(schema);
        p->header->page_type = NODE;
        p->header->next_page = 0;
        p->header->prev_page = 0;
        index->first_page_num = p->header->this_page;

    }
    ++index->count;
    save_index(schema, index);

    add_chunk(schema, p, node_chunk);
    destroy_page(p);
    free(node_chunk);
    res->type = RESPONSE_TYPE__STATUS_R;
    res->status = "node successfully added\n";
    return res;
}

struct find_context find_index_by_id(schema_t *schema, uint32_t i_id) {
    return find(schema, schema->first_index, find_index_by_type_id_action, &i_id);
}

struct find_context find_node_by_id(schema_t *schema, index_t *index, uint32_t id) {
    struct {
        index_t *index;
        uint32_t id;
    } *extra = malloc(sizeof(struct {
        index_t *index;
        uint32_t id;
    }));
    extra->id = id;
    extra->index = index;

    struct find_context node_context = find(schema, index->first_page_num, find_node_by_id_action, extra);
    free(extra);
    return node_context;
}

void add_link_to_node(schema_t *schema, uint32_t node_type_id, uint32_t node_id, link_ref_t *link, bool is_from) {
    struct find_context ind_context = find_index_by_id(schema, node_type_id);
    destroy_page(ind_context.page);
    index_t *ind = ind_context.thing;

    struct find_context node_context = find_node_by_id(schema, ind, node_id);

    node_t *node = node_context.thing;

    link_ref_t *new_links;
    if (is_from) {
        ++node->out_cnt;
        new_links = realloc(node->links_out, node->out_cnt * sizeof(link_ref_t));
        new_links[node->out_cnt - 1] = *link;
        node->links_out = new_links;
    } else {
        ++node->in_cnt;
        new_links = realloc(node->links_in, node->in_cnt * sizeof(link_ref_t));
        new_links[node->in_cnt - 1] = *link;
        node->links_in = new_links;
    }

    chunk_t *encoded = encode_node(node, ind);
    if (node_context.page->header->free_space + node_context.page->chunks[node_context.i].size - encoded->size >=
        0) {
        node_context.page->header->free_space += node_context.page->chunks[node_context.i].size - encoded->size;
        free(node_context.page->chunks[node_context.i].data);
        node_context.page->chunks[node_context.i] = *encoded;
        write_page(schema->fd, node_context.page);
    } else {
        delete_chunk(schema, node_context.page, node_context.i);
        add_node(schema, ind, node);
    }

    free(encoded);
    free_node(ind, node);
    free_index(ind);
}

Response *add_link(schema_t *schema, index_t *index, link_t *link) {
    Response *res = malloc(sizeof(Response));
    response__init(res);
    page_t *p;
    chunk_t *link_chunk = encode_link(link);

    if (index->first_page_num != 0) {
        p = read_page(schema->fd, index->first_page_num);
        if (p->header->free_space < link_chunk->size + sizeof(uint32_t)) {
            page_t *new_p = get_free_page(schema);
            new_p->header->page_type = LINK;
            new_p->header->next_page = p->header->this_page;
            new_p->header->prev_page = 0;

            p->header->prev_page = new_p->header->this_page;
            write_header(schema->fd, p->header);
            destroy_page(p);

            index->first_page_num = new_p->header->this_page;
            p = new_p;
        }
    } else {
        p = get_free_page(schema);
        p->header->page_type = LINK;
        p->header->next_page = 0;
        p->header->prev_page = 0;
        index->first_page_num = p->header->this_page;
    }
    link_ref_t *link_ref = malloc(sizeof(link_ref_t));
    link_ref->link_type_id = index->type_id;
    link_ref->link_id = link->link_id;
    add_link_to_node(schema, link->node_from_type_id, link->node_from_id, link_ref, true);
    add_link_to_node(schema, link->node_to_type_id, link->node_to_id, link_ref, false);
    free(link_ref);

    ++index->count;
    save_index(schema, index);

    add_chunk(schema, p, link_chunk);
    destroy_page(p);
    free(link_chunk);
    res->type = RESPONSE_TYPE__STATUS_R;
    res->status = "link successfully added\n";
    return res;
}

Response *
set_node_attribute(schema_t *schema, index_t *index, uint32_t node_id, char attr_name[16], value_t new_value) {
    Response *res = malloc(sizeof(Response));
    response__init(res);
    struct {
        index_t *index;
        uint32_t id;
    } *extra = malloc(sizeof(struct {
        index_t *index;
        uint32_t id;
    }));
    extra->id = node_id;
    extra->index = index;

    struct find_context context = find(schema, index->first_page_num, find_node_by_id_action, extra);
    free(extra);

    for (uint32_t i = 0; i < index->description.attr_count; ++i) {
        if (strcmp(index->description.attr_types[i].type_name, attr_name) == 0) {
            ((node_t *) context.thing)->attrs[i] = new_value;
            break;
        }
    }

    chunk_t *chunk = encode_node(context.thing, index);

    if (context.page->header->free_space + context.page->chunks[context.i].size - chunk->size >= 0) {
        context.page->header->free_space += context.page->chunks[context.i].size - chunk->size;
        free(context.page->chunks[context.i].data);
        context.page->chunks[context.i] = *chunk;
        write_page(schema->fd, context.page);
    } else {
        delete_chunk(schema, context.page, context.i);
        add_node(schema, index, context.thing);
    }

    free(chunk);
    free_node(index, context.thing);
    destroy_page(context.page);
    res->type = RESPONSE_TYPE__STATUS_R;
    res->status = "node successfully changed\n";
    return res;
}

void free_index(index_t *index) {
    if (index->kind == NODE) {
        free(index->description.attr_types);
    }
    free(index);
}

Response *add_index(index_t *index, schema_t *schema) {
    Response *res = malloc(sizeof(Response));
    response__init(res);
    page_t *p;
    struct find_context name_context = find(schema, schema->first_index, find_index_by_name_action,
                                            index->type_name);
    if (name_context.thing != 0) {
        destroy_page(name_context.page);
        free_index(name_context.thing);
        res->type = RESPONSE_TYPE__STATUS_R;
        res->status = "Error. Such an index already exists.\n";
        return res;
    }
    chunk_t *index_chunk = encode_index(index);
    if (schema->first_index != 0) {
        p = read_page(schema->fd, schema->first_index);
        if (p->header->free_space < index_chunk->size + sizeof(uint32_t)) {
            page_t *new_p = get_free_page(schema);
            new_p->header->page_type = INDEX;
            new_p->header->next_page = p->header->this_page;
            new_p->header->prev_page = 0;

            p->header->prev_page = new_p->header->this_page;
            write_header(schema->fd, p->header);
            destroy_page(p);

            schema->first_index = new_p->header->this_page;
            p = new_p;
        }
    } else {
        p = get_free_page(schema);
        p->header->page_type = INDEX;
        p->header->next_page = 0;
        p->header->prev_page = 0;

        schema->first_index = p->header->this_page;
    }
    add_chunk(schema, p, index_chunk);
    destroy_page(p);
    free(index_chunk);
    save(schema);
    res->type = RESPONSE_TYPE__STATUS_R;
    res->status = "Index successfully added\n";
    return res;
}

index_t *get_first_index(schema_t *schema, char name[16]) {
    struct find_context context = find(schema, schema->first_index, find_index_by_name_action, name);
    destroy_page(context.page);
    return context.thing;
}

index_t *create_index(char name[16], attr_type_t *attrs, uint32_t cnt, element_kind_t kind) {
    index_t *index = malloc(sizeof(index_t));
    memcpy(index->type_name, name, 16);
    index->kind = kind;
    index->type_id = clock();
    index->description.attr_count = cnt;
    index->description.attr_types = attrs;
    index->size = 4 + 16 + sizeof(element_kind_t) + 4 + 4 + cnt * sizeof(attr_type_t) + 4 + 4;
    index->count = 0;
    index->first_page_num = 0;
    return index;
}

node_t *create_node(value_t *values, uint32_t cnt) {
    node_t *node = malloc(sizeof(node_t));
    node->id = clock();
    node->attrs = values;
    node->in_cnt = 0;
    node->out_cnt = 0;
    node->links_in = 0;
    node->links_out = 0;
    return node;
}

link_t *create_link(uint32_t node_from_id, uint32_t node_from_type_id, uint32_t node_to_id, uint32_t node_to_type_id) {
    link_t *link = malloc(sizeof(link_t));
    link->link_id = clock();
    link->node_to_type_id = node_to_type_id;
    link->node_to_id = node_to_id;
    link->node_from_type_id = node_from_type_id;
    link->node_from_id = node_from_id;
    return link;
}

Index *construct_index(index_t *index) {
    Index *i = malloc(sizeof(Index));
    index__init(i);
    i->type_name = malloc(16);
    memcpy(i->type_name, index->type_name, 16);
    i->kind = (index->kind == I_NODE) ? ELEMENT_KIND__NODE_I : ELEMENT_KIND__LINK_I;
    i->type_id = index->type_id;
    i->description = convert_node_description(index->description);
    return i;
}

NodeType *convert_node_description(node_type_t type) {
    NodeType *n_type = malloc(sizeof(NodeType));
    node_type__init(n_type);
    n_type->n_attrs = type.attr_count;
    AttrType **attrs = malloc(type.attr_count * sizeof(AttrType *));
    n_type->attrs = attrs;
    for (uint32_t i = 0; i < type.attr_count; ++i) {
        attrs[i] = convert_attr_type(type.attr_types[i]);
    }
    return n_type;
}

AttrType *convert_attr_type(attr_type_t type) {
    AttrType *a = malloc(sizeof(AttrType));
    attr_type__init(a);
    ValType d[] = {
            [INT] = VAL_TYPE__INT,
            [DOUBLE] = VAL_TYPE__DOUBLE,
            [STRING] = VAL_TYPE__STRING,
            [BOOL] = VAL_TYPE__BOOL,
    };
    a->val = d[type.type];
    a->name = malloc(16);
    memcpy(a->name, type.type_name, 16);
    return a;
}

Node *construct_node(node_t *node, index_t *index) {
    Node *n = malloc(sizeof(Node));
    node__init(n);
    n->id = node->id;
    n->n_attrs = index->description.attr_count;
    Attr **attrs = malloc(n->n_attrs * sizeof(Attr *));
    n->attrs = attrs;
    ValType d[] = {
            [INT] = VAL_TYPE__INT,
            [DOUBLE] = VAL_TYPE__DOUBLE,
            [STRING] = VAL_TYPE__STRING,
            [BOOL] = VAL_TYPE__BOOL,
    };
    for (uint32_t i = 0; i < n->n_attrs; ++i) {
        Attr *a = malloc(sizeof(Attr));
        attr__init(a);
        a->type = d[index->description.attr_types[i].type];
        switch (a->type) {
            case VAL_TYPE__INT: {
                a->i = node->attrs[i].i;
                break;
            }
            case VAL_TYPE__DOUBLE: {
                a->d = node->attrs[i].d;
                break;
            }
            case VAL_TYPE__STRING: {
                a->str = malloc(sizeof(String));
                string__init(a->str);
                a->str->size = node->attrs[i].str.size;
                a->str->str = malloc(a->str->size);
                memcpy(a->str->str, node->attrs[i].str.str, a->str->size);
                break;
            }
            case VAL_TYPE__BOOL: {
                a->b = node->attrs[i].b;
                break;
            }
        }
        attrs[i] = a;
    }
    n->n_links_in = node->in_cnt;
    n->n_links_out = node->out_cnt;
    n->links_in = malloc(node->in_cnt * sizeof(LinkRef *));
    n->links_out = malloc(node->out_cnt * sizeof(LinkRef *));
    for (uint32_t i = 0; i < node->in_cnt; ++i) {
        n->links_in[i] = convert_link_ref(node->links_in[i]);
    }
    for (uint32_t i = 0; i < node->out_cnt; ++i) {
        n->links_out[i] = convert_link_ref(node->links_out[i]);
    }
    return n;
}

LinkRef *convert_link_ref(link_ref_t link) {
    LinkRef *l = malloc(sizeof(LinkRef));
    link_ref__init(l);
    l->link_id = link.link_id;
    l->link_type_id = link.link_type_id;
    return l;
}

Link *convert_link(link_t *link) {
    Link *l = malloc(sizeof(Link));
    link__init(l);
    l->link_id = link->link_id;
    l->node_to_type_id = link->node_to_type_id;
    l->node_to_id = link->node_to_id;
    l->node_from_type_id = link->node_from_type_id;
    l->node_from_id = link->node_from_id;
    return l;
}
