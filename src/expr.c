/*

    casm - Simple, portable assembler

    Copyright (C) 2003-2015  Ian Cowburn (ianc@noddybox.co.uk)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    -------------------------------------------------------------------------

    Expands an expression.

*/
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "global.h"
#include "expr.h"
#include "label.h"
#include "state.h"
#include "util.h"

/* ---------------------------------------- MACROS
*/
#define TYPE_OPERAND    0
#define TYPE_LABEL      1
#define TYPE_OPERATOR   2       /* This acts as a base for operator tokens */
#define TYPE_LPAREN     3
#define TYPE_RPAREN     4
#define TYPE_DIVIDE     5
#define TYPE_MULTIPLY   6
#define TYPE_ADD        7
#define TYPE_SUBTRACT   8
#define TYPE_NOT        9
#define TYPE_AND        10
#define TYPE_OR         11
#define TYPE_XOR        12
#define TYPE_UNARY_PLUS 13
#define TYPE_UNARY_NEG  14
#define TYPE_SHIFTL     15
#define TYPE_SHIFTR     16
#define TYPE_EQUALITY   17
#define TYPE_BOOL_AND   18
#define TYPE_BOOL_OR    19
#define TYPE_LT         21
#define TYPE_GT         22
#define TYPE_LTEQ       23
#define TYPE_GTEQ       24
#define TYPE_INEQUALITY 25

#define SYM_LPAREN      "{"
#define SYM_RPAREN      "}"
#define SYM_DIVIDE      "/"
#define SYM_MULTIPLY    "*"
#define SYM_ADD         "+"
#define SYM_SUBTRACT    "-"
#define SYM_NOT         "~"
#define SYM_AND         "&"
#define SYM_OR          "|"
#define SYM_XOR         "^"
#define SYM_UNARY_PLUS  "+"
#define SYM_UNARY_NEG   "-"
#define SYM_SHIFTL      "<<"
#define SYM_SHIFTR      ">>"
#define SYM_EQUALITY    "=="
#define SYM_BOOL_AND    "&&"
#define SYM_BOOL_OR     "||"
#define SYM_LT          "<"
#define SYM_GT          ">"
#define SYM_LTEQ        "<="
#define SYM_GTEQ        ">="
#define SYM_INEQUALITY  "!="

#define OPERATOR_CHARS  "{}/*+-~&|^<>=!"

#define IS_OPERATOR_TYPE(a)     ((a)>=TYPE_OPERATOR)

#define IS_PAREN(c)             ((c)==SYM_LPAREN[0] || (c)==SYM_RPAREN[0])

#define IS_OPERATOR(c)          (strchr(OPERATOR_CHARS,(c)))

#define IS_OPERAND_END(c)       ((c)==' ' || (c)=='\t' || IS_OPERATOR(c))

/* ---------------------------------------- TYPES
*/

typedef struct stk
    {
    int         token;
    int         priority;
    int         is_unary;
    char        *text;
    struct stk  *next;
    } Stack;


typedef enum {OpBinary, OpPretendUnary, OpUnary} OpType;

typedef struct
    {
    const char  *symbol;
    int         priority;
    int         token;
    OpType      type;
    int         allow_unary;
    } Operator;


typedef enum {OpOk, OpSyntaxError, OpUnknown} OpError;


/* ---------------------------------------- GLOBALS
*/
static char             error[1024];

