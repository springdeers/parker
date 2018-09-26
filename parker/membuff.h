#ifndef __MEM_BUFF_H__
#define __MEM_BUFF_H__

#ifdef __cplusplus
extern "C"{
#endif

	typedef struct _membuff_st{
		char* data;
		int   len;
		int   cap;
	}membuff_st,*membuff_t;

	membuff_t membuff_new(int cap);
	void      membuff_free(membuff_t);

	char*     membuff_data(membuff_t);
	int       membuff_len(membuff_t);
	int       membuff_cap(membuff_t);

	void      membuff_clear(membuff_t);
	membuff_t membuff_addstr(membuff_t mb,char* p,int len);
	membuff_t membuff_addchar(membuff_t mb,char p);
	membuff_t membuff_add_printf(membuff_t, char* fmt, ...);

#define _assure_clearbuff(__buff_ , __size_){\
	if(__buff_ == NULL) __buff_ = membuff_new(__size_);\
	membuff_clear(__buff_);\
	}

#ifdef __cplusplus
}
#endif

#endif