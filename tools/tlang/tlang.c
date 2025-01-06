#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "tlvm.h"

#define TOKEN_MAX 128
#define TOKEN_LEN 127

typedef struct token_t
{
    char contents[TOKEN_MAX];
    int line;
    int column;
} token_t;

typedef struct infile_t
{
    FILE * file;
    char name[TOKEN_MAX];
    int line;
    int column;
    int c;
    bool is_eof;
    token_t curr;
    token_t next;
} infile_t;

typedef struct outfile_t
{
    FILE * file;
} outfile_t;

typedef struct symbol_t
{
    enum
    {
        DT_MASK_TYPE = 128 | 256 | 512 | 1024,
        DT_MASK_STRUCT = 127,
        DT_CHAR = 128,
        DT_INT = 256,
        DT_LONG = 512,
        DT_STRUCT = 1024,
        DT_POINTER = 2048,
        DT_POINTER2 = 4096
    } datatype;
    bool is_local;
    int position;
    struct symbol_t * next;
    struct symbol_t * child;
    char name[1];
} symbol_t;

infile_t * _in = NULL;
outfile_t * _out = NULL;
symbol_t * _locals = NULL;
symbol_t * _globals = NULL;
symbol_t * _funcs = NULL;
symbol_t * _structs = NULL;
int _last_struct_id = 0;
int _num_label = 0;
int _return_label = 0;
int _break_label = 0;
int _continue_label = 0;
int _return_dtype = 0;

void error(char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if(_in != NULL)
    {
        fprintf(stderr, "%s:%d:%d: ", _in->name, _in->line, _in->column);
    }
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, ".\n");
    va_end(args);
    exit(1);
}

void * new(size_t size)
{
    void * obj = malloc(size);
    if(obj == 0)
    {
        error("memory overflow");
    }
    memset(obj, 0, size);
    return obj;
}

int calc_size(int dtype)
{
    if(dtype == 0) return 0;
    if(dtype & DT_POINTER2) return 2;
    if(dtype & DT_POINTER) return 2;
    if((dtype & DT_MASK_TYPE) == DT_CHAR) return 1;
    if((dtype & DT_MASK_TYPE) == DT_INT) return 2;
    if((dtype & DT_MASK_TYPE) == DT_LONG) return 4;
    if((dtype & DT_MASK_TYPE) == DT_STRUCT)
    {
        int size = 0;
        symbol_t * sym = _structs;
        while(sym)
        {
            if((sym->datatype & DT_MASK_STRUCT) == (dtype & DT_MASK_STRUCT))
            {
                sym = sym->child;
                while(sym)
                {
                    size += calc_size(sym->datatype);
                    sym = sym->next;
                }
            }
            sym = sym->next;
        }
        return size;
    }
    error("invalid datatype [0x%x]", dtype);
}

bool global_level_exists(char * name)
{
    symbol_t * sym = _globals;
    while(sym)
    {
        if(!strcmp(sym->name, name))
        {
            return true;
        }
        sym = sym->next;
    }
    sym = _funcs;
    while(sym)
    {
        if(!strcmp(sym->name, name))
        {
            return true;
        }
        sym = sym->next;
    }
    sym = _structs;
    while(sym)
    {
        if(!strcmp(sym->name, name))
        {
            return true;
        }
        sym = sym->next;
    }
    return false;
}

bool global_exists(char * name)
{
    symbol_t * sym = _globals;
    while(sym)
    {
        if(!strcmp(sym->name, name))
        {
            return true;
        }
        sym = sym->next;
    }
    return false;
}

bool struct_exists(char * name)
{
    symbol_t * sym = _structs;
    while(sym)
    {
        if(!strcmp(sym->name, name))
        {
            return true;
        }
        sym = sym->next;
    }
    return false;
}

bool function_exists(char * name)
{
    symbol_t * sym = _funcs;
    while(sym)
    {
        if(!strcmp(sym->name, name))
        {
            return true;
        }
        sym = sym->next;
    }
    return false;
}

bool local_exists(char * name)
{
    symbol_t * sym = _locals;
    while(sym)
    {
        if(!strcmp(sym->name, name))
        {
            return true;
        }
        sym = sym->next;
    }
    return false;
}