static const Operator   op_info[]=
    {
        /* Unary ops - must be first in list.  Note that LPAREN is treated
           as a unary as it must be preceeded by another operator, rather
           than an operand.
        */
        {SYM_NOT,       9,      TYPE_NOT,       OpUnary,        TRUE},
        {SYM_UNARY_PLUS,9,      TYPE_UNARY_PLUS,OpUnary,        TRUE},
        {SYM_UNARY_NEG, 9,      TYPE_UNARY_NEG, OpUnary,        TRUE},
        {SYM_LPAREN,    99,     TYPE_LPAREN,    OpPretendUnary, TRUE},

        /* Binary ops and don't care ops
        */
        {SYM_RPAREN,    99,     TYPE_RPAREN,    OpBinary,       FALSE},
        {SYM_SHIFTL,    6,      TYPE_SHIFTL,    OpBinary,       TRUE},
        {SYM_SHIFTR,    6,      TYPE_SHIFTR,    OpBinary,       TRUE},
        {SYM_MULTIPLY,  5,      TYPE_MULTIPLY,  OpBinary,       TRUE},
        {SYM_DIVIDE,    5,      TYPE_DIVIDE,    OpBinary,       TRUE},
        {SYM_ADD,       4,      TYPE_ADD,       OpBinary,       TRUE},
        {SYM_SUBTRACT,  4,      TYPE_SUBTRACT,  OpBinary,       TRUE},
        {SYM_EQUALITY,  1,      TYPE_EQUALITY,  OpBinary,       TRUE},
        {SYM_INEQUALITY,1,      TYPE_INEQUALITY,OpBinary,       TRUE},
        {SYM_LTEQ,      1,      TYPE_LTEQ,      OpBinary,       TRUE},
        {SYM_GTEQ,      1,      TYPE_GTEQ,      OpBinary,       TRUE},
        {SYM_LT,        1,      TYPE_LT,        OpBinary,       TRUE},
        {SYM_GT,        1,      TYPE_GT,        OpBinary,       TRUE},
        {SYM_BOOL_AND,  0,      TYPE_BOOL_AND,  OpBinary,       TRUE},
        {SYM_AND,       0,      TYPE_AND,       OpBinary,       TRUE},
        {SYM_BOOL_OR,   0,      TYPE_BOOL_OR,   OpBinary,       TRUE},
        {SYM_OR,        0,      TYPE_OR,        OpBinary,       TRUE},
        {SYM_XOR,       0,      TYPE_XOR,       OpBinary,       TRUE},

        {0,0,0}
    };


/* ---------------------------------------- PRIVATE FUNCTIONS
*/

/* Empty a stack
*/
static Stack *ClearStack(Stack *stack)
{
    while(stack)
    {
        Stack *tmp=stack;

        stack=stack->next;
        free(tmp->text);
        free(tmp);
    }

    return stack;
}


/* Push onto the stack
*/
static Stack *Push(Stack *stack, int token, int priority, 
                   int is_unary, const char *text)
{
    Stack *e=Malloc(sizeof *e);

    e->text=DupStr(text);
    e->priority=priority;
    e->token=token;
    e->is_unary=is_unary;
    e->next=stack;
    stack=e;

    return stack;
}


/* Destroy the top element on the stack
*/
static Stack *Pop(Stack *stack)
{
    if (stack)
    {
        Stack *tmp=stack;

        stack=stack->next;

        free(tmp->text);
        free(tmp);
    }

    return stack;
}


/* Debug to dump a stack non-destructively
*/
static void Dump(const char *name, Stack *stack)
{
    while (stack)
    {
        printf("%-10s: Type: %-2d  is_unary: %d  text: %s\n",
                        name,stack->token,stack->is_unary,stack->text);

        stack=stack->next;
    }
}


/* Find the symbol for an operator
*/
static const char *ToString(int token)
{
    static char buff[32];
    int f;

    strcpy(buff,"UNKNOWN");

    for(f=0;op_info[f].symbol;f++)
    {
        if (op_info[f].token==token)
        {
            strcpy(buff,op_info[f].symbol);
            return buff;
        }
    }

    return "UNKNOWN";
}


/* Search the operator info
*/
static const OpError FindOp(const char *p, int prev_was_op, const Operator **op)
{
    int f;
    int found=FALSE;

    *op=NULL;

    for(f=0;op_info[f].symbol;f++)
    {
        if (strncmp(op_info[f].symbol,p,strlen(op_info[f].symbol))==0)
        {
            found=TRUE;

            if ((prev_was_op && op_info[f].type!=OpBinary) ||
                (!prev_was_op && op_info[f].type==OpBinary))
                
            {
                *op=op_info+f;
                return OpOk;
            }
        }
    }

    if (found)
        return OpSyntaxError;
    else
        return OpUnknown;
}


