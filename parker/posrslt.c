#include "posrslt.h"
#include <stdlib.h>
#include <string.h>
#include <string.h>
//#include "cJSON.h"
#include "json/cJSON.h"
#include "mysqldb.h"
#include "appconfig.h"

extern config_st g_conf;
extern mysqlquery_t sqlobj_venue_db;
extern mysqlquery_t sqlobj_userinfo_db;


void positions_clear(positions_t con)
{
	if(con){
		memset(con,0,sizeof(positions_st));
	}
}

int  positions_add(positions_t con,pos_t pos)
{
	if(con->posnum < _maxpos_num)
		con->pos[con->posnum++] = *pos;

	return con->posnum;
}

void positions_freeitems(positions_t con)
{
	int i = 0;

	if(con == NULL) return;

	for(; i < con->posnum ; i++){
		free(con->pos[i].json);
		free(con->pos[i].scenename);
	}
}