symbol_t * get_global(char * name)
{
    symbol_t * sym = _globals;
    while(sym)
    {
        if(!strcmp(sym->name, name))
        {
            return sym;
        }
        sym = sym->next;
    }
    error("%s not found", name);
}
symbol_t * get_local(char * name)
{
    symbol_t * sym = _locals;
    while(sym)
    {
        if(!strcmp(sym->name, name))
        {
            return sym;
        }
        sym = sym->next;
    }
    error("%s not found", name);
}

symbol_t * get_function(char * name)
{
    symbol_t * sym = _funcs;
    while(sym)
    {
        if(!strcmp(sym->name, name))
        {
            return sym;
        }
        sym = sym->next;
    }
    error("%s not found", name);
}

symbol_t * get_struct(char * name)
{
    symbol_t * sym = _structs;
    while(sym)
    {
        if(!strcmp(sym->name, name))
        {
            return sym;
        }
        sym = sym->next;
    }
    error("%s not found", name);
}

symbol_t * new_global(char * name, int dtype)
{
    if(global_level_exists(name)) error("%s already exists", name);
    symbol_t * sym = new(sizeof(symbol_t) + strlen(name));
    strcpy(sym->name, name);
    sym->datatype = dtype;
    sym->next = _globals;
    _globals = sym;
    return sym;
}

symbol_t * new_function(char * name, int dtype)
{
    if(global_level_exists(name)) error("%s already exists", name);
    symbol_t * sym = new(sizeof(symbol_t) + strlen(name));
    strcpy(sym->name, name);
    sym->datatype = dtype;
    sym->next = _funcs;
    _funcs = sym;
    return sym;
}

symbol_t * new_local(char * name, int dtype)
{
    if(local_exists(name)) error("%s already exists", name);
    symbol_t * sym = new(sizeof(symbol_t) + strlen(name));
    strcpy(sym->name, name);
    sym->datatype = dtype;
    sym->next = _locals;
    _locals = sym;
    return sym;
}

symbol_t * new_struct(char * name)
{
    if(global_level_exists(name)) error("%s already exists", name);
    symbol_t * sym = new(sizeof(symbol_t) + strlen(name));
    strcpy(sym->name, name);
    sym->datatype = DT_STRUCT | _last_struct_id++;
    sym->next = _structs;
    _structs = sym;
    return sym;
}

symbol_t * new_field(symbol_t * structure, char * name)
{
    symbol_t * sym = new(sizeof(symbol_t) + strlen(name));
    strcpy(sym->name, name);
    sym->datatype = DT_STRUCT | _last_struct_id++;
    sym->position = 0;
    if(structure->child == NULL)
    {
        structure->child = sym;
    }
    else
    {
        symbol_t * last = structure->child;
        while(last->next != NULL)
        {
            if(!strcmp(last->name, name))
            {
                error("%s already exists", name);
            }
            sym->position += calc_size(last->datatype);
            last = last->next;
        }
        last->next = sym;
    }
    return sym;
}

void clear_locals()
{
    symbol_t * sym = _locals;
    while(sym)
    {
        symbol_t * next_sym = sym->next;
        free(sym);
        sym = next_sym;
    }
    _locals = NULL;
}

void emit(int cg, int value, void * data, int len)
{
    fwrite(&cg, 1, 2, _out->file);
    fwrite(&value, 1, 2, _out->file);
    fwrite(&len, 1, 2, _out->file);
    if(len > 0)
        fwrite(data, 1, len, _out->file);
    fflush(_out->file);
}

void emit_val(int cg, int value)
{
    emit(cg, value, 0, 0);
}

void emit_str(int cg, int value, char * string)
{
    emit(cg, value, string, strlen(string) + 1);
}

char c_next()
{
    _in->c = fgetc(_in->file);
    if(_in->c < 0)
    {
        _in->c = 0;
        _in->is_eof = true;
        return 0;
    }
    _in->is_eof = false;
    _in->column ++;
    if(_in->c == '\n' || _in->c == '\r') _in->column = 0;
    if(_in->c == '\n') _in->line ++;
    return _in->c;
}

char c()
{
    return _in->c;
}

bool c_is(char c)
{
    return _in->c == c;
}

bool c_between(char min, char max)
{
    return _in->c >= min && _in->c <= max;
}

char * curr_contents()
{
    return _in->curr.contents;
}

