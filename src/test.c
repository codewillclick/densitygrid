
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "dgrid.h"
#include "bmper.h"

#define newr(a,s) ((a*)malloc(sizeof(a)*s))
#define new(a) newr(a,1)

// { Scruff

// NOTE: Looks like this will only work for 'int' and 'double' of the literal
//   values, thanks to va_args automatically promoting to 'int' and 'double'
//   from their shorter variants.
#define define_aarr(type) \
type* aarr_ ## type (int size, ...) {\
	type* pp = newr(type,size);\
	va_list r;\
	va_start(r,size);\
	int i;\
	for (i=0; i < size; ++i)\
		pp[i] = va_arg(r,type);\
	va_end(r);\
	return pp;\
}

float* af(float f) {
	float* p = new(float);
	*p = f;
	return p;
}
void** avarr(int size, ...) {
	void** pp = (void**)malloc(sizeof(void*)*(size+1));
	pp[size] = 0;
	
	va_list r;
	va_start(r,size);
	int i;
	for (i=0; i < size; ++i)
		pp[i] = va_arg(r,void*);
	va_end(r);
	return pp;
}
// }

define_aarr(double)
define_aarr(int)

// { Callbacks
void* new_cell() {
	int* p = aarr_int(1,0);
	return (void*)p;
}
void del_cell(void* cell, int del_flags) {
	free(cell);
}
valpair* topairs(void* vals, int* count, dgrid_config* conf) {
	*count = 0;
	
	double* f = (double*)vals;
	valpair* p = new(valpair);
	*count = 1;
	
	(*p).x = (int)f[0];
	(*p).y = (int)f[1];
	(*p).val = aarr_int(1,1);
	
	return p;
}
void ctrans(int* c, dgrid* dg) {
	c[0] -= dg->frame.x;
	c[1] -= dg->frame.y;
}
void act(void* cell, void* val) {
	printf("act: %x %x\n",cell,val); fflush(stdout);
	int* p = (int*)cell;
	*p += *(int*)val;
}
void iter_callback_bmp_white(int x, int y, void* val, void* params) {
	BMP24File* bmp = (BMP24File*)params;
	bmp->pr[y][x] = pix24_int(0xffffff);
}
// }

int main(int argc, char** argv) {
	
	// Pull data from source.
	FILE* f = stdin;
	
	// Process with densitygrid.
	dgrid dg;
	init_dgrid2(&dg, 0,0, 10,10, new_cell,del_cell,0);
	
	dgrid_config conf;
	init_dgrid_config(&conf, topairs,ctrans,act);
	dgrid_config_free(&conf,
		DGRID_FREE, 0,
		0,          0);
	
	printf("Do something.\n");
	
	float x;
	float y;
	while (fscanf(f,"%f %f\n",&x,&y) == 2) {
		printf("read: %f %f\n",x,y);
		double* vals = aarr_double(2,x,y);
		vals[0] = x;
		vals[1] = y;
		dgrid_process_values(&dg,&conf,vals);
		free(vals);
	}
	printf("ending xy: %f %f\n",x,y); fflush(stdout);
	
	// Render to bmp file with bmper.
	BMP24File bmp;
	init_BMP24File(&bmp,dg.frame.w,dg.frame.h);
	BMP24File_fill(&bmp,pix24_int(0x303030));
	
	// TODO: A callback for cell iteration would be nice.
	/*
	int i,j,v;
	int* p;
	for (i=0; i < dg.frame.h; ++i) {
		for (j=0; j < dg.frame.w; ++j) {
			p = (int*)dg.rows[i][j];
			v = (p ? *p : 0);
			printf("%2i ",v); fflush(stdout);
			if (v)
				bmp.pr[i][j] = pix24_int(0xffffff);
		}
		printf("\n");
	}
	// */
	dgrid_iterate_values(&dg, iter_callback_bmp_white ,&bmp);
	
	printf("point X\n"); fflush(stdout);
	FILE* bf = fopen("output.bmp","wb");
	BMP24File_write(&bmp,bf);
	fclose(bf);
	printf("point X.1\n"); fflush(stdout);
	del_BMP24File(&bmp,DEL_NONE);
	printf("point X.2\n"); fflush(stdout);
	
	del_dgrid(&dg,DEL_NONE);
	
	return 0;
}

