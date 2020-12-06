#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>	

typedef struct Group Group;

struct Group {
	int npeople;
	int answers[26];
	Group *next;
};

Group*
readgroup(Biobuf *in)
{
	int ch;
	Group *g;
	
	while(ch = Bgetc(in), ch != -1 && isspace(ch))
		;
	if(ch == -1) return nil;
	Bungetc(in);
	if((g = mallocz(sizeof(*g), 1)) == nil)
		sysfatal("malloc: %r");
	while(ch = Bgetc(in), ch != -1){
		if(ch == '\n'){
			g->npeople += 1;
			if((ch = Bgetc(in)) == -1 || ch == '\n')
				return g;
			Bungetc(in);
			continue;
		}
		if(ch < 0x61 || ch > 0x7a) /* [a-z] */
			sysfatal("invalid input");
		g->answers[ch - 0x61] += 1;
	}
	free(g);
	return nil;
}

void
main(int, char **)
{
	Biobuf *in;
	Group *groups, *g;
	int i, anyone, everyone;
	
	if((in = Bfdopen(0, OREAD)) == nil) sysfatal("%r");
	groups = nil;
	while((g = readgroup(in)) != nil){
		g->next = groups;
		groups = g;
	}
	anyone = everyone = 0;
	for(g = groups; g; g = g->next){
		for(i = 0; i < nelem(g->answers); i++){
			if(g->answers[i] > 0) anyone++;
			if(g->answers[i] == g->npeople) everyone++;
		}
	}
	print("anyone = %d\n", anyone);
	print("everyone = %d\n", everyone);
	exits(nil);
}