bool curr_is(char * sym)
{
    return !strcmp(sym, _in->curr.contents);
}

bool curr_is_eof()
{
    return _in->curr.contents[0] == 0;
}

bool curr_is_symbol()
{
    char c = _in->curr.contents[0];
    return (isalnum(c) || c == '_');
}

bool curr_is_number()
{
    return isdigit(_in->curr.contents[0]);
}

int curr_parse()
{
    int value = 0;
    char * c = _in->curr.contents;
    if(*c == '0')
    {
        c++;
        if(*c == 'x')
        {
            c++;
            while(isxdigit(*c))
            {
                value <<= 4;
                if(isdigit(*c))
                    value += *c - '0';
                else
                    value += *c - 'a' + 10;
                c++;
            }
        }
        else if(*c == 'o')
        {
            c++;
            while(*c >= '0' && *c <= '7')
            {
                value <<= 3;
                value += *c - '0';
                c++;
            }
        }
        else if(*c == 'b')
        {
            c++;
            while(*c >= '0' && *c <= '1')
            {
                value <<= 1;
                value += *c - '0';
                c++;
            }
        }
        else
        {
            while(*c >= '0' && *c <= '9')
            {
                value *= 10;
                value += *c - '0';
                c++;
            }
        }
    }
    else
    {
        while(*c >= '0' && *c <= '9')
        {
            value *= 10;
            value += *c - '0';
            c++;
        }
    }
    return value;
}

void next_concat(char c)
{

    char cat[2];
    cat[0] = c;
    cat[1] = 0;
    if(strlen(_in->next.contents) >= TOKEN_LEN) return;
    strcat(_in->next.contents, cat);
}

bool next()
{
    memcpy(&_in->curr, &_in->next, sizeof(token_t));
    memset(&_in->next, 0, sizeof(token_t));
    while(c_is(' ') || c_is('\t') || c_is('\r') || c_is('\n') || c_is('!'))
    {
        if(c_is('!'))
        {
            while(!c_is('\n'))
            {
                if(c_is(0)) break;
                c_next();
            }
        }
        else c_next();
    }
    _in->next.column = _in->column;
    _in->next.line = _in->line;
    if(c_is(0)) return !curr_is_eof();
    if(c_between('a', 'z') || c_between('A', 'Z') || c_between('0', '9') || c_is('_'))
    {
        while(c_between('a', 'z') || c_between('A', 'Z') || c_between('0', '9') || c_is('_') || c_is('.'))
        {
            next_concat(tolower(c()));
            c_next();
        }
    }
    else if(c_is('<'))
    {
        next_concat(c());
        c_next();
        if(c_is('>') || c_is('=') || c_is('<'))
        {
            next_concat(c());
            c_next();
        }
    }
    else if(c_is('>'))
    {
        next_concat(c());
        c_next();
        if(c_is('>') || c_is('='))
        {
            next_concat(c());
            c_next();
        }
    }
    else if(c_is(':'))
    {
        next_concat(c());
        c_next();
        if(c_is('='))
        {
            next_concat(c());
            c_next();
        }
    }
    else if(c_is('-'))
    {
        next_concat(c());
        c_next();
        if(c_is('>'))
        {
            next_concat(c());
            c_next();
        }
    }
    else if(c_is('+'))
    {
        next_concat(c());
        c_next();
        if(c_is('+'))
        {
            next_concat(c());
            c_next();
        }
    }
    else if(c_is('-'))
    {
        next_concat(c());
        c_next();
        if(c_is('-'))
        {
            next_concat(c());
            c_next();
        }
    }
    else 
    {
        next_concat(c());
        c_next();
    }
    return !curr_is_eof();
}

int datatype_parse()
{
    int dtype = 0;
    if(curr_is("@")) 
    {
        dtype |= DT_POINTER;
        next();
    }
    if(curr_is("@"))
    {
        dtype |= DT_POINTER2;
        next();
    }
    if(curr_is("char"))
    {
        dtype |= DT_CHAR;
        next();
    }
    else if(curr_is("int"))
    {
        dtype |= DT_INT;
        next();
    }
    else if(curr_is("long"))
    {
        dtype |= DT_LONG;
        next();
    }
    else
    {
        symbol_t * sym = _structs;
        while(sym)
        {
            if(!strcmp(sym->name, curr_contents()))
            {
                dtype |= sym->datatype;
                next();
                return dtype;
            }
            sym = sym->next;
        }
        error("datatype not found: %s", curr_contents());
    }
    return dtype;
}

