#include <u.h>
#include <libc.h>
#include <bio.h>

typedef struct Array Array;

struct Array {
	int alloc;
	int count;
	int *data;
};

Array*
arraynew(void)
{
	Array *a;

	a = mallocz(sizeof(Array), 1);
	if(a == nil)
		sysfatal("malloc: %r");
	return a;
}

void
arrayinsert(Array *a, int x)
{
	if(a->alloc <= a->count){
		a->alloc = 32 + a->alloc*2;
		a->data = realloc(a->data, a->alloc*sizeof(*a->data));
		if(a->data == nil)
			sysfatal("realloc: %r");
	}
	a->data[a->count++] = x;
}

int
numcmp(void *ap, void *bp)
{
	int *a = ap, *b = bp;

	if(*a == *b)
		return 0;
	return *a < *b ? 1 : -1;
}

void
main(int argc, char *argv[])
{
	char *s;
	Biobuf *in;
	Array *nums;

	in = Bfdopen(0, OREAD);
	if(in == nil)
		sysfatal("Bopen: %r");

	nums = arraynew();
	while((s = Brdstr(in, '\n', 1)) != nil){
		int n = strtol(s, nil, 10);
		arrayinsert(nums, n);
		free(s);
	}

	qsort(nums->data, nums->count, sizeof(*nums->data), numcmp);

	for(int x = 0; x < nums->count; x++){
		int a = nums->data[x];
		int b = 2020 - a;
		if(b <= 0)
			continue;
		/* TODO: binary search */
		for(int y = nums->count-1; y >= 0 ; y--){
			int t = nums->data[y];
			if(t > b)
				break;
			if(t == b){
				print("(%d, %d) = %d\n", a, b, a * b);
				break;
			}
		}
	}

	for(int x = 0; x < nums->count; x++){
		int a = nums->data[x];
		for(int y = nums->count-1; y > x; y--){
			int b = nums->data[y];
			int c = 2020 - (a + b);
			if(c <= 0)
				continue;
			/* TODO: binary search */
			for(int z = nums->count-1; z >= 0; z--){
				int t = nums->data[z];
				if(t > c)
					break;
				if(t == c){
					print("(%d, %d, %d) = %d\n", a, b, c, a * b * c);
					break;
				}
			}
		}
	}
}
