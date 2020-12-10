#include <u.h>
#include <libc.h>
#include <bio.h>

typedef struct Array Array;

struct Array {
	int ndata, nalloc;
	int *data;
};

Array*
arraygrow(Array *a, int inc)
{
	if(a->ndata + inc <= a->nalloc)
		return a;
	a->nalloc = inc + a->nalloc*2;
	a->data = realloc(a->data, a->nalloc*sizeof(*a->data));
	if(a->data == nil)
		sysfatal("realloc: %r");
	return a;
}

int
findsum(int *arr, int slice, int sum)
{
	int i, j;

	for(i = 0; i < slice; i++)
	for(j = slice-1; j > i; j--)
		if(arr[i] + arr[j] == sum)
			return 1;
	return 0;
}

int
sum(int *arr, int slice)
{
	int i, x;
	
	for(i = x = 0; i < slice; i++)
		x += arr[i];
	return x;
}

int
numcmp(void *ap, void *bp)
{
	int *a = ap, *b = bp;
	if(*a == *b) return 0;
	return *a > *b ? 1 : -1;
}

void
main(int, char**)
{
	char *s;
	Biobuf *in;
	Array nums;
	int *d, n;
	int i, slice, inv, x;
	
	if((in = Bopen("/fd/0", OREAD)) == nil)
		sysfatal("Bopen: %r");
	memset(&nums, 0, sizeof(nums));
	while((s = Brdstr(in, '\n', 1)) != nil){
		arraygrow(&nums, 1);
		nums.data[nums.ndata] = strtol(s, nil, 10);
		nums.ndata++;
		free(s);
	}
	inv = 0;
	slice = 25;
	d = nums.data;
	n = nums.ndata;
	for(i = 0; i < n - slice; i++){
		inv = d[i+slice];
		if(findsum(&d[i], slice, inv) == 0)
			break;
	}
	slice = 2;
	for(i = 0; i < n - slice;){
		x = sum(&d[i], slice);
		if(x < inv)
			slice++;
		else if(x > inv)
			slice--, i++;
		else
			break;
	}
	qsort(&d[i], slice, sizeof(int), numcmp);
	print("%d+%d = %d\n", d[i], d[i+slice-1], d[i]+d[i+slice-1]);
}
