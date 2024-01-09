#include "../includes/page.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "stdio.h"

page_header_t *read_header(uint32_t fd, uint32_t page_num) {
    lseek(fd, page_num * PAGE_SIZE, SEEK_SET);
    page_header_t *header = malloc(sizeof(page_header_t));
    read(fd, header, sizeof(page_header_t));

    return header;
}

page_t *read_page(uint32_t fd, uint32_t page_num) {
    lseek(fd, page_num * PAGE_SIZE, SEEK_SET);


    page_t *p = alloc_page();
    read(fd, p->header, sizeof(page_header_t));

    p->chunks = malloc(sizeof(chunk_t) * p->header->chunks_count);

    uint32_t size;
    for (uint32_t i = 0; i < p->header->chunks_count; ++i) {
        read(fd, &size, sizeof(uint32_t));
        p->chunks[i].size = size;
        void *data = malloc(p->chunks[i].size);
        read(fd, data, p->chunks[i].size);
        p->chunks[i].data = data;
    }
    return p;
}

void write_page(uint32_t fd, page_t *page) {
    write_header(fd, page->header);
    for (uint32_t i = 0; i < page->header->chunks_count; ++i) {
        write(fd, &page->chunks[i].size, sizeof(uint32_t));
        write(fd, page->chunks[i].data, page->chunks[i].size);
    }
}

void write_header(uint32_t fd, page_header_t *header) {
    lseek(fd, header->this_page * PAGE_SIZE, SEEK_SET);
    write(fd, header, sizeof(page_header_t));
}

void destroy_page(page_t *page) {
    for (uint32_t i = 0; i < page->header->chunks_count; ++i) {
        free((page->chunks[i]).data);
    }
    free(page->header);
    free(page->chunks);
    free(page);
}

page_t *alloc_page(void) {
    page_t *p = malloc(sizeof(page_t));
    p->header = alloc_header();
    p->chunks = 0;
    return p;
}

page_header_t *alloc_header(void) {
    page_header_t *header = malloc(sizeof(page_header_t));
    header->chunks_count = 0;
    header->free_space = PAGE_SIZE - sizeof(page_header_t);
    header->page_type = FREE;
    return header;
}

void free_page(schema_t *schema, uint32_t num) {
    page_header_t *header = read_header(schema->fd, num);

    ++schema->cnt_free;
    if (header->next_page) {
        page_header_t *next_h = read_header(schema->fd, header->next_page);
        next_h->prev_page = header->prev_page;
        write_header(schema->fd, next_h);
        free(next_h);
    }
    if (header->prev_page) {
        page_header_t *prev_h = read_header(schema->fd, header->prev_page);
        prev_h->next_page = header->next_page;
        write_header(schema->fd, prev_h);
        free(prev_h);
    }
    if (num == schema->first_index) {
        schema->first_index = header->next_page;
    }

    header->next_page = schema->free_page;
    header->prev_page = 0;
    header->page_type = FREE;
    header->free_space = PAGE_SIZE - sizeof(page_header_t);
    header->chunks_count = 0;
    write_header(schema->fd, header);
    free(header);

    page_header_t *old_header = read_header(schema->fd, schema->free_page);
    old_header->prev_page = num;
    write_header(schema->fd, old_header);
    free(old_header);

    schema->free_page = num;

    save(schema);
    if (schema->cnt_free >= schema->last_page_num / 2) shrink(schema);
}

page_t *get_free_page(schema_t *schema) {
    page_t *p;
    if (schema->free_page != 0) {
        p = read_page(schema->fd, schema->free_page);
        schema->free_page = p->header->next_page;

        page_header_t *old = read_header(schema->fd, p->header->next_page);
        old->prev_page = 0;
        write_header(schema->fd, old);
        free(old);

        --schema->cnt_free;
    } else {
        p = alloc_page();
        p->header->this_page = ++schema->last_page_num;
    }
    save(schema);
    return p;
}

void add_chunk(schema_t *schema, page_t *p, chunk_t *new_chunk) {
    ++p->header->chunks_count;
    chunk_t *new_chunks = realloc(p->chunks, p->header->chunks_count * sizeof(chunk_t));
    p->chunks = new_chunks;
    p->chunks[p->header->chunks_count - 1] = *new_chunk;
    p->header->free_space -= (new_chunk->size + sizeof(uint32_t));
    write_page(schema->fd, p);
}

void delete_chunk(schema_t *schema, page_t *page, uint32_t i) {
    free(page->chunks[i].data);
    page->header->free_space += page->chunks[i].size + sizeof(uint32_t);
    --page->header->chunks_count;

    chunk_t *tmp = malloc(page->header->chunks_count * sizeof(chunk_t));
    for (uint32_t it = 0; it < i; ++it) {
        tmp[it] = page->chunks[it];
    }
    for (uint32_t it = i; it < page->header->chunks_count; ++it) {
        tmp[it] = page->chunks[it + 1];
    }
    free(page->chunks);
    page->chunks = tmp;
    write_page(schema->fd, page);

    if (page->header->chunks_count == 0) {
        free_page(schema, page->header->this_page);
    }
}

void shrink(schema_t *schema) {
    uint32_t last = schema->last_page_num;
    uint32_t free_num = schema->free_page;

    page_t *last_page;
    page_header_t *free_page_header;

    while (last > 0 && free_num != 0) {
        last_page = read_page(schema->fd, last);
        if (last_page->header->page_type != FREE) {
            free_page_header = read_header(schema->fd, free_num);
            while (free_num > last && free_num != 0) {
                free_num = free_page_header->next_page;
                free(free_page_header);
                free_page_header = read_header(schema->fd, free_num);
            }

            if (free_num == 0) {
                free(free_page_header);
                destroy_page(last_page);
                break;
            }
            free_num = free_page_header->next_page;

            if (last_page->header->page_type == INDEX) {
                if (last_page->header->this_page == schema->first_index) {
                    schema->first_index = free_page_header->this_page;
                }
            }

            if(last_page->header->next_page) {
                page_header_t *next = read_header(schema->fd, last_page->header->next_page);
                next->prev_page = free_page_header->this_page;
                write_header(schema->fd, next);
                free(next);
            }

            if(last_page->header->prev_page) {
                page_header_t *prev = read_header(schema->fd, last_page->header->prev_page);
                prev->next_page = free_page_header->this_page;
                write_header(schema->fd, prev);
                free(prev);
            }

            last_page->header->this_page = free_page_header->this_page;
            free(free_page_header);

            write_page(schema->fd, last_page);
        }
        destroy_page(last_page);
        --last;
    }
    ftruncate(schema->fd, (schema->last_page_num - schema->cnt_free + 1) * PAGE_SIZE);
    schema->last_page_num = schema->last_page_num - schema->cnt_free;
    printf("%d\n", schema->last_page_num);
    schema->cnt_free = 0;
    schema->free_page = 0;
    save(schema);
}

void save(schema_t *schema) {
    page_t *p = alloc_page();
    p->header->this_page = 0;
    p->header->next_page = 0;
    p->header->prev_page = 0;
    p->header->page_type = HEADER;
    p->header->chunks_count = 0;
    chunk_t chunk = {.size = sizeof(schema_t), .data = (char *) schema};
    add_chunk(schema, p, &chunk);
    free(p->header);
    free(p->chunks);
    free(p);
}