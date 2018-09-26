#ifndef __ORDER_QUERY_H__
#define __ORDER_QUERY_H__
#include "route.h"

typedef struct _orderquery_st{
	router_st _base;
}orderquery_st, *orderquery_t;

orderquery_t orderquery_new();
void         orderquery_free(orderquery_t query);

#endif