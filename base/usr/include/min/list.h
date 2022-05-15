#pragma once

#pragma once

struct list;

typedef struct list_node
{
    struct list *list;
    void *value;
    struct list_node *prev;
    struct list_node *next;
} list_node_t;

typedef struct list
{
    list_node_t *head;
    list_node_t *tail;
    int size;
    char *name;
} list_t;

#define foreach(list, node) for (list_node_t *node = list->head; node; node = node->next)
#define foreach_r(list, node) for (list_node_t *node = list->tail; node; node = node->prev)

list_t *new_list();
list_t *list_clone(list_t *list, void *(*clone)(void *));
void list_delete_all(list_t *list, void (*delete)(void *));


list_node_t *new_list_node(void *value);

list_node_t *list_get_node(list_t *list, void *value);

void list_append_node(list_t *list, list_node_t *node);
void list_prepend_node(list_t *list, list_node_t *node);

list_node_t *list_append(list_t *list, void *value);
list_node_t *list_prepend(list_t *list, void *value);

void list_remove_node(list_t *list, list_node_t *node);
int list_remove(list_t *list, void *value);

list_node_t *list_take_first_node(list_t *list);
list_node_t *list_take_last_node(list_t *list);

void *list_take_first(list_t *list);
void *list_take_last(list_t *list);

void list_node_add_before(list_t *list, list_node_t *node, list_node_t *new_node);
void list_node_add_after(list_t *list, list_node_t *node, list_node_t *new_node);

list_node_t *list_add_before(list_t *list, void *value, void *new_value);
list_node_t *list_add_after(list_t *list, void *value, void *new_value);
