#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>

typedef struct Parser Parser;
typedef struct Entry Entry;

struct Parser {
	Biobuf *input;
	char peekch;
	int line;
};

struct Entry {
	char letter;
	int min, max;
	char *password;
};

char
peek(Parser *p)
{
	if(p->peekch == 0)
		p->peekch = Bgetc(p->input);
	return p->peekch;
}

char
takeeof(Parser *p)
{
	char ch;
	
	if(p->peekch == 0)
		ch = Bgetc(p->input);
	else{
		ch = p->peekch;
		p->peekch = 0;
	}
	return ch;
}

char
take(Parser *p)
{
	char ch;
	
	if((ch = takeeof(p)) == -1)
		sysfatal("unexpected eof, line %d", p->line);
	return ch;
}

char
untake(Parser *p)
{
	return Bungetc(p->input);
}

char
expect(Parser *p, char c)
{
	char ch;

	ch = take(p);
	if(ch != c)
		sysfatal("expected '%c', got '%c', line %d", c, ch, p->line);
	return ch;
}

Entry*
parseentry(Parser *p, Entry *e)
{
	char ch;
	char buf[128];
	int i;

	while(ch = takeeof(p), ch != -1 && isspace(ch))
		if(ch == '\n') p->line++;
	if(ch == -1) return nil;
	untake(p);

	i = 0;
	while(ch = take(p), isdigit(ch))
		buf[i++] = ch;
	buf[i++] = '\0';
	untake(p);
	e->min = strtol(buf, nil, 10);

	expect(p, '-');

	i = 0;
	while(ch = take(p), isdigit(ch))
		buf[i++] = ch;
	buf[i++] = '\0';
	untake(p);
	e->max = strtol(buf, nil, 10);

	expect(p, ' ');
	e->letter = take(p);
	expect(p, ':');
	expect(p, ' ');

	i = 0;
	while(ch = take(p), ch != '\n')
		buf[i++] = ch;
	buf[i++] = '\0';
	untake(p);
	e->password = strdup(buf);
	if(e->password == nil)
		sysfatal("strdup: %r");

	return e;
}

int
validatepolicy1(Entry *e)
{
	int n;

	n = 0;
	for(int i = 0; i < strlen(e->password); i++)
		if(e->password[i] == e->letter)
			n++;
	return n >= e->min && n <= e->max;
}

int
validatepolicy2(Entry *e)
{
	char *s;
	int p, q;

	s = e->password;
	p = e->min - 1;
	q = e->max - 1;
	return s[p] != s[q] && (s[p] == e->letter || s[q] == e->letter);
}

void
main(int argc, char *argv[])
{
	int p1count, p2count;
	Parser p;
	Entry e;

	p.line = 0;
	p.input = Bfdopen(0, OREAD);
	if(p.input == nil)
		sysfatal("Bfdopen: %r");

	p1count = 0;
	p2count = 0;
	while(parseentry(&p, &e) != nil){
		if(validatepolicy1(&e))
			p1count++;
		if(validatepolicy2(&e))
			p2count++;
	}
	print("policy 1 = %d\n", p1count);
	print("policy 2 = %d\n", p2count);

	exits(nil);
}
