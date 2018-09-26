#ifndef __USER_QUERY_H__
#define __USER_QUERY_H__
#include "route.h"

typedef struct _userquery_st{
	router_st _base;
}userquery_st, *userquery_t;

userquery_t userquery_new();
void        userquery_free(userquery_t query);

#endif