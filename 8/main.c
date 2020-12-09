#include <u.h>
#include <libc.h>
#include <bio.h>

typedef struct VM VM;
typedef struct Inst Inst;

struct VM {
	int pc;
	int acc;
	
	int ncode, xcode;
	Inst **code;
};

struct Inst {
	enum {
		Nop,
		Acc,
		Jmp
	} opcode;
	int arg;
};

char *mnemonic[] = {
	[Nop] "nop",
	[Acc] "acc",
	[Jmp] "jmp",
};

Inst*
parseinst(Biobuf *in)
{
	char *s, *ip, *ap;
	int op;
	Inst *inst;
	
	if((s = Brdstr(in, '\n', 1)) == nil)
		return nil;
	ip = s;
	ap = strchr(s, ' ');
	*ap = '\0';
	ap++;
	
	if((inst = mallocz(sizeof(*inst), 1)) == nil)
		sysfatal("malloc: %r");
	for(op = 0; op < nelem(mnemonic); op++)
		if(strcmp(mnemonic[op], ip) == 0){
			inst->opcode = op;
			break;
		}
	if(op == nelem(mnemonic))
		sysfatal("unrecognized instruction %s", ip);
	inst->arg = strtol(ap, nil, 0);
	
	free(s);
	return inst;
}

VM*
vmnew(void)
{
	VM *vm;
	
	if((vm = mallocz(sizeof(*vm), 1)) == nil)
		sysfatal("malloc: %r");
	vm->pc = 0;
	vm->acc = 0;
	vm->ncode = vm->xcode = 0;
	vm->code = nil;
	return vm;
}

int
vmload(VM *vm, char *file)
{
	Biobuf *in;
	Inst *inst;

	if((in = Bopen(file, OREAD)) == nil)
		return -1;
	while((inst = parseinst(in)) != nil){
		if(vm->ncode >= vm->xcode){
			vm->xcode = 32 + vm->xcode*2;
			vm->code = realloc(vm->code, vm->xcode*sizeof(Inst*));
			if(vm->code == nil)
				sysfatal("realloc: %r");
		}
		vm->code[vm->ncode++] = inst;
	}
	return 0;
}

int
vmrun(VM *vm, int debug)
{
	char *runmap;
	Inst *inst;

	if((runmap = mallocz(vm->ncode*sizeof(char), 1)) == nil)
		sysfatal("malloc: %r");

	while(vm->pc < vm->ncode){
		if(debug)
			print("pc=%d op=%d acc=%d\n",
				vm->pc, vm->code[vm->pc]->opcode, vm->acc);
				
		if(runmap[vm->pc]++ != 0){
			free(runmap);
			return -1;
		}

		inst = vm->code[vm->pc];
		switch(inst->opcode){
		default:
			sysfatal("unimplemented opcode %d", inst->opcode);
		case Nop:
			vm->pc++;
			break;
		case Acc:
			vm->acc += inst->arg;
			vm->pc++;
			break;
		case Jmp:
			vm->pc += inst->arg;
			break;
		}
	}
	free(runmap);
	return 0;
}

void
vmreset(VM *vm)
{
	vm->pc = 0;
	vm->acc = 0;
}

void
insttoggle(Inst *inst)
{
	switch(inst->opcode){
	case Jmp: inst->opcode = Nop; break;
	case Nop: inst->opcode = Jmp; break;
	}
}

void
main(int argc, char **argv)
{
	int debug = 0;
	VM *vm;
	
	ARGBEGIN{
	case 'd': debug = 1; break;
	}ARGEND;
	
	vm = vmnew();
	if(vmload(vm, "/fd/0") == -1)
		sysfatal("vmload: %r");
	for(int i = 0; ; i++){
		for(; vm->code[i]->opcode == Acc; i++)
			;
		insttoggle(vm->code[i]);
		if(vmrun(vm, debug) == 0)
			break;
		vmreset(vm);
		insttoggle(vm->code[i]);
	}
	print("acc = %d\n", vm->acc);
}
