#include <u.h>
#include <libc.h>
#include <bio.h>

typedef struct Map Map;

struct Map {
	int alloch;
	int width, height;
	char **data;
};

Map*
readmap(Biobuf *in)
{
	char *ln;
	Map *m;

	m = mallocz(sizeof(*m), 1);
	if(m == nil)
		sysfatal("malloc: %r");
	for(int i = 0; (ln = Brdstr(in, '\n', 1)) != nil; i++){
		m->width = Blinelen(in);
		m->height += 1;
		if(m->height > m->alloch){
			m->alloch = 32 + m->alloch*2;
			m->data = realloc(m->data, m->alloch*sizeof(*m->data));
			if(m->data == nil)
				sysfatal("realloc: %r");
		}
		m->data[i] = ln;
	}
	return m;
}

int
traverse(Map *m, int sx, int sy)
{
	int x, y;
	int obstacles;

	x = y = 0;
	obstacles = 0;
	for(;;){
		x = (x + sx) % m->width;
		y = (y + sy);
		if(y >= m->height)
			break;
		if(m->data[y][x] == '#')
			obstacles++;
	}
	return obstacles;
}

void
main(int argc, char *argv[])
{
	Biobuf *in;
	Map *m;

	in = Bfdopen(0, OREAD);
	if(in == nil)
		sysfatal("Bfdopen: %r");

	m = readmap(in);
	int slopes[][2] = {
		/* x, y */
		1, 1,
		3, 1,
		5, 1,
		7, 1,
		1, 2,
	};
	vlong r = 1;
	for(int i = 0; i < nelem(slopes); i++)
		r *= traverse(m, slopes[i][0], slopes[i][1]);;
	print("%lld\n", r);

	exits(nil);
}
