#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define REG_DEFINE 1
#define REG_END_DEFINE 2
#define REG_CONTENTS 3

#define REGS_SIZE 5120
#define TOKEN_SIZE 256

struct reg
{
    uint16_t type;
    void * next;
};

struct reg_contents
{
    struct reg reg;
    size_t size;
    char contents[1];
};

struct reg_define
{
    struct reg reg;
    char name[1];
};

char _token[TOKEN_SIZE];
char _regs[REGS_SIZE];
int _regs_next = 0;
struct reg * _regs_last = 0;
int _c;
FILE * _in;
FILE * _out;

void * new_generic(uint16_t type, size_t size)
{
    struct reg * ptr = (struct reg *)(size_t)&_regs[_regs_next];
    _regs_next += size;
    if(_regs_last != 0)
    {
        _regs_last->next = ptr;
    }
    if(_regs_next >= REGS_SIZE)
    {
        fputs("error: alloc table overflow.", stdout);
        exit(1);
    }
    memset(ptr, 0, size);
    ptr->type = type;
    ptr->next = 0;
    _regs_last = ptr;
    return ptr;
}

struct reg_contents * new_contents(void * data, size_t size)
{
    struct reg_contents * ptr = new_generic(REG_CONTENTS, sizeof(struct reg_contents) + size);
    memset(ptr->contents, 0, size + 1);
    memcpy(ptr->contents, data, size);
    ptr->size = size;
    return ptr;
}

struct reg_define * new_define(char * name)
{
    struct reg_define * ptr = new_generic(REG_DEFINE, sizeof(struct reg_define) + strlen(name));
    strcpy(ptr->name, name);
    return ptr;
}

struct reg * new_end_define()
{
    return new_generic(REG_END_DEFINE, sizeof(struct reg));
}

bool streq(char * s1, char * s2)
{
    return !strcmp(s1, s2);
}

char next_c()
{
    _c = fgetc(_in);
    if(_c == EOF)
    {
        _c = 0;
    }
    return _c;
}

void concat_c()
{
    char tmp[2];
    tmp[0] = _c;
    tmp[1] = 0;
    strncat(_token, tmp, TOKEN_SIZE - 1);
}

bool scan(bool skip_spaces)
{
    strcpy(_token, "");
    if(skip_spaces) while(_c == ' ' || _c == '\t' || _c == '\r')
    {
        next_c();
    }
    if(_c == 0)
    {
        return false;
    }
    else if(_c == '#' || isalnum(_c) || _c == '_')
    {
        do
        {
            concat_c();
        } while(next_c() && (isalnum(_c) || _c == '_'));
    }
    else if(_c == ' ' || _c == '\r' || _c == '\t')
    {
        do
        {
            concat_c();
        } while(next_c() && (_c == ' ' || _c == '\r' || _c == '\t'));
    }
    else if(_c == '"' || _c == '\'')
    {
        do
        {
            concat_c();
            if(_c == '\\')
            {
                next_c();
                concat_c();
            }
        } while(next_c() && (_c != _token[0]));
    }
    else
    {
        concat_c();
        next_c();
    }
    fflush(stdout);
    return true;
}

void write_token(char * tok)
{
    struct reg * reg;
    reg = (struct reg *)_regs;
    while(reg)
    {
        if(reg->type == REG_DEFINE && streq(((struct reg_define *)reg)->name, tok))
        {
            reg = reg->next;
            while(reg && reg->type != REG_END_DEFINE)
            {
                write_token(((struct reg_contents *)reg)->contents);
                reg = reg->next;
            }
            return;
        }
        reg = reg->next;
    }
    fwrite(tok, 1, strlen(tok), _out);
}

int main(int argc, char ** argv)
{
    bool first;
    int i;
    _in = stdin;
    _out = stdout;

    for(i = 1; i < argc; i++)
    {
        if(streq(argv[i], "-o"))
        {
            i++;
            if(i < argc)
            {
                _out = fopen(argv[i], "w");
            }
        }
        else
        {
            _in = fopen(argv[i], "r");
        }
    }

    next_c();
    while(scan(false))
    {
        if(streq(_token, "#define"))
        {
            scan(true);
            new_define(_token);
            first = true;
            while(scan(first) && !streq(_token, "\n"))
            {
                first = false;
                new_contents(_token, strlen(_token));
            }
            new_end_define();
        }
        else 
        {
            write_token(_token);
        }
    }

    if(_in != stdin) fclose(_in);
    if(_out != stdout) fclose(_out);
}