static Stack *ToPostfix(const char *expr)
{
    Stack *stack=NULL;
    Stack *output=NULL;
    char buff[1024];
    char *p;
    int prev_was_op=TRUE;

    /* Parse the infix expression into postfix
    */
    while(*expr)
    {
        if (IS_OPERATOR(*expr))
        {
            /* Found an operator - parse it
            */
            const Operator *op;
            OpError op_err;
            
            op_err=FindOp(expr,prev_was_op,&op);

            /* Unknown operator in expression
            */
            if (!op)
            {
                if (op_err==OpSyntaxError)
                    sprintf(error,"Syntax error with operator %c",*expr);
                else
                    sprintf(error,"Unknown operator %c",*expr);

                stack=ClearStack(stack);
                output=ClearStack(output);

                return NULL;
            }

            /* Special handling for closing parenthesis
            */
            if (op->token==TYPE_RPAREN)
            {
                while(stack && stack->token!=TYPE_LPAREN)
                {
                    output=Push(output,
                                stack->token,
                                stack->priority,
                                stack->is_unary,
                                stack->text);
                    stack=Pop(stack);
                }

                /* Syntax error in expression
                */
                if (!stack)
                {
                    sprintf(error,"Missing %s", SYM_LPAREN);

                    stack=ClearStack(stack);
                    output=ClearStack(output);

                    return NULL;
                }

                /* Remove the opening bracket and continue
                */
                stack=Pop(stack);
            }
            else
            {
                /* Output the stack until an operator of lower precedence is
                   found
                */
                if (op->type==OpUnary)
                {
                    while(stack && !IS_PAREN(stack->text[0]) &&
                            !stack->is_unary &&
                                (stack->priority >= op->priority))
                    {
                        output=Push(output,
                                    stack->token,
                                    stack->priority,
                                    stack->is_unary,
                                    stack->text);

                        stack=Pop(stack);
                    }
                }
                else
                {
                    while(stack && !IS_PAREN(stack->text[0]) &&
                                (stack->priority >= op->priority))
                    {
                        output=Push(output,
                                    stack->token,
                                    stack->priority,
                                    stack->is_unary,
                                    stack->text);

                        stack=Pop(stack);
                    }
                }

                stack=Push(stack,
                           op->token,
                           op->priority,
                           op->type==OpUnary,
                           op->symbol);
            }

            /* Move past token
            */
            expr+=strlen(op->symbol);

            prev_was_op=op->allow_unary;
        }
        else
        {
            /* Found an operand - parse and store the operand
            */
            p=buff;

            while(*expr && !IS_OPERAND_END(*expr))
            {
                *p++=*expr++;
            }

            *p=0;

            /* Note that operands are marked as unary just to chose one over
               the other branch in EvalPostfix() - no other reason
            */
            output=Push(output,TYPE_OPERAND,0,TRUE,buff);

            prev_was_op=FALSE;
        }

        while(isspace((unsigned char)*expr))
        {
            expr++;
        }
    }

    while(stack)
    {
        output=Push(output,
                    stack->token,
                    stack->priority,
                    stack->is_unary,
                    stack->text);

        stack=Pop(stack);
    }

    /* Return the stack
    */
    return output;
}


