#ifndef _POSRSLT_H__
#define _POSRSLT_H__

#define _maxpos_num (1024)

typedef struct _pos_st{
	int   sceneid;
	char* scenename;
	int   credit;			// 添加学分字段
	int cardid;
	float fpos;
	int state;
	int secondcost;
	char* json;	
}pos_st,*pos_t;



typedef struct _positions_st{
	int posnum;					// 体验过后所有历史位置的数量
	pos_st pos[_maxpos_num];			// 体验过的各个历史位置数据
}positions_st,*positions_t;




void positions_clear(positions_t);
int  positions_add(positions_t con,pos_t pos);
void positions_freeitems(positions_t);


#endif