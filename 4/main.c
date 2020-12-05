#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>

typedef struct String String;
typedef struct Parser Parser;
typedef struct Pass Pass;

struct String {
	char *str;
	int len;
	int alloc;
};

struct Parser {
	Biobuf *input;
	char peek;
	int line;
};

struct Pass {
	char *birthyear;
	char *issueyear;
	char *expirationyear;
	char *height;
	char *haircolor;
	char *eyecolor;
	char *passid;
	char *countryid; /* ignore */
};

String*
strgrow(String *s, int len)
{
	if(s->alloc >= s->len + len)
		return s;
	s->alloc = 32 + s->alloc*2 + 1;
	if(s->alloc < s->len + len)
		s->alloc += len;
	s->str = realloc(s->str, s->alloc*sizeof(*s->str));
	if(s->str == nil)
		sysfatal("malloc: %r");
	return s;
}

String*
strnew(void)
{
	String *s;
	
	s = mallocz(sizeof(*s), 1);
	if(s == nil)
		sysfatal("malloc: %r");
	strgrow(s, 1);
	return s;
}

void
strfree(String *s)
{
	if(s == nil) return;
	free(s->str);
	free(s);
}

void
strreset(String *s)
{
	s->len = 0;
	memset(s->str, 0, s->alloc);
}

char
strappend(String *s, char c)
{
	strgrow(s, 1);
	s->str[s->len++] = c;
	s->str[s->len] = '\0';
	return c;
}

