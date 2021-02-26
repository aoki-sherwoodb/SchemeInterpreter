#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "value.h"
#include "interpreter.h"
#include "parser.h"
#include "tokenizer.h"
#include "linkedlist.h"
#include "talloc.h"

void printValue(Value *item);

void evalError(char* errorMessage) {
    printf("%s:%s", "Evaluation error", errorMessage);
    texit(1);
}

void printList(Value *tree) {
    printf("(");
    while (tree->type != NULL_TYPE) {
        if (car(tree)->type == CONS_TYPE) {
            printList(car(tree));
        } else {
            printValue(car(tree));
        }
        //add whitespace if not last token in expression
        if (cdr(tree)->type != NULL_TYPE) {
            printf(" ");
        }
        Value *next = cdr(tree);
        tree = next;
    }
    printf(") ");
}

void printValue(Value *item) {
    if (item->type == BOOL_TYPE) {
        if (item->i == 0) {
            printf("#f\n");
        } else {
            printf("#t\n");
        }
    } else if (item->type == INT_TYPE) {
        printf("%i\n", item->i);
    } else if (item->type == DOUBLE_TYPE) {
        printf("%f\n", item->d);
    } else if (item->type == STR_TYPE || item->type == SYMBOL_TYPE) {
        printf("%s\n", item->s);
    } else if (item->type == CONS_TYPE) {
        printList(item);
    } else if (item->type == NULL_TYPE) {
        printf("()");
    }
}

//Interprets each top level S-expression in the tree
//and prints out the results.
void interpret(Value *tree) {
    while (tree->type != NULL_TYPE) {
        Value *expr = car(tree);
        Frame *globalFrame = talloc(sizeof(Frame));
        globalFrame->bindings = makeNull();
        globalFrame->parent = NULL;
        printValue(eval(expr, globalFrame));
        Value *next = cdr(tree);
        tree = next;
    }
}

/* store bindings as cons cells with the car as the variable and the 
cdr as the value. Returns the value of a bound variable, if applicable */
Value *getBoundValue(Value *symbol, Frame *frame) {
    while (frame != NULL) {
        Value *binding = frame->bindings;
        while (binding->type != NULL_TYPE) {
            if (!strcmp(car(car(binding))->s, symbol->s)) {
                //binding found
                return cdr(car(binding));
            } else {
                binding = cdr(binding);
            }
        }
        frame = frame->parent;
    }
    evalError("reference to unbound variable");
    return NULL;
}

Value *evalIf(Value *ifArgs, Frame *frame) {
    //check that if has exactly 3 arguments
    Value *argCheck = ifArgs;
    for (int i = 0; i < 2; i++) {
        if (argCheck->type == NULL_TYPE) {
            evalError("wrong number of arguments for if");
        }
        Value *next = cdr(argCheck);
        argCheck = next;
    }
    if (cdr(argCheck)->type != NULL_TYPE) {
        evalError("wrong number of arguments for if");
    }

    Value *condition = eval(car(ifArgs), frame);
    if (condition->type != BOOL_TYPE) {
        evalError("non-boolean condition for if");
    }
    if (condition->i) {
        return eval(car(cdr(ifArgs)), frame);
    } else {
        return eval(car(cdr(cdr(ifArgs))), frame);
    }
}

int isAlreadyBound(Value *variable, Frame *letFrame){
    Value *bindings = letFrame->bindings;
    int isBound = 0;

    while(bindings->type != NULL_TYPE){
        if(!strcmp(car(car(bindings))->s, variable->s)){
            isBound = 1;
            break;
        }
        bindings = cdr(bindings);
    }
    return isBound;
}

Value *evalLet(Value *letArgs, Frame *frame) {
    Value *bindingList = car(letArgs);
    if (bindingList->type != CONS_TYPE && bindingList->type != NULL_TYPE) {
        evalError("improper variable binding format in let");
    }
    //create new frame to hold let bindings
    Frame *letFrame = talloc(sizeof(Frame));
    letFrame->parent = frame;
    letFrame->bindings = makeNull();
    //set bindings
    while (bindingList->type != NULL_TYPE) {
        Value *binding = car(bindingList);
        if (binding->type != CONS_TYPE) {
            evalError("improper variable binding format in let");
        } else if (cdr(binding)->type == NULL_TYPE) {
            evalError("no value to bind variable to in let");
        } else if (cdr(cdr(binding))->type != NULL_TYPE) {
            evalError("too many items in variable binding in let");
        }
        Value *variable = car(binding);
        if (variable->type != SYMBOL_TYPE) {
            evalError("improper variable type for binding in let");
        } else if (isAlreadyBound(variable, letFrame)) {
            evalError("cannot bind the same variable multiple times in let");
        }
        Value *boundValue = eval(car(cdr(binding)), frame);
        Value *newBinding = cons(variable, boundValue);
        Value *temp = cons(newBinding, letFrame->bindings);
        letFrame->bindings = temp;
        bindingList = cdr(bindingList);
    }
    //eval let body
    Value *letBody = cdr(letArgs);
    if (letBody->type == NULL_TYPE) {
        //empty body of let
        evalError("no body in let");
        return NULL;
    } else {
        while (cdr(letBody)->type != NULL_TYPE) {
            eval(car(letBody), letFrame);
            letBody = cdr(letBody);
        }
        return eval(car(letBody), letFrame);
    }
}

//Evaluates the S-expression referred to by expr
//in the given frame.
Value *eval(Value *tree, Frame *frame) {
    valueType type = tree->type;
    if (type == INT_TYPE || type == DOUBLE_TYPE || 
        type == BOOL_TYPE || type == STR_TYPE) {
        //atomic expressions
        return tree;
    } else if (type == SYMBOL_TYPE) {
        //variables
        return getBoundValue(tree, frame);
    } else if (type == CONS_TYPE) {
        //special forms
        Value *first = car(tree);
        Value *args = cdr(tree);
        if (args->type == NULL_TYPE) {
            evalError("no arguments passed to special form");
        }
        Value *result;
    
        if (!strcmp(first->s,"if")) {
            result = evalIf(args, frame);
        } else if (!strcmp(first->s, "let")) {
            result = evalLet(args, frame);
        } else if (!strcmp(first->s, "quote")) {
            if (cdr(args)->type != NULL_TYPE) {
                /*test-e doesn't allow any messages after 
                'Evaluation error'
                evalError("quote has more than 1 argument");*/
                printf("Evaluation error");
                texit(1);
            }
            result = car(args);
        } else {
            evalError("not a recognized special form");
        }
        return result;
    }
    return NULL;    
}