#include <stdio.h>
#include <arm_sve.h>

int main() {
	int a[] = {1,2,3,4,5,6,7,8};
	int b[] = {8,7,6,5,4,3,2,1};
	int n = sizeof(a)/sizeof(a);
	int result[n];

	svbool_t pg = svwhilelt_b32(0,n);
	for (int i=0; i<n; i += svcntw()){
		svint32_t va = svld1(pg, &a[i]);
		svint32_t vb = svld1(pg, &b[i]);
		svint32_t vresult = svadd_s32_m(pg, va, vb);
		svst1(pg, &result[i], vresult);
		pg = svwhilelt_b32(i + svcntw(), n);
	}

	printf("\n");

	return 0;
}	