static int EvalPostfix(Stack **stack, long *result)
{
    Stack *expr;
    int token;

    expr=*stack;

    if (!expr)
    {
        sprintf(error,"Called with empty postfix stack");
        return FALSE;
    }

    token=expr->token;

    if (expr->is_unary)
    {
        if (token==TYPE_OPERAND)
        {
            if (!LabelExpand(expr->text, result) && IsFinalPass())
            {
                sprintf(error,"Invalid value '%s'",expr->text);
                return FALSE;
            }

            expr = Pop(expr);
        }
        else if (token==TYPE_LABEL)
        {
            const Label *label = NULL;

            if (IsFinalPass())
            {
                if (!(label = LabelFind(expr->text, ANY_LABEL)))
                {
                    sprintf(error,"Undefined label '%s'",expr->text);
                    return FALSE;
                }
            }

            *result = (label == NULL ? 0 : label->value);
            expr = Pop(expr);
        }
        else
        {
            long val;

            expr=Pop(expr);

            if (!expr || !EvalPostfix(&expr,&val))
            {
                sprintf(error,"Operator '%s' expects an argument",
                                ToString(token));
                *stack=expr;
                return FALSE;
            }

            switch(token)
            {
                case TYPE_NOT:
                    *result=~val;
                    break;

                case TYPE_UNARY_PLUS:
                    *result=+val;
                    break;

                case TYPE_UNARY_NEG:
                    *result=-val;
                    break;

                default:
                    sprintf(error,"Execpected unary token '%s' processed",
                                        ToString(token));
                    *stack=expr;
                    return FALSE;
                    break;
            }
        }
    }
    else
    {
        long left,right;

        expr=Pop(expr);

        if (!expr || !EvalPostfix(&expr,&right))
        {
            sprintf(error,"Operator '%s' expects two "
                                "arguments (unknown label?)",
                                ToString(token));
            *stack=expr;
            return FALSE;
        }

        if (!expr || !EvalPostfix(&expr,&left))
        {
            sprintf(error,"Operator '%s' expects two "
                                "arguments (unknown label?)",
                                ToString(token));
            *stack=expr;
            return FALSE;
        }

        switch(token)
        {
            case TYPE_DIVIDE:
                *result=left/right;
                break;

            case TYPE_MULTIPLY:
                *result=left*right;
                break;

            case TYPE_ADD:
                *result=left+right;
                break;

            case TYPE_SUBTRACT:
                *result=left-right;
                break;

            case TYPE_AND:
                *result=left&right;
                break;

            case TYPE_OR:
                *result=left|right;
                break;

            case TYPE_BOOL_AND:
                *result=left&&right;
                break;

            case TYPE_BOOL_OR:
                *result=left||right;
                break;

            case TYPE_XOR:
                *result=left^right;
                break;

            case TYPE_SHIFTL:
                if (right<0)
                {
                    sprintf(error,"Cannot shift left by a "
                                        "negative number (%ld)",right);
                    *stack=expr;
                    return FALSE;
                }

                *result=left<<right;
                break;

            case TYPE_SHIFTR:
                if (right<0)
                {
                    sprintf(error,"Cannot shift right by a "
                                        "negative number (%ld)",right);
                    *stack=expr;
                    return FALSE;
                }

                *result=left>>right;
                break;

            case TYPE_EQUALITY:
                *result=left==right;
                break;

            case TYPE_INEQUALITY:
                *result=left!=right;
                break;

            case TYPE_LT:
                *result=left<right;
                break;

            case TYPE_GT:
                *result=left>right;
                break;

            case TYPE_LTEQ:
                *result=left<=right;
                break;

            case TYPE_GTEQ:
                *result=left>=right;
                break;

            default:
                sprintf(error,"Unexpected binary token '%s' processed",
                                ToString(token));
                *stack=expr;
                return FALSE;
                break;
        }
    }

    *stack=expr;

    return TRUE;
}


/* ---------------------------------------- INTERFACES
*/

int ExprConvert(int no_bits, long value)
{
    if (value<0)
    {
        value=-value;
        value--;
        value=~value;
    }

    return value & ((1<<no_bits)-1);
}


int ExprEval(const char *expr, long *result)
{
    Stack *output=ToPostfix(expr);
    int ret;

    if (!output)
        return FALSE;

    ret=EvalPostfix(&output,result);

    ClearStack(output);

    return ret;
}


/* Gets a readable reason for an error from ExprEval() or ExprParse.
*/
const char *ExprError(void)
{
    return error;
}

/*
vim: ai sw=4 ts=8 expandtab
*/