char
takeeof(Parser *p)
{
	char ch;
	
	if(p->peek == 0)
		ch = Bgetc(p->input);
	else{
		ch = p->peek;
		p->peek = 0;
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
peek(Parser *p)
{
	if(p->peek == 0)
		p->peek = Bgetc(p->input);
	return p->peek;
}

char
expect(Parser *p, char c)
{
	char ch;
	
	if((ch = take(p)) != c)
		sysfatal("expected '%c', got '%c', line %d\n", c, ch, p->line);
	return ch;
}

char
expectany(Parser *p, char *set)
{
	char ch;
	
	ch = take(p);
	if(strchr(set, ch) == nil)
		sysfatal("expected '[%s]', got '%c', line %d\n", set, ch, p->line);
	return ch;
}

Pass*
passset(Pass *pp, char *key, char *value)
{
	if     (strncmp(key, "byr", 3) == 0)
		pp->birthyear = value;
	else if(strncmp(key, "iyr", 3) == 0)
		pp->issueyear = value;
	else if(strncmp(key, "eyr", 3) == 0)
		pp->expirationyear = value;
	else if(strncmp(key, "hgt", 3) == 0)
		pp->height = value;
	else if(strncmp(key, "hcl", 3) == 0)
		pp->haircolor = value;
	else if(strncmp(key, "ecl", 3) == 0)
		pp->eyecolor = value;
	else if(strncmp(key, "pid", 3) == 0)
		pp->passid = value;
	else if(strncmp(key, "cid", 3) == 0)
		pp->countryid = value;
	else
		return nil;
	return pp;
}

int
parsepair(Parser *p, char **key, char **value)
{
	int ch;
	String *s;
	
	s = strnew();
	while(ch = take(p), isalpha(ch))
		strappend(s, ch);
	untake(p);
	*key = strdup(s->str);

	expect(p, ':');
	
	strreset(s);
	while(ch = take(p), !isspace(ch))
		strappend(s, ch);
	*value = strdup(s->str);
	untake(p);

	strfree(s);
	return 1;
}

Pass*
parsepass(Parser *p)
{
	char ch;
	char *key, *value;
	Pass *pp;

	pp = mallocz(sizeof(*pp), 1);
	if(pp == nil)
		sysfatal("malloc: %r");
		
start:
	while(ch = takeeof(p), ch != -1 && isspace(ch))
		if(ch == '\n')
			p->line++;
	untake(p);
	if(ch == -1)
		return nil;
	
	parsepair(p, &key, &value);
	if(passset(pp, key, value) == nil)
		sysfatal("unrecognized field '%s'\n", key);
	
	ch = expectany(p, " \t\n");
	if(ch == '\n')
		p->line++;
	if(ch == '\n' && peek(p) == '\n')
		return pp;
	
	goto start;
}

void
passfree(Pass *pp)
{
	if(pp == nil) return;
	free(pp->birthyear);
	free(pp->issueyear);
	free(pp->expirationyear);
	free(pp->height);
	free(pp->haircolor);
	free(pp->eyecolor);
	free(pp->passid);
	free(pp->countryid);
	free(pp);
}

int
validatebirthyear(char *s)
{
	int n = strtol(s, nil, 10);
	return n >= 1920 && n <= 2002;
}

int validateissueyear(char *s)
{
	int n = strtol(s, nil, 10);
	return n >= 2010 && n <= 2020;
}

int validateexpirationyear(char *s)
{
	int n = strtol(s, nil, 10);
	return n >= 2020 && n <= 2030;
}

int validateheight(char *s)
{
	int n;
	char *e;
	
	n = strtol(s, &e, 10);
	if     (strncmp(e, "cm", 2) == 0)
		return n >= 150 && n <= 193;
	else if(strncmp(e, "in", 2) == 0)
		return n >= 59 && n <= 76;
	else
		return 0;
}

int validatehaircolor(char *s)
{
	int i = 0;
	
	if(s[i++] != '#')
		return 0;
	while(i < 7)
		if(strchr("0123456789abcdef", s[i++]) == nil)
			return 0;
	if(s[i] != '\0')
		return 0;
	return 1;
}

int validateeyecolor(char *s)
{
	char *colors[] = {
		"amb", "blu", "brn", "gry", "grn", "hzl", "oth",
	};
	for(int i = 0; i < nelem(colors); i++)
		if(strcmp(colors[i], s) == 0)
			return 1;
	return 0;
}

int validatepassid(char *s)
{
	int i = 0;
	
	while(i < 9)
		if(strchr("0123456789", s[i++]) == nil)
			return 0;
	if(s[i] != '\0')
		return 0;
	return 1;
}

int
validate(Pass *pp)
{
	if(!(pp->birthyear && pp->issueyear && pp->expirationyear
	&& pp->height && pp->haircolor && pp->eyecolor && pp->passid))
		return 0;
#ifdef NO
	if(!validatebirthyear(pp->birthyear))
		print("byr %s\n", pp->birthyear);
	if(!validateissueyear(pp->issueyear))
		print("iyr %s\n", pp->issueyear);
	if(!validateexpirationyear(pp->expirationyear))
		print("eyr %s\n", pp->expirationyear);
	if(!validateheight(pp->height))
		print("hgt %s\n", pp->height);
	if(!validatehaircolor(pp->haircolor))
		print("hcl %s\n", pp->haircolor);
	if(!validateeyecolor(pp->eyecolor))
		print("ecl %s\n", pp->eyecolor);
	if(!validatepassid(pp->passid))
		print("pid %s\n", pp->passid);
#endif
	if(!(validatebirthyear(pp->birthyear)
	&&   validateissueyear(pp->issueyear)
	&&   validateexpirationyear(pp->expirationyear)
	&&   validateheight(pp->height)
	&&   validatehaircolor(pp->haircolor)
	&&   validateeyecolor(pp->eyecolor)
	&&   validatepassid(pp->passid)))
		return 0;
	return 1;
}

void
main(int, char**)
{
	Parser parser;
	Pass *p;
	int count;
	
	memset(&parser, 0, sizeof(parser));
	parser.input = Bfdopen(0, OREAD);
	if(parser.input == nil)
		sysfatal("Bfdopen: %r");
	count = 0;
	while((p = parsepass(&parser)) != nil){
		if(validate(p))
			count++;
		passfree(p);
	}
	print("%d valid passports\n", count);
	exits(nil);
}