int datatype_deref(int dtype)
{
    if(dtype & DT_POINTER2) return dtype & (~DT_POINTER2);
    if(dtype & DT_POINTER) return dtype & (~DT_POINTER);
    error("invalid dereference");
}

int datatype_ref(int dtype)
{
    if(dtype & DT_POINTER) return dtype | DT_POINTER2;
    if((dtype & DT_POINTER2) == 0) return dtype | DT_POINTER;
    error("invalid reference");
}

void function_level();
void expr(int dtype);

void expr_attrib(int dtype, int pointer_lvl)
{
    for(int i = 0; i < pointer_lvl; i++)
    {
        emit_val(CG_LOAD_16, 0);
    }
    switch(calc_size(dtype))
    {
        case 1:
            emit_val(CG_PUSH_A16, 0);
            expr(dtype);
            emit_val(CG_CONV_A8_TO_A16, 0);
            emit_val(CG_POP_B16, 0);
            emit_val(CG_XCHG_A16_B16, 0);
            emit_val(CG_STORE_8, 0);
            break;
        case 2:
            emit_val(CG_PUSH_A16, 0);
            expr(dtype);
            emit_val(CG_POP_B16, 0);
            emit_val(CG_XCHG_A16_B16, 0);
            emit_val(CG_STORE_8, 0);
            break;
        case 4:
            emit_val(CG_PUSH_A16, 0);
            expr(dtype);
            emit_val(CG_STORE_32_FROM_STACK, 0);
            break;
        default:
            error("invalid datatype operation");
            break;
    }
}

void auto_conv(int dest_dtype, int src_dtype)
{
    if(dest_dtype == 0) return;
    if(calc_size(dest_dtype) == calc_size(src_dtype)) return;
    switch (calc_size(src_dtype))
    {
        case 1:
            switch (calc_size(dest_dtype))
            {
                case 2:
                    emit_val(CG_CONV_A8_TO_A16, 0);
                    return;
                case 4:
                    emit_val(CG_CONV_A8_TO_BA32, 0);
                    return;
            }
            break;
        case 2:
            switch (calc_size(dest_dtype))
            {
                case 1:
                    emit_val(CG_CONV_A16_TO_A8, 0);
                    return;
                case 4:
                    emit_val(CG_CONV_A16_TO_BA32, 0);
                    return;
            }
            break;
        case 4:
            switch (calc_size(dest_dtype))
            {
                case 1:
                    emit_val(CG_CONV_BA32_TO_A8, 0);
                    return;
                case 2:
                    emit_val(CG_CONV_BA32_TO_A16, 0);
                    return;
            }
            break;
    }
    error("Conversion not supported");
}

void expr_value(int dtype)
{
    int internal_dtype = dtype;
    int internal_pointer_lvl = 0;
    symbol_t * sym;
    if(curr_is_number())
    {
        switch(calc_size(dtype))
        {
            case 1:
                emit_val(CG_SET_A8, curr_parse());
                next();
                break;
            case 2:
                emit_val(CG_SET_A16, curr_parse());
                next();
                break;
            case 4:
                emit_val(CG_SET_B16, (curr_parse() >> 16) & 0xffff);
                emit_val(CG_SET_A16, curr_parse() & 0xffff);
                next();
                break;
            default:
                error("invalid datatype");
                break;
        }
    }
    else if(curr_is_symbol() || curr_is("@"))
    {
        while(curr_is("@"))
        {
            internal_dtype = datatype_ref(internal_dtype);
            internal_pointer_lvl++;
            next();
        }
        if(global_exists(curr_contents()))
        {
            sym = get_global(curr_contents());
            next();
            emit_str(CG_SET_A16_AS_GLOBAL_PTR, 0, sym->name);
            if(curr_is(":="))
            {
                next();
                expr_attrib(sym->datatype, internal_pointer_lvl);
            }
            else
            {
                for(int i = 0; i < internal_pointer_lvl; i++)
                {
                    emit_val(CG_LOAD_16, 0);
                }
                switch(calc_size(sym->datatype))
                {
                    case 1:
                        emit_val(CG_LOAD_8, 0);
                        break;
                    case 2:
                        emit_val(CG_LOAD_16, 0);
                        break;
                    case 4:
                        emit_val(CG_LOAD_32, 0);
                        break;
                    default:
                        error("invalid datatype operation");
                        break;
                }
                auto_conv(internal_dtype, sym->datatype);
            }
        }
        else if(local_exists(curr_contents()))
        {
            sym = get_local(curr_contents());
            next();
            emit_str(CG_SET_A16_AS_LOCAL_PTR, 0, sym->name);
            if(curr_is(":="))
            {
                next();
                expr_attrib(sym->datatype, internal_pointer_lvl);
            }
            else
            {
                for(int i = 0; i < internal_pointer_lvl; i++)
                {
                    emit_val(CG_LOAD_16, 0);
                }
                switch(calc_size(sym->datatype))
                {
                    case 1:
                        emit_val(CG_LOAD_8, 0);
                        break;
                    case 2:
                        emit_val(CG_LOAD_16, 0);
                        break;
                    case 4:
                        emit_val(CG_LOAD_32, 0);
                        break;
                    default:
                        error("invalid datatype operation");
                        break;
                }
            }
            auto_conv(internal_dtype, sym->datatype);
        }
        else error("%s not found", curr_contents());
    }
}

