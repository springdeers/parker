#ifndef _JLIST_H___
#define _JLIST_H___

#include "pool.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct _listnode_st{
	struct _listnode_st* next;
	struct _listnode_st* prev;
	void * data;
}listnode_st, *listnode_t;

typedef struct _jlist_st{
	listnode_t head;
	listnode_t tail;
	listnode_t freelist;
	int        size;
	pool_t     pool;  
}jlist_st,*jlist_t;


typedef int(*jlistpredict_t)(void* pdata,void* param);
typedef int(*jlistwalk_t)   (void* pdata, listnode_t,void* param);

jlist_t jlist_new();
void    jlist_free(jlist_t);

jlist_t jlist_push(jlist_t jlist, void*pdata);

void*   jlist_pull(jlist_t jlist);

int     jlist_remove_first_if(jlist_t jlist, jlistpredict_t predict, void* param);

int     jlist_size(jlist_t jlist);

void*   jlist_find_first_if(jlist_t hlist, jlistpredict_t predict, void* param);

void    jlist_walk(jlist_t jlist, jlistwalk_t,void* param);

void*   jlist_front(jlist_t jlist);

void*   jlist_back(jlist_t jlist);

#ifdef __cplusplus
}
#endif

#endif