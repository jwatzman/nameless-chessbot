#include <stdlib.h>
#include "types.h"
#include "movelist.h"

Movelist *movelist_create()
{
	Movelist *list = malloc(sizeof(Movelist));
	list->head = list->tail = 0;
	return list;
}

void movelist_destroy(Movelist *list)
{
	while (movelist_next_move(list))
		;

	free(list);
}

Movelist *movelist_clone(Movelist *list)
{
	Movelist *result = movelist_create();

	Movelistnode *cur = list->head;
	while (cur)
	{
		movelist_append_move(result, cur->move);
		cur = cur->next;
	}

	return result;
}

Move movelist_next_move(Movelist *list)
{
	if (!movelist_is_empty(list))
	{
		Movelistnode *node = list->head;
		Move result = node->move;

		list->head = node->next;
		if (!list->head)
			list->tail = 0;

		free(node);
		return result;
	}
	else
		return 0;
}

void movelist_prepend_move(Movelist *list, Move move)
{
	Movelistnode *node = malloc(sizeof(Movelistnode));

	node->move = move;

	if (!movelist_is_empty(list))
	{
		node->next = list->head;
		list->head = node;
	}
	else
	{
		node->next = 0;
		list->head = list->tail = node;
	}
}

void movelist_append_move(Movelist *list, Move move)
{
	Movelistnode *node = malloc(sizeof(Movelistnode));

	node->move = move;
	node->next = 0;

	if (!movelist_is_empty(list))
	{
		list->tail->next = node;
		list->tail = node;
	}
	else
	{
		list->head = list->tail = node;
	}
}

void movelist_append_movelist(Movelist *list, Movelist *append)
{
	if (!movelist_is_empty(list))
	{
		list->tail->next = append->head;
		list->tail = append->tail;
	}
	else
	{
		list->head = append->head;
		list->tail = append->tail;
	}
}

int movelist_is_empty(Movelist *list)
{
	return (list->head == 0);
}
