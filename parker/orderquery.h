#ifndef __ORDER_QUERY_H__
#define __ORDER_QUERY_H__
#include "route.h"

typedef struct _unsettled_order_st{
	char orderdate[64];
	char orderno[36];
	char username[32];
	int  age;
	char cardid[16];//身份证号.
	char telephone[12];
	char workunit[128];
	char email[32];
	char team[64];
	char group[64];
	char teamleader[64];
}unsettled_order_st, *unsettled_order_t;


typedef struct _settled_order_st{
	char orderdate[64];
	char orderno[36];
	char username[32];
	int  age;
	char cardid[16];//身份证号.
	char telephone[12];
	char workunit[128];
	char email[32];
	char team[64];
	char group[64];
	char teamleader[64];
	char operator[32];
	char opertime[32];
	float price;//not used..
}settled_order_st, *settled_order_t;

#define _field_orderno     "orderno"
#define _field_orderdate   "orderdate"
#define _field_username    "username"
#define _field_age         "age"
#define _field_cardid      "cardid"
#define _field_telephone   "telephone"
#define _field_workunit    "workunit"
#define _field_email       "email"
#define _field_team        "team"
#define _field_group       "group"
#define _field_teamleader  "teamleader"
#define _field_operator    "operator"
#define _field_opertime    "opertime"
#define _field_price       "price"

typedef struct _orderquery_st{
	router_st _base;
}orderquery_st, *orderquery_t;

orderquery_t orderquery_new();
void         orderquery_free(orderquery_t query);

#endif