#include <unistd.h>
#include "../includes/query.h"
#include "stdlib.h"
#include "stdio.h"

Response *exec(query_t *q) {
    schema_t *schema = init_db(q->filename);
    Response *res;
    switch (q->q_type) {
        case ADD: {
            switch (q->target) {
                case Q_INDEX: {
                    res = add_index(q->index, schema);
                    break;
                }
                case Q_NODE: {
                    res = add_node(schema, q->index, q->query_body.add.node);
                    break;
                }
                case Q_LINK: {
                    res = add_link(schema, q->index, q->query_body.add.link);
                    break;
                }
            }
            break;
        }
        case SELECT: {
            switch (q->target) {
                case Q_INDEX: {
                    res = index_enumerate(schema);
                    break;
                }
                case Q_NODE: {
                    res = node_enumerate(schema, q->index, q->query_body.sel);
                    break;
                }
                case Q_LINK: {
                    res = link_enumerate(schema, q->index);
                    break;
                }
            }
            break;
        }
        case DELETE: {
            switch (q->target) {
                case Q_INDEX: {
                    res = delete_index_by_name(schema, q->index->type_name);
                    break;
                }
                case Q_NODE: {
                    res = delete_node_by_id(schema, q->index, q->query_body.del.id);
                    break;
                }
                case Q_LINK: {
                    res = delete_link_by_id(schema, q->index, q->query_body.del.id);
                    break;
                }
            }
            break;
        }
        case SET: {
            if (q->target == Q_NODE) {
                res = set_node_attribute(schema, q->index, q->query_body.set.node_id, q->query_body.set.attr_name,
                                         q->query_body.set.new_value);
            }
            break;
        }
    }
    close(schema->fd);
    free(schema);
    return res;
}