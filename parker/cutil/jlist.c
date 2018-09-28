#include "jlist.h"
#include <stdlib.h>
#include <windows.h>
#include <assert.h>

jlist_t jlist_new(){
	jlist_t rt = 0;
	pool_t  p;

	p = pool_new();

	while ((rt = pmalloco(p, sizeof(jlist_st))) == 0) Sleep(1);

	rt->pool = p;
	rt->head = pmalloco(p, sizeof(listnode_st));
	rt->tail = rt->head;
	rt->size = 0;
	rt->freelist = 0;

	return rt;
}

void    jlist_free(jlist_t jlist)
{
	if (jlist == 0) return;

	listnode_t item = jlist->head;
	listnode_t to_release = 0;

	while (jlist->pool)
	{
		pool_free(jlist->pool);
		break;
	}

}

jlist_t jlist_push(jlist_t jlist, void*pdata)
{
	if (jlist == 0 || pdata == 0) return jlist;

	listnode_t news = 0;

 	if (jlist->freelist){
 		news = jlist->freelist;
		jlist->freelist = news->next;
		news->next = news->prev = 0; //clear..
 	}else
		news = pmalloco(jlist->pool, sizeof(listnode_st));

	news->data = pdata;
	news->prev = jlist->tail;

	jlist->tail->next = news;

	jlist->tail = news;//更新tail

	jlist->size++;

	return jlist;
}

void*   jlist_pull(jlist_t jlist)
{
	void* rt = 0;

	if (jlist == 0 || jlist->size == 0) return rt;

	listnode_t node2pick = jlist->head->next;
	
	jlist->head->next = node2pick->next;
	if (node2pick->next) 
		node2pick->next->prev = jlist->head;

	jlist->size--;

	rt = node2pick->data;
	
	if (node2pick == jlist->tail) {
		assert(jlist->size == 0);
		jlist->tail = jlist->head; //更新tail
	}

	node2pick->next = jlist->freelist;//cache..
	jlist->freelist = node2pick;

	return rt;
}

int     jlist_remove_first_if(jlist_t jlist, jlistpredict_t predict, void* param)
{
	int removed = 0;

	if (jlist == 0 || predict == 0 || jlist->size == 0) return 0;

	listnode_t item = jlist->head->next;
	while (item != 0){
		if (predict(item->data, param)){
			item->prev->next = item->next;

			if (item->next) 
				item->next->prev = item->prev;
			else  
				jlist->tail = item->prev;

			item->next = jlist->freelist;//cache..
			jlist->freelist = item;

			jlist->size--;

			removed = 1;

			break;
		}
		item = item->next;
	}

	return  removed;
}

void*   jlist_find_first_if(jlist_t jlist, jlistpredict_t predict,void* param)
{
	void* findata = 0;

	if (jlist == 0 || predict == 0 || jlist->size == 0) return 0;

	listnode_t item = jlist->head->next;
	while (item != 0){
		if (predict(item->data, param)){
			findata = item->data;
			break;
		}
		item = item->next;
	}

	return  findata;
}

int     jlist_size(jlist_t jlist)
{
	return jlist->size;
}

void    jlist_walk(jlist_t jlist, jlistwalk_t walk,void* param)
{
	if (jlist == 0 || jlist->size == 0) return ;

	listnode_t item = jlist->head->next;
	while (item != 0){
		walk(item->data, item, param);
		item = item->next;
	}

}

void*   jlist_front(jlist_t jlist)
{
	if (jlist == 0 || jlist->size == 0) return 0;

	return jlist->head->next->data;
}

void*   jlist_back(jlist_t jlist)
{
	if (jlist == 0 || jlist->size == 0) return 0;

	return jlist->tail->data;
}