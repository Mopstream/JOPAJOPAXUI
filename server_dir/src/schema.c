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
    printf("schema.c 7\n");
    printf("JOPAJOPAJOPA\n");
    chunk_t *index_chunk = malloc(sizeof(chunk_t));
    printf("%d\n", index);
    printf("AAAAAAAAAAAAAA\n");
    printf("%d\n", index->type);
    printf("%d\n", index->type.size);
    printf("schema.c 7\n");
    printf("%d\n", index->type.size);
    char *data = malloc(index->type.size + 4 + 4);
    printf("schema.c 7\n");
    index_chunk->size = index->type.size + 4 + 4;
    printf("schema.c 7\n");
    char *ptr = data;
    printf("schema.c 7\n");
    memcpy(ptr, &index->type.size, 4);
    printf("schema.c 7\n");
    ptr += 4;
    memcpy(ptr, index->type.type_name, 16);
    printf("schema.c 7\n");
    ptr += 16;
    memcpy(ptr, &index->type.kind, sizeof(element_kind_t));
    printf("schema.c 7\n");
    ptr += sizeof(element_kind_t);
    printf("schema.c 7\n");
    if (index->type.kind == I_NODE) {
        printf("schema.c 73\n");
        memcpy(ptr, &index->type.description.node.type_id, 4);
        ptr += 4;
        memcpy(ptr, &index->type.description.node.attr_count, 4);
        ptr += 4;
        memcpy(ptr, index->type.description.node.attr_types,
               index->type.description.node.attr_count * sizeof(attr_type_t));
        ptr += index->type.description.node.attr_count * sizeof(attr_type_t);
    } else {
        printf("schema.c 82\n");
        memcpy(ptr, &index->type.description.link.type_id, 4);
        ptr += 4;
        memcpy(ptr, index->type.description.link.type_name, 16);
        ptr += 16;
    }
    printf("schema.c 88\n");
    memcpy(ptr, &index->count, 4);
    ptr += 4;
    printf("schema.c 7\n");
    memcpy(ptr, &index->first_page_num, 4);
    printf("schema.c 7\n");
    index_chunk->data = data;
    printf("schema.c 7\n");
    return index_chunk;
}

index_t *decode_index(char *data) {
    char *ptr = data;
    index_t *index = malloc(sizeof(index_t));

    index->type.size = *((uint32_t *) ptr);
    ptr += 4;
    memcpy(index->type.type_name, ptr, 16);
    ptr += 16;
    index->type.kind = *((element_kind_t *) ptr);
    ptr += sizeof(element_kind_t);
    if (index->type.kind == I_LINK) {
        memcpy(&index->type.description.link, ptr, sizeof(link_type_t));
        ptr += sizeof(link_type_t);
    } else {
        memcpy(&index->type.description.node, ptr, 2 * sizeof(uint32_t));
        ptr += 2 * sizeof(uint32_t);
        uint32_t cnt = index->type.description.node.attr_count;
        index->type.description.node.attr_types = malloc(cnt * sizeof(attr_type_t));
        memcpy(index->type.description.node.attr_types, ptr, cnt * sizeof(attr_type_t));
        ptr += cnt * sizeof(attr_type_t);
    }

    index->count = *((uint32_t *) ptr);
    ptr += 4;
    index->first_page_num = *((uint32_t *) ptr);

    return index;
}

