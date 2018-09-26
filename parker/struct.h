#ifndef __STRUCT_H__
#define __STRUCT_H__

#define _abv_age40 1
#define _abv_age30 2
#define _abv_age20 3
#define _abv_age15 4
#define _abv_age10 5
#define _abv_others 6

typedef struct _scoredeploy_st{
	int sceneid;
	char buffer[1024];
}scoredeploy_st;

typedef struct _agedeploy_st{
	int sceneid;
	char buffer[1024];	
}agedeploy_st;

#endif