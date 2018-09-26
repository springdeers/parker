#ifndef _SCENE_RSLT_H__
#define _SCENE_RSLT_H__

typedef struct _sceneinfo_st{
	int  sceneid;
	char* scenename;
	char* templ;
}sceneinfo_st,*sceneinfo_t;

sceneinfo_t  sceneinfo_new();
void         sceneinfo_free(sceneinfo_t);
sceneinfo_t  sceneinfo_assign(sceneinfo_t info,int sceneid,char* name,char* templ);

#define _init_scenes_capacity 256

typedef struct _sceneinfos_st{
	sceneinfo_t *infos;
	int num;
	int capcity;
}sceneinfos_st,*sceneinfos_t;

sceneinfos_t sceneinfos_new();

int  sceneinfos_add(sceneinfos_t con,sceneinfo_t info);

void sceneinfos_clear(sceneinfos_t con);

void sceneinfos_free(sceneinfos_t con);

int  sceneinfos_num(sceneinfos_t con);

#endif