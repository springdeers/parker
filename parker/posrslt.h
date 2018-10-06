#ifndef _POSRSLT_H__
#define _POSRSLT_H__

#define _maxpos_num (1024)

typedef struct _pos_st{
	int   sceneid;
	char* scenename;
	int   credit;			// ���ѧ���ֶ�
	int cardid;
	float fpos;
	int state;
	int secondcost;
	char* json;	
}pos_st,*pos_t;



typedef struct _positions_st{
	int posnum;					// �������������ʷλ�õ�����
	pos_st pos[_maxpos_num];			// ������ĸ�����ʷλ������
}positions_st,*positions_t;




void positions_clear(positions_t);
int  positions_add(positions_t con,pos_t pos);
void positions_freeitems(positions_t);


#endif