#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <min/list.h>

list_node_t *new_list_node(void *value)
{
    list_node_t *node = malloc(sizeof(list_node_t));
    node->value = value;
    node->next = NULL;
    node->prev = NULL;
    node->list = NULL;
    return node;
}

list_t *new_list()
{
    list_t *list = malloc(sizeof(list_t));
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}

list_t *list_clone(list_t *list, void *(*clone)(void *))
{
    list_t *cl = new_list();
    foreach (list, n)
    {
        list_append(cl, clone(n->value));
    }

    return cl;
}

void list_delete_all(list_t *list, void (*delete)(void *))
{
    while (list->head)
    {
        delete (list_take_first(list));
    }
}

list_node_t *list_get_node(list_t *list, void *value)
{
    foreach (list, n)
    {
        if (n->value == value)
            return n;
    }

    return NULL;
}

void list_append_node(list_t *list, list_node_t *node)
{
    node->prev = list->tail;
    node->next = NULL;
    if (!list->tail)
    {
        assert(list->size == 0);
        list->head = node;
    }
    else
    {
        list->tail->next = node;
    }
    list->tail = node;
    node->list = list;
    list->size++;
}

void list_prepend_node(list_t *list, list_node_t *node)
{
    node->next = list->head;
    node->prev = NULL;
    if (!list->head)
    {
        assert(list->size == 0);
        list->tail = node;
    }
    else
    {
        list->head->prev = node;
    }
    list->head = node;
    node->list = list;
    list->size++;
}

list_node_t *list_append(list_t *list, void *value)
{
    list_node_t *node = new_list_node(value);
    list_append_node(list, node);
    return node;
}

list_node_t *list_prepend(list_t *list, void *value)
{
    list_node_t *node = new_list_node(value);
    list_prepend_node(list, node);
    return node;
}

void list_remove_node(list_t *list, list_node_t *node)
{
    assert(node->list == list);
    list_node_t *p = node->prev;
    list_node_t *n = node->next;

    if (p)
        p->next = n;
    if (n)
        n->prev = p;
    if (node == list->head)
        list->head = n;
    if (node == list->tail)
        list->tail = p;
    list->size--;

    node->list = NULL;
    node->prev = NULL;
    node->next = NULL;

    if (!list->size)
    {
        assert(list->head == NULL);
        assert(list->tail == NULL);
    }
}

int list_remove(list_t *list, void *value)
{
    list_node_t *node = list_get_node(list, value);
    if (!node)
        return -1;

    list_remove_node(list, node);
    free(node);
    return 0;
}

list_node_t *list_take_first_node(list_t *list)
{
    list_node_t *node = list->head;
    list_remove_node(list, node);
    return node;
}

list_node_t *list_take_last_node(list_t *list)
{
    list_node_t *node = list->tail;
    list_remove_node(list, node);
    return node;
}

void *list_take_first(list_t *list)
{
    list_node_t *node = list_take_first_node(list);
    if (!node)
        return NULL;
    void *value = node->value;
    free(node);
    return value;
}

void *list_take_last(list_t *list)
{
    list_node_t *node = list_take_last_node(list);
    if (!node)
        return NULL;
    void *value = node->value;
    free(node);
    return value;
}

void list_node_add_before(list_t *list, list_node_t *node, list_node_t *new_node)
{
    assert(node->list == list);
    assert(new_node->list == NULL);
    list_node_t *prev = node->prev;

    // prev | new | node
    if (prev)
        prev->next = new_node;
    new_node->prev = prev;
    new_node->next = node;
    node->prev = new_node;
    if (node == list->head)
        list->head = new_node;
    new_node->list = list;
    list->size++;
}

void list_node_add_after(list_t *list, list_node_t *node, list_node_t *new_node)
{
    assert(node->list == list);
    assert(new_node->list == NULL);

    list_node_t *next = node->next;
    // node | new | next

    node->next = new_node;
    new_node->prev = node;
    new_node->next = next;
    if (next)
        next->prev = new_node;
    if(node==list->tail)
        list->tail = new_node;
    new_node->list = list;
    list->size++;
}

list_node_t *list_add_before(list_t *list, void *value, void *new_value)
{
    list_node_t *node = list_get_node(list, value);
    list_node_t *new = new_list_node(new_value);
    list_node_add_before(list, node, new);
    return new;
}

list_node_t *list_add_after(list_t *list, void *value, void *new_value)
{
    list_node_t *node = list_get_node(list, value);
    list_node_t *new = new_list_node(new_value);
    list_node_add_after(list, node, new);
    return new;
}
