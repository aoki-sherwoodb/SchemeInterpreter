#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "talloc.h"
#include "linkedlist.h"
#include "parser.h"

void syntaxError(const char *errorMessage) {
    printf("%s", errorMessage);
    texit(1);
}

Value *updateTree(Value *tree, int *depth, Value *token) {
    if (token->type != CLOSE_TYPE) {
        Value *temp = cons(token, tree);
        tree = temp;
        if (token->type == OPEN_TYPE) {
            *depth = *depth + 1;
        }
    } else {
        if (tree->type == NULL_TYPE || *depth < 1) {
            syntaxError("Syntax error: too many close parentheses");
        }
        *depth = *depth - 1;
        Value *subtree = makeNull();
        while (car(tree)->type != OPEN_TYPE) {
            Value *temp = cons(car(tree), subtree);
            subtree = temp;
            tree = cdr(tree);
            if (tree->type == NULL_TYPE) {
                syntaxError("Syntax error: too many close parentheses");
            }
        }
        tree->c.car = subtree;
    }
    return tree;
}

// Takes a list of tokens from a Racket program, and returns a pointer to a
// parse tree representing that program.
Value *parse(Value *tokens) {
    Value *tree = makeNull();
    int depth = 0;
    Value *curNode = tokens;
    assert(curNode != NULL && "Parse error: null token list");
    while (curNode->type != NULL_TYPE) {
        tree = updateTree(tree, &depth, car(curNode));
        Value *next = cdr(curNode);
        curNode = next;
    }
    if (depth != 0) {
        syntaxError("Syntax error: not enough close parentheses");
    }
    return reverse(tree);
}


void printToken(Value *token) {
    if (token->type == SYMBOL_TYPE || token->type == STR_TYPE) {
        printf("%s", token->s);
    } else if (token->type == INT_TYPE) {
        printf("%i", token->i);
    } else if (token->type == DOUBLE_TYPE) {
        printf("%f", token->d);
    } else if (token->type == BOOL_TYPE) {
        int intBool = token->i;
        if (intBool == 0) {
            printf("#f");
        } else {
            printf("#t");
        }
    } else if (token->type ==  SINGLEQUOTE_TYPE) {
        printf("quote");
    } else if (token->type == DOT_TYPE) {
        printf(".");
    } else if (token->type == NULL_TYPE) {
        printf("()");
    }
}

void printSExpr(Value *tree) {
    if (tree->type != CONS_TYPE) {
        printToken(tree);
    } else {
        printf("(");
        while (tree->type != NULL_TYPE) {
            if (car(tree)->type == CONS_TYPE) {
                printSExpr(car(tree));
            } else {
                printToken(car(tree));
            }
            //add whitespace if not last token in expression
            if (cdr(tree)->type != NULL_TYPE) {
                printf(" ");
            }
            Value *next = cdr(tree);
            tree = next;
        }
        printf(")");
    }
}

// Prints the tree to the screen in a readable fashion. It should look just like
// Racket code; use parentheses to indicate subtrees.
void printTree(Value *tree) {
    while (tree->type != NULL_TYPE) {
        printSExpr(car(tree));
        tree = cdr(tree);
        printf("\n");
    }
}

// int main() {
//     Value *open = talloc(sizeof(Value));
//     open->type = OPEN_TYPE;
//     Value *close = talloc(sizeof(Value));
//     close->type = CLOSE_TYPE;
//     Value *temp = cons(close, makeNull());
//     Value *head = cons(open, temp);
//     Value *parseTree = parse(head);
//     printTree(parseTree);
//     texit(0);
// }