void expr_shift(int dtype)
{
    char op[10];
    expr_value(dtype);
    while(curr_is("<<") || curr_is(">>"))
    {
        strcpy(op, curr_contents());
        next();
        switch(calc_size(dtype))
        {
            case 1:
                emit_val(CG_PUSH_A8, 0);
                expr_value(dtype);
                emit_val(CG_COPY_A8_TO_B8, 0);
                emit_val(CG_POP_A8, 0);
                if(!strcmp(op, "<<"))
                    emit_val(CG_SHL_8, 0);
                else if(!strcmp(op, ">>"))
                    emit_val(CG_SHR_8, 0);
                break;
            case 2:
                emit_val(CG_PUSH_A16, 0);
                expr_value(dtype);
                emit_val(CG_COPY_A16_TO_B16, 0);
                emit_val(CG_POP_A16, 0);
                if(!strcmp(op, "<<"))
                    emit_val(CG_SHL_16, 0);
                else if(!strcmp(op, ">>"))
                    emit_val(CG_SHR_16, 0);
                break;
            case 4:
                emit_val(CG_PRE_CALL, 0);
                emit_val(CG_PUSH_BA32, 0);
                expr_value(dtype);
                emit_val(CG_PUSH_BA32, 0);
                if(!strcmp(op, "<<"))
                    emit_str(CG_CALL, 0, "shl32");
                else if(!strcmp(op, ">>"))
                    emit_str(CG_CALL, 0, "shr32");
                emit_val(CG_POST_CALL, 8);
                break;
            default:
                error("invalid datatype operation");
                break;
        }
    }
}

void expr_mul(int dtype)
{
    char op[10];
    expr_shift(dtype);
    while(curr_is("*") || curr_is("/") || curr_is("%"))
    {
        strcpy(op, curr_contents());
        next();
        switch(calc_size(dtype))
        {
            case 1:
                emit_val(CG_PUSH_A8, 0);
                expr_shift(dtype);
                emit_val(CG_COPY_A8_TO_B8, 0);
                emit_val(CG_POP_A8, 0);
                if(!strcmp(op, "*"))
                    emit_val(CG_MUL_8, 0);
                else if(!strcmp(op, "/"))
                    emit_val(CG_DIV_8, 0);
                else if(!strcmp(op, "%"))
                    emit_val(CG_MOD_8, 0);
                break;
            case 2:
                emit_val(CG_PUSH_A16, 0);
                expr_shift(dtype);
                emit_val(CG_COPY_A16_TO_B16, 0);
                emit_val(CG_POP_A16, 0);
                if(!strcmp(op, "*"))
                    emit_val(CG_MUL_16, 0);
                else if(!strcmp(op, "/"))
                    emit_val(CG_DIV_16, 0);
                else if(!strcmp(op, "%"))
                    emit_val(CG_MOD_16, 0);
                break;
            case 4:
                emit_val(CG_PRE_CALL, 0);
                emit_val(CG_PUSH_BA32, 0);
                expr_shift(dtype);
                emit_val(CG_PUSH_BA32, 0);
                if(!strcmp(op, "*"))
                    emit_str(CG_CALL, 0, "mul32");
                else if(!strcmp(op, "/"))
                    emit_str(CG_CALL, 0, "div32");
                else if(!strcmp(op, "%"))
                    emit_str(CG_CALL, 0, "mod32");
                emit_val(CG_POST_CALL, 8);
                break;
            default:
                error("invalid datatype operation");
                break;
        }
    }
}