chunk_t *encode_node(node_t *node, index_t *index) {
    chunk_t *node_chunk = malloc(sizeof(chunk_t));
    uint32_t size_attrs = 0;
    node_type_t type = index->type.description.node;
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
    node->attrs = malloc(index->type.description.node.attr_count * sizeof(value_t));
    for (uint32_t i = 0; i < index->type.description.node.attr_count; ++i) {
        if (index->type.description.node.attr_types[i].type == STRING) {
            string_t s;
            s.size = *((uint32_t *) ptr);
            ptr += 4;
            s.str = malloc(s.size);
            memcpy(s.str, ptr, s.size);
            ptr += s.size;
            node->attrs[i].str = s;
        } else if (index->type.description.node.attr_types[i].type == INT) {
            node->attrs[i].i = *((int32_t *) ptr);
            ptr += 4;
        } else if (index->type.description.node.attr_types[i].type == DOUBLE) {
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
    for (uint32_t i = 0; i < index->type.description.node.attr_count; ++i) {
        if (index->type.description.node.attr_types[i].type == STRING) {
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
    if (strcmp(cur_index->type.type_name, ((index_t *) index)->type.type_name) == 0) {
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
    index_t *index = decode_index(context.thing);
    printf("===== INDEX ====\n");
    printf("name: %s\n", index->type.type_name);
    printf("kind: %s\n", index->type.kind == I_NODE ? "node" : "link");
    printf("elements count: %d\n", index->count);
    printf("type id: %d\n", index->type.description.node.type_id);
    if (index->type.kind == I_NODE) {
        printf("attributes count: %d\n", index->type.description.node.attr_count);
        for (uint32_t i = 0; i < index->type.description.node.attr_count; ++i) {
            printf("%s\n", index->type.description.node.attr_types[i].type_name);
        }
    } else {
        printf("type name: %s\n", index->type.description.link.type_name);
    }
    printf("==============\n\n");
    free_index(index);
    return (struct find_context) {0};
}

void print_node(node_t *node, index_t *index) {
    printf("===== NODE =====\n");
    printf("name: %s\n", index->type.type_name);
    printf("id: %d\n", node->id);
    printf("attributes:\n");
    for (uint32_t i = 0; i < index->type.description.node.attr_count; ++i) {
        if (index->type.description.node.attr_types[i].type == INT)
            printf("%s = %d\n", index->type.description.node.attr_types[i].type_name, node->attrs[i].i);
        if (index->type.description.node.attr_types[i].type == DOUBLE)
            printf("%s = %f\n", index->type.description.node.attr_types[i].type_name, node->attrs[i].d);
        if (index->type.description.node.attr_types[i].type == STRING)
            printf("%s = %s\n", index->type.description.node.attr_types[i].type_name, node->attrs[i].str.str);
        if (index->type.description.node.attr_types[i].type == BOOL)
            printf("%s = %d\n", index->type.description.node.attr_types[i].type_name, node->attrs[i].b);
    }
    printf("\nlinks out:\n");
    for (uint32_t i = 0; i < node->out_cnt; ++i) {
        printf("%d. link type id = %d, link id = %d\n", i + 1, node->links_out[i].link_type_id,
               node->links_out[i].link_id);
    }

    printf("\nlinks in:\n");
    for (uint32_t i = 0; i < node->in_cnt; ++i) {
        printf("%d. link type id = %d, link id = %d\n", i + 1, node->links_in[i].link_type_id,
               node->links_in[i].link_id);
    }
    printf("===========\n\n");
}

void print_link(link_t *link) {
    printf("==== LINK =====\n");
    printf("link id: %d\n", link->link_id);
    printf("node from type id: %d\n", link->node_from_type_id);
    printf("node_from_id: %d\n", link->node_from_id);
    printf("node_to_type_id: %d\n", link->node_to_type_id);
    printf("node_to_id: %d\n", link->node_to_id);
    printf("==========\n\n");
}

struct find_context node_enumerating_action(struct find_context context, void *extra) {
    struct {
        index_t *index;
        select_q *conditionals;
    } *cond = extra;
    node_t *node = decode_node(context.thing, cond->index);
    bool flag = true;
    for (uint32_t i = 0; i < cond->conditionals->cond_cnt; ++i) {
        cond_t cur_cond = cond->conditionals->conditionals[i];
        for (uint32_t j = 0; j < cond->index->type.description.node.attr_count; ++j) {
            attr_type_t curr_attr_type = cond->index->type.description.node.attr_types[j];
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
    if (flag) print_node(node, cond->index);

    free_node(cond->index, node);
    return (struct find_context) {0};
}

struct find_context link_enumerating_action(struct find_context context, void *extra) {
    link_t *link = decode_link(context.thing);

    print_link(link);

    free_link(link);
    return (struct find_context) {0};
}

struct find_context find_index_by_name_action(struct find_context context, void *name) {
    index_t *index = decode_index(context.thing);
    if (strcmp(index->type.type_name, name) == 0) {
        context.thing = index;
        return context;
    }
    free_index(index);
    return (struct find_context) {0};
}

struct find_context find_index_by_type_id_action(struct find_context context, void *extra) {
    struct {
        uint32_t id;
        bool is_node;
    } *params = extra;
    index_t *index = decode_index(context.thing);
    if (((index->type.kind == I_NODE) == (params->is_node)) && index->type.description.node.type_id == params->id) {
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

void index_enumerate(schema_t *schema) {
    find(schema, schema->first_index, index_enumerating_action, 0);
}

void node_enumerate(schema_t *schema, index_t *index, select_q *conditionals) {
    struct {
        index_t *index;
        select_q *conditionals;
    } *extra = malloc(sizeof(struct {
        index_t *index;
        select_q *conditionals;
    }));
    find(schema, index->first_page_num, node_enumerating_action, extra);
}

void link_enumerate(schema_t *schema, index_t *index) {
    find(schema, index->first_page_num, link_enumerating_action, 0);
}

void delete_node_by_id(schema_t *schema, index_t *index, uint32_t node_id) {
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
}

void delete_link_by_id(schema_t *schema, index_t *index, uint32_t link_id) {

}

void delete_index_by_name(schema_t *schema, char name[16]) {
    struct find_context context = find(schema, schema->first_index, find_index_by_name_action, name);
    if (((index_t *) context.thing)->count == 0) {
        delete_chunk(schema, context.page, context.i);
    }
    free_index(context.thing);
    destroy_page(context.page);
}

void add_node(schema_t *schema, index_t *index, node_t *node) {
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
}

void add_link_to_node(schema_t *schema, uint32_t node_type_id, uint32_t node_id, link_ref_t *link, bool is_from) {
    struct {
        uint32_t id;
        bool is_node;
    } *extra = malloc(sizeof(struct {
        uint32_t id;
        bool is_node;
    }));
    extra->id = node_type_id;
    extra->is_node = true;
    struct find_context ind_context = find(schema, schema->first_index, find_index_by_type_id_action, extra);
    free(extra);
    destroy_page(ind_context.page);

    struct {
        index_t *index;
        uint32_t id;
    } *extra1 = malloc(sizeof(struct {
        index_t *index;
        uint32_t id;
    }));
    extra1->id = node_id;
    extra1->index = ind_context.thing;

    struct find_context node_context = find(schema, ((index_t *) ind_context.thing)->first_page_num,
                                            find_node_by_id_action, extra1);
    free(extra1);

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

    chunk_t *encoded = encode_node(node, ind_context.thing);
    if (node_context.page->header->free_space + node_context.page->chunks[node_context.i].size - encoded->size >=
        0) {
        node_context.page->header->free_space += node_context.page->chunks[node_context.i].size - encoded->size;
        free(node_context.page->chunks[node_context.i].data);
        node_context.page->chunks[node_context.i] = *encoded;
        write_page(schema->fd, node_context.page);
    } else {
        delete_chunk(schema, node_context.page, node_context.i);
        add_node(schema, ind_context.thing, node);
    }

    free(encoded);
    free_node(ind_context.thing, node);
    free_index(ind_context.thing);
}

void add_link(schema_t *schema, index_t *index, link_t *link) {
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
    link_ref->link_type_id = index->type.description.link.type_id;
    link_ref->link_id = link->link_id;
    add_link_to_node(schema, link->node_from_type_id, link->node_from_id, link_ref, true);
    add_link_to_node(schema, link->node_to_type_id, link->node_to_id, link_ref, false);
    free(link_ref);

    ++index->count;
    save_index(schema, index);

    add_chunk(schema, p, link_chunk);
    destroy_page(p);
    free(link_chunk);
}

void set_node_attribute(schema_t *schema, index_t *index, uint32_t node_id, char attr_name[16], value_t new_value) {
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

    for (uint32_t i = 0; i < index->type.description.node.attr_count; ++i) {
        if (strcmp(index->type.description.node.attr_types[i].type_name, attr_name) == 0) {
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

}

void free_index(index_t *index) {
    if (index->type.kind == NODE) {
        free(index->type.description.node.attr_types);
    }
    free(index);
}

void add_index(index_t *index, schema_t *schema) {
    page_t *p;
    printf("schema: %d\n", schema);
    printf("schema.c 719\n");
    struct find_context name_context = find(schema, schema->first_index, find_index_by_name_action,
                                            index->type.type_name);
    if (name_context.thing != 0) {
        destroy_page(name_context.page);
        free_index(name_context.thing);
        return;
    }
    printf("schema.c 727\n");
    printf("AAA: %d\n", index);
    chunk_t *index_chunk = encode_index(index);
    printf("schema.c 729\n");
    printf("schema: %d\n", schema);
    printf("%d\n", schema->first_index);
    if (schema->first_index != 0) {
        printf("schema.c 731\n");
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
        printf("schema.c 747\n");
        p = get_free_page(schema);
        printf("schema.c 750\n");
        p->header->page_type = INDEX;
        p->header->next_page = 0;
        p->header->prev_page = 0;

        schema->first_index = p->header->this_page;
    }
    printf("schema.c 756\n");
    add_chunk(schema, p, index_chunk);
    printf("schema.c 758\n");
    destroy_page(p);
    printf("schema.c 760\n");
    free(index_chunk);
    printf("schema.c 762\n");
    save(schema);
}

index_t *get_first_index(schema_t *schema, char name[16]) {
    struct find_context context = find(schema, schema->first_index, find_index_by_name_action, name);
    destroy_page(context.page);
    return context.thing;
}

index_t *create_index(char name[16], attr_type_t *attrs, uint32_t cnt) {
    printf("name: ");
    for (uint32_t i = 0; i < 16; ++i){
        printf("%c", name[i]);
    }
    printf("\ncnt = %d\n", cnt);
    for(uint32_t i = 0; i < cnt; ++i){
        printf("%d\ntypename: ", i);
        for (uint32_t j = 0; j < 16; ++j){
            printf("%c", attrs[i].type_name[j]);
        }
        printf("\ntype = %d\n\n", attrs[i].type);
    }


    index_t *index = malloc(sizeof(index_t));
    element_type_t el_type;
    memcpy(el_type.type_name, name, 16);
    el_type.kind = I_NODE;
    el_type.description.node.type_id = clock();
    el_type.description.node.attr_count = cnt;
    el_type.description.node.attr_types = attrs;
    el_type.size = 28 + sizeof(element_kind_t) + cnt * sizeof(attr_type_t);
    index->type = el_type;
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
