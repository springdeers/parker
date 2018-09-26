#include "statusrslt.h"
#include <stdlib.h>
#include <string.h>
#include <Windows.h>

static void _assure_capacity(scenestatuses_t con)
{
	if(con == NULL) return;

	if(con->statusnum == con->capcity){
		con->status = (scenestatus_t)realloc(con->status,(con->statusnum+con->statusnum/2)*sizeof(scenestatus_st));
		con->capcity = (con->statusnum+con->statusnum/2);
	}
}

scenestatuses_t scenestatuses_new()
{
	scenestatuses_t rtn = NULL;

	while((rtn = (scenestatuses_t)calloc(1,sizeof(scenestatuses_st))) == NULL) Sleep(10);

	rtn->capcity = _init_capacity;
	rtn->status  = (scenestatus_t)calloc(sizeof(scenestatus_st),_init_capacity);	
	rtn->statusnum = 0;

	return rtn;
}

int  scenestatuses_add(scenestatuses_t con,scenestatus_t status)
{
	if(con == NULL || status == NULL) return 0;

	_assure_capacity(con);

	con->status[con->statusnum++] = *status;

	return con->statusnum;
}

void scenestatuses_clear(scenestatuses_t con)
{
	if(con == NULL ) return ;

	con->statusnum = 0;
}

void scenestatuses_free(scenestatuses_t con)
{
	if(con == NULL) return;

	if(con->status) free(con->status);	

	free(con);
}