void expr_add(int dtype)
{
    char op[10];
    expr_mul(dtype);
    while(curr_is("+") || curr_is("-"))
    {
        strcpy(op, curr_contents());
        next();
        switch(calc_size(dtype))
        {
            case 1:
                emit_val(CG_PUSH_A8, 0);
                expr_mul(dtype);
                emit_val(CG_COPY_A8_TO_B8, 0);
                emit_val(CG_POP_A8, 0);
                if(!strcmp(op, "+"))
                    emit_val(CG_ADD_8, 0);
                else if(!strcmp(op, "-"))
                    emit_val(CG_SUB_8, 0);
                break;
            case 2:
                emit_val(CG_PUSH_A16, 0);
                expr_mul(dtype);
                emit_val(CG_COPY_A16_TO_B16, 0);
                emit_val(CG_POP_A16, 0);
                if(!strcmp(op, "+"))
                    emit_val(CG_ADD_16, 0);
                else if(!strcmp(op, "-"))
                    emit_val(CG_SUB_16, 0);
                break;
            case 4:
                emit_val(CG_PRE_CALL, 0);
                emit_val(CG_PUSH_BA32, 0);
                expr_mul(dtype);
                emit_val(CG_PUSH_BA32, 0);
                if(!strcmp(op, "+"))
                    emit_str(CG_CALL, 0, "add32");
                else if(!strcmp(op, "-"))
                    emit_str(CG_CALL, 0, "sub32");
                emit_val(CG_POST_CALL, 8);
                break;
            default:
                error("invalid datatype operation");
                break;
        }
    }
}

void expr_cmp(int dtype)
{
    char op[10];
    expr_add(dtype);
    while(curr_is("=") || curr_is("<>") || curr_is("<=") || curr_is(">=") || curr_is("<") || curr_is(">"))
    {
        strcpy(op, curr_contents());
        next();
        switch(calc_size(dtype))
        {
            case 1:
                emit_val(CG_CONV_A8_TO_A16, 0);
                emit_val(CG_PUSH_A16, 0);
                expr_add(dtype);
                emit_val(CG_CONV_A8_TO_A16, 0);
                emit_val(CG_COPY_A16_TO_B16, 0);
                emit_val(CG_POP_A16, 0);
                if(!strcmp(op, "="))
                    emit_val(CG_SET_IF_EQ_16, 0);
                else if(!strcmp(op, "<>"))
                    emit_val(CG_SET_IF_NE_16, 0);
                else if(!strcmp(op, "<="))
                    emit_val(CG_SET_IF_LE_16, 0);
                else if(!strcmp(op, "<"))
                    emit_val(CG_SET_IF_LT_16, 0);
                else if(!strcmp(op, ">="))
                    emit_val(CG_SET_IF_GE_16, 0);
                else if(!strcmp(op, ">"))
                    emit_val(CG_SET_IF_GT_16, 0);
                break;
            case 2:
                emit_val(CG_PUSH_A16, 0);
                expr_add(dtype);
                emit_val(CG_COPY_A16_TO_B16, 0);
                emit_val(CG_POP_A16, 0);
                if(!strcmp(op, "="))
                    emit_val(CG_SET_IF_EQ_16, 0);
                else if(!strcmp(op, "<>"))
                    emit_val(CG_SET_IF_NE_16, 0);
                else if(!strcmp(op, "<="))
                    emit_val(CG_SET_IF_LE_16, 0);
                else if(!strcmp(op, "<"))
                    emit_val(CG_SET_IF_LT_16, 0);
                else if(!strcmp(op, ">="))
                    emit_val(CG_SET_IF_GE_16, 0);
                else if(!strcmp(op, ">"))
                    emit_val(CG_SET_IF_GT_16, 0);
                break;
            case 4:
                emit_val(CG_PRE_CALL, 0);
                emit_val(CG_PUSH_BA32, 0);
                expr_add(dtype);
                emit_val(CG_PUSH_BA32, 0);
                if(!strcmp(op, "="))
                    emit_str(CG_CALL, 0, "seteq32");
                else if(!strcmp(op, "<>"))
                    emit_str(CG_CALL, 0, "setne32");
                else if(!strcmp(op, "<="))
                    emit_str(CG_CALL, 0, "setle32");
                else if(!strcmp(op, "<"))
                    emit_str(CG_CALL, 0, "setlt32");
                else if(!strcmp(op, ">="))
                    emit_str(CG_CALL, 0, "setge32");
                else if(!strcmp(op, ">"))
                    emit_str(CG_CALL, 0, "setgt32");
                emit_val(CG_POST_CALL, 8);
                break;
            default:
                error("invalid datatype operation");
                break;
        }
    }
}

