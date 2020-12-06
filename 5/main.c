#include <u.h>
#include <libc.h>
#include <bio.h>

void
decode(char *a, int *row, int *col)
{
	char *p;
	int nr, nc, x;
	
	nr = nc = 0;
	*row = *col = 0;
	for(p = a; p; p++, nr++){
		x = p[0] == 'F' ? 1
		  : p[0] == 'B' ? 0
		  : -1;
		if(x == -1)
			break;
		*row = (*row << 1) | x;
	}
	*row = (1<<nr)-1 - *row;
	for(; p; p++, nc++){
		x = p[0] == 'L' ? 1
		  : p[0] == 'R' ? 0
		  : -1;
		if(x == -1)
			break;
		*col = (*col << 1) | x;
	}
	*col = (1<<nc)-1 - *col;
}

int
arrayinsert(int **p, int *n, int *a, int v)
{
	if(*n >= *a){
		*a = 32 + (*a * 2);
		*p = realloc(*p, *a * sizeof(v));
		if(*p == nil)
			sysfatal("realloc: %r");
	}
	(*p)[(*n)++] = v;
	return v;
}

int
intcmp(void *ap, void *bp)
{
	int *a = ap, *b = bp;
	
	if(*a == *b) return 0;
	return *a > *b ? 1 : -1;
}

void
main(int, char**)
{
	char *s;
	int row, col;
	int nseats, xseats;
	int *seats;
	Biobuf *in;
	
	in = Bfdopen(0, OREAD);
	if(in == nil)
		sysfatal("Bfdopen: %r");
	nseats = xseats = 0;
	while((s = Brdstr(in, '\n', 1)) != nil){
		decode(s, &row, &col);
		arrayinsert(&seats, &nseats, &xseats, row*8 + col);
	}
	qsort(seats, nseats, sizeof(*seats), intcmp);
	print("last id = %d\n", seats[nseats-1]);
	for(int i = 0; i+1 < nseats; i++)
		if(seats[i+1] - seats[i] != 1)
			print("my id = %d\n", seats[i]+1);
}
