#ifndef __CARD_QUERY_H__
#define __CARD_QUERY_H__
#include "route.h"

typedef struct _cardquery_st{
	router_st _base;
}cardquery_st, *cardquery_t;

cardquery_t cardquery_new();
void        cardquery_free(cardquery_t query);

#endif