void expr(int dtype)
{
    expr_cmp(dtype);
}

void do_var(bool root_level)
{
    char name[TOKEN_MAX];
    int dtype;
    next();
    while(!curr_is(";") && !curr_is_eof())
    {
        if(!curr_is_symbol()) error("variable name expected");
        strcpy(name, curr_contents());
        next();
        if(!curr_is("as")) error("'as' expected");
        next();
        dtype = datatype_parse();
        if(root_level)
        {
            new_global(name, dtype);
            emit_str(CG_GLOBAL_VAR, calc_size(dtype), name);
        }
        else
        {
            new_local(name, dtype);
            emit_str(CG_LOCAL_VAR, calc_size(dtype), name);
        }
        if(!curr_is(",")) break;
        next();
    }
}

void do_argument()
{
    int dtype;
    char name[TOKEN_MAX];
    if(curr_is(")")) return;
    if(!curr_is_symbol()) error("argument name expected");
    strcpy(name, curr_contents());
    next();
    if(!curr_is("as")) error("'as' expected");
    next();
    dtype = datatype_parse();
    new_local(name, dtype);
    emit_str(CG_ARG_VAR, calc_size(dtype), name);
    if(curr_is(","))
    {
        next();
        do_argument();
    }
}

void do_function()
{
    int dtype = 0;
    char name[TOKEN_MAX];
    _return_label = ++_num_label;
    strcpy(name, curr_contents());
    next();
    emit_str(CG_PUBLIC_LABEL, 0, name);
    emit_str(CG_NAME_LABEL, 0, name);
    emit_val(CG_FUNCTION_START, 0);
    clear_locals();
    if(!curr_is("(")) error("'(' expected");
    next();
    do_argument();
    if(!curr_is(")")) error("')' expected");
    next();
    if(curr_is("as"))
    {
        next();
        dtype = datatype_parse();
    }
    _return_dtype = dtype;
    new_function(name, dtype);
    function_level();
    emit_val(CG_NUM_LABEL, _return_label);
    emit_val(CG_FUNCTION_END, 0);
    _return_label = 0;
    _return_dtype = 0;
}

void do_if()
{
    int lbl_else;
    int lbl_end = ++_num_label;
    next();
    expr(DT_INT);
    emit_val(CG_JMP_IF_FALSE, lbl_end);
    function_level();
    if(curr_is("else"))
    {
        lbl_else = lbl_end;
        lbl_end = ++_num_label;
        emit_val(CG_JMP, lbl_end);
        emit_val(CG_NUM_LABEL, lbl_else);
        next();
        function_level();
    }
    emit_val(CG_NUM_LABEL, lbl_end);
}

void do_return()
{
    next();
    if(_return_dtype != 0)
    {
        expr(_return_dtype);
    }
    emit_val(CG_JMP, _return_label);
}

void do_while()
{
    int old_continue = _continue_label;
    int old_break = _break_label;
    _continue_label = ++_num_label;
    _break_label = ++_num_label;
    next();
    emit_val(CG_NUM_LABEL, _continue_label);
    expr(DT_INT);
    emit_val(CG_JMP_IF_FALSE, _break_label);
    function_level();
    emit_val(CG_JMP, _continue_label);
    emit_val(CG_NUM_LABEL, _break_label);
    _continue_label = old_continue;
    _break_label = old_break;
}

