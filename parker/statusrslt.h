#ifndef _SCENESTATUSRSLT_H__
#define _SCENESTATUSRSLT_H__
	
typedef struct _scenestatus_st{
	int sceneid;
	int mcuid;
	int type;
	int battery;
	int mcustate;
	int sensorstate;
	int sensortype;
	char time[64];
	char* json;	
}scenestatus_st,*scenestatus_t;

#define _init_capacity (128)

typedef struct _scenestatuses_st{
	int statusnum;
	scenestatus_st *status;
	int capcity;
}scenestatuses_st,*scenestatuses_t;

scenestatuses_t scenestatuses_new();
void scenestatuses_clear(scenestatuses_t con);
int  scenestatuses_add(scenestatuses_t con,scenestatus_t status);
void scenestatuses_free(scenestatuses_t con);

#endif