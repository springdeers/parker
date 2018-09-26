#include "scenerslt.h"
#include "stdlib.h"
#include <string.h>
#include <Windows.h>

sceneinfo_t sceneinfo_new()
{
	sceneinfo_t rtn = NULL;

	while((rtn = (sceneinfo_t)calloc(1,sizeof(sceneinfo_st))) == NULL) Sleep(10);

	return rtn;
}

void  sceneinfo_free(sceneinfo_t info)
{
	if(info){
		free(info->scenename);
		free(info->templ);
		free(info);
	}
}

sceneinfo_t  sceneinfo_assign(sceneinfo_t info,int sceneid,char* name,char* templ)
{
	if(info)
	{
		info->sceneid = sceneid;

		if(name)
			info->scenename = _strdup(name);
		else
			info->scenename = NULL;

		if(templ)
			info->templ = _strdup(templ);
		else
			info->templ = NULL;
	}

	return info;
}

static void _assure_capacity(sceneinfos_t con)
{
	if(con == NULL) return;

	if(con->num == con->capcity){
		con->infos = (sceneinfo_t*)realloc(con->infos,(con->num+con->num/2)*sizeof(sceneinfo_t));
		con->capcity = (con->num+con->num/2);
	}
}

sceneinfos_t sceneinfos_new()
{
	sceneinfos_t rtn = NULL;

	while((rtn = (sceneinfos_t)calloc(1,sizeof(sceneinfos_st))) == NULL) Sleep(10);

	rtn->capcity = _init_scenes_capacity;
	rtn->infos  = (sceneinfo_t*)calloc(sizeof(sceneinfo_t),_init_scenes_capacity);	
	rtn->num = 0;

	return rtn;
}

int  sceneinfos_add(sceneinfos_t con,sceneinfo_t info)
{
	if(con == NULL || info == NULL) return 0;

	_assure_capacity(con);

	con->infos[con->num++] = info;

	return con->num;
}

void sceneinfos_clear(sceneinfos_t con)
{
	if(con == NULL ) return ;

	con->num = 0;
}

void sceneinfos_free(sceneinfos_t con)
{
	if(con == NULL) return;

	if(con->infos){
		int i = 0;
		for(;i<con->num;i++)
			sceneinfo_free(con->infos[i]);	

		free(con);
	}

}

int  sceneinfos_num(sceneinfos_t con)
{
	if(con) return con->num;

	return 0;
}