void do_for()
{
    int old_continue = _continue_label;
    int old_break = _break_label;
    int contents_label = ++_num_label;
    int comp_label = ++_num_label;
    _continue_label = ++_num_label;
    _break_label = ++_num_label;
    symbol_t * sym;
    next();
    sym = get_local(curr_contents());
    if(calc_size(sym->datatype) != 2) error("invalid datatype");
    next();
    if(!curr_is(":=")) error("':=' expected");
    next();
    expr(sym->datatype);
    emit_val(CG_COPY_A16_TO_B16, 0);
    emit_str(CG_SET_A16_AS_LOCAL_PTR, 0, sym->name);
    emit_val(CG_STORE_16, 0);
    if(!curr_is("to")) error("'to' expected");
    next();
    emit_val(CG_NUM_LABEL, comp_label);
    emit_str(CG_SET_A16_AS_LOCAL_PTR, 0, sym->name);
    emit_val(CG_LOAD_16, 0);
    emit_val(CG_PUSH_A16, 0);
    expr(sym->datatype);
    emit_val(CG_POP_B16, 0);
    emit_val(CG_SET_IF_LE_16, 0);
    emit_val(CG_JMP_IF_FALSE, _break_label);
    emit_val(CG_JMP, contents_label);

    emit_val(CG_NUM_LABEL, _continue_label);

    if(curr_is("step"))
    {
        next();
        expr(sym->datatype);
        emit_val(CG_PUSH_A16, 0);
        emit_str(CG_SET_A16_AS_LOCAL_PTR, 0, sym->name);
        emit_val(CG_LOAD_16, 0);
        emit_val(CG_POP_B16, 0);
        emit_val(CG_ADD_16, 0);
        emit_val(CG_COPY_A16_TO_B16, 0);
        emit_str(CG_SET_A16_AS_LOCAL_PTR, 0, sym->name);
        emit_val(CG_STORE_16, 0);
    }
    else
    {
        emit_str(CG_SET_A16_AS_LOCAL_PTR, 0, sym->name);
        emit_val(CG_LOAD_16, 0);
        emit_val(CG_SET_A16, 1);
        emit_val(CG_ADD_16, 0);
        emit_val(CG_COPY_A16_TO_B16, 0);
        emit_str(CG_SET_A16_AS_LOCAL_PTR, 0, sym->name);
        emit_val(CG_STORE_16, 0);
    }

    emit_val(CG_JMP, comp_label);
    emit_val(CG_NUM_LABEL, contents_label);

    function_level();


    emit_val(CG_JMP, _continue_label);
    emit_val(CG_NUM_LABEL, _break_label);

    
    _continue_label = old_continue;
    _break_label = old_break;
}

void function_level()
{
    if(curr_is("var"))
    {
        do_var(false);
    }
    else if(curr_is("return"))
    {
        do_return();
    }
    else if(curr_is("while"))
    {
        do_while();
        return;
    }
    else if(curr_is("for"))
    {
        do_for();
        return;
    }
    else if(curr_is("if"))
    {
        do_if();
        return;
    }
    else if(curr_is("do"))
    {
        next();
        while(!curr_is("end"))
        {
            function_level();
        }
        next();
        return;
    }
    else expr(0);
    if(!curr_is(";")) error("';' expected.");
    next();
}

void root_level()
{
    if(curr_is("var"))
    {
        do_var(true);
    }
    else if(curr_is_symbol())
    {
        do_function();
        return;
    }
    else error("invalid command: %s", curr_contents());
    if(!curr_is(";")) error("';' expected.");
    next();
}

void process(char * fname)
{
    infile_t * old_in = _in;
    _in = new(sizeof(infile_t));
    _in->file = fopen(fname, "r");
    _in->column = 0;
    _in->line = 1;
    strncpy(_in->name, fname, TOKEN_LEN);
    if(_in->file == NULL) error("file not found");
    _in->is_eof = false;
    c_next();
    next();
    next();
    while(!curr_is_eof())
        root_level();
}

int main(int argc, char ** argv)
{
    if(argc != 3)
    {
        printf("Usage: tlang [source] [tokens]\n");
        return 1;
    }
    _out = new(sizeof(outfile_t));
    _out->file = fopen(argv[2], "wb");
    if(_out->file == 0) error("cannot create file: %s", argv[2]);
    process(argv[1]);
    fclose(_out->file);
    return 0;
}