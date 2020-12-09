#include <u.h>
#include <libc.h>
#include <bio.h>
#include <regexp.h>

typedef struct Parser Parser;
typedef struct Bag Bag;
typedef struct Rule Rule;
typedef struct Color Color;

struct Parser {
	Biobuf *input;
	Reprog *bag, *rule;
};

struct Bag {
	int color;
	Rule *ruleset;
	Bag *next;
};

struct Rule {
	int color;
	int count;
	Rule *next;
};

struct Color {
	char *name;
	int code;
	Color *next;
};

Color *colors;

int
colorcode(char *name)
{
	static int code = 0;
	Color *c;
	
	for(c = colors; c != nil; c = c->next)
		if(strcmp(c->name, name) == 0)
			return c->code;
	c = mallocz(sizeof(*c), 1);
	if(c == nil) sysfatal("malloc: %r");
	c->name = strdup(name);
	if(c->name == nil) sysfatal("strdup: %r");
	c->code = code++;
	c->next = colors;
	colors = c;
	return c->code;
}

Parser*
parserinit(Parser *p, int fd)
{
	if((p->input = Bfdopen(fd, OREAD)) == nil)
		sysfatal("Bfdopen: %r");
	if((p->bag = regcomp("^([a-z]+ [a-z]+) bags contain (.+)")) == nil)
		sysfatal("regcomp: %r");
	if((p->rule = regcomp("^(([0-9]+ [a-z]+ [a-z]+)|(no other)) bags?[,.]?( |$)")) == nil)
		sysfatal("regcomp: %r");
	return p;
}

Rule*
parserules(Parser *pp, char *s)
{
	Resub m[3];
	Rule *ruleset, *r;
	char *sp, *p, *e;
	
	ruleset = nil;
	for(sp = s; sp[0] != '\0'; sp = m[0].ep){
		memset(m, 0, sizeof(m));
		if(regexec(pp->rule, sp, m, nelem(m)) == 0)
			sysfatal("expected rule");

		if((r = mallocz(sizeof(*r), 1)) == nil)
			sysfatal("malloc: %r");
		if((p = smprint("%.*s", (int)(m[1].ep - m[1].sp), m[1].sp)) == nil)
			sysfatal("smprint: %r");
		r->count = strtol(p, &e, 10);
		if(p == e){
			if(strcmp(p, "no other") != 0)
				sysfatal("color rule expected");
			free(r);
			return nil;
		}
		if(e[0] != ' ') sysfatal("color rule expected");
		e += 1;
		r->color = colorcode(e);
		free(p);
		r->next = ruleset;
		ruleset = r;
	}
	return ruleset;
}

Bag*
parsebag(Parser *pp)
{
	Resub m[3];
	char *s, *color, *list;
	Bag *bag;
	
	if((s = Brdstr(pp->input, '\n', 1)) == nil)
		return nil;
	bag = mallocz(sizeof(*bag), 1);
	if(bag == nil) sysfatal("malloc: %r");
	
	memset(m, 0, sizeof(m));
	if(regexec(pp->bag, s, m, nelem(m)) == 0)
		sysfatal("expected bag description: %r");

	if((color = smprint("%.*s", (int)(m[1].ep - m[1].sp), m[1].sp)) == nil)
		sysfatal("smprint: %r");
	bag->color = colorcode(color);
	free(color);
	
	if((list = smprint("%.*s", (int)(m[2].ep - m[2].sp), m[2].sp)) == nil)
		sysfatal("smprint: %r");
	bag->ruleset = parserules(pp, list);
	free(list);
	
	free(s);
	return bag;
}

Bag*
bagfind(Bag *bags, int code)
{
	Bag *b;
	
	for(b = bags; b; b = b->next)
		if(b->color == code)
			return b;
	return nil;
}

int
cancontain(Bag *bags, int hay, int needle)
{
	Bag *b;
	Rule *r;
	
	b = bagfind(bags, hay);
	for(r = b->ruleset; r; r = r->next){
		if(r->color == needle)
			return 1;
		if(cancontain(bags, r->color, needle))
			return 1;
	}
	return 0;
}

int
mustcontain(Bag *bags, int color)
{
	int n;
	Bag *b;
	Rule *r;
	
	n = 0;
	b = bagfind(bags, color);
	for(r = b->ruleset; r; r = r->next)
		n += r->count + r->count * mustcontain(bags, r->color);
	return n;
}

void
main(int, char**)
{
	Parser parser;
	Bag *b, *bags;
	
	bags = nil;
	parserinit(&parser, 0);
	while((b = parsebag(&parser)) != nil){
		b->next = bags;
		bags = b;
	}
	int shinygold = colorcode("shiny gold");
	int count = 0;
	for(b = bags; b; b = b->next)
		if(cancontain(bags, b->color, shinygold))
			count++;
	print("%d\n", count);
	print("%d\n", mustcontain(bags, shinygold));
}
