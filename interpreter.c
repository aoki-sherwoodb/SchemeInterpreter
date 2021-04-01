/* The evaluation stage of a Scheme interpreter in C. Takes in parsed input from Scheme code. */
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
    printf("%s: %s", "Evaluation error", errorMessage);
    texit(1);
}

void printList(Value *tree) {
    printf("(");
    while (tree->type != NULL_TYPE) {
        if (car(tree)->type == CONS_TYPE) {
            printList(car(tree));
        } else {
            printValue(car(tree));
            if (cdr(tree)->type != CONS_TYPE && 
                cdr(tree)->type != NULL_TYPE) {
                printf(" . ");
                printValue(cdr(tree));
                break;
            }
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
        printf("()\n");
    } else if (item->type == CLOSURE_TYPE) {
        printf("#<procedure>\n");
    }
}

/*
 * Adds a binding between the given name
 * and the input function. Used to add
 * bindings for primitive funtions to the top-level
 * bindings list.
 */
void bindFn(char *name, Value *(*function)(Value *), Frame *frame) {
	Value *funcName = makeNull();
	funcName->type = SYMBOL_TYPE;
	funcName->s = name;
    Value *primitiveFunction = makeNull();
    primitiveFunction->type = PRIMITIVE_TYPE;
    primitiveFunction->primFn = function;
    Value *binding = cons(funcName, primitiveFunction);
	Value *temp = cons(binding, frame->bindings);
	frame->bindings = temp;
    return;
}

double applyOperation(char operation, double num1, double num2) {
	double result;
	if (operation == '+'){
		result = num1 + num2;
	} else if (operation == '-'){
		result = num1 - num2;
	} else if (operation == '*'){
		result = num1 * num2;
	} else if (operation == '/'){
		result = num1 / num2;
	}
	return result;
}

Value *helpArithmetic(Value *args, char operation) {
    Value *result = makeNull();
    result->type = INT_TYPE;
    result->i = 0;
    if (operation == '*' || operation == '/') {
        result->i = 1;
    }
    if (operation == '/' || operation == '-') {
        int startFrom = result->i;
        if (car(args)->type == DOUBLE_TYPE) {
            result->type = DOUBLE_TYPE;
            result->d = car(args)->d;
        } else {
            result->i = car(args)->i;
        }
        if (length(args) == 1) {
            //if only 1 argument is provided, - starts from 0 and 
            // / starts from 1
            if (result->type == DOUBLE_TYPE) {
                result->type = DOUBLE_TYPE;
                result->d = applyOperation(operation, startFrom, result->d);
            } else if (result->type == INT_TYPE) {
                result->i = applyOperation(operation, startFrom, result->i);
            }
            return result;
        } else {
            //starting value for subtraction or division is set; //advance to next arg
            args = cdr(args);
        }
	}

	while (args->type != NULL_TYPE) {
        Value *number = car(args);
        if (result->type == INT_TYPE) {
            if (number->type == INT_TYPE) {
                double operatorApplication = 
                    applyOperation(operation, result->i, number->i);
                if (operation == '/' && result->i % number->i != 0) {
                    //result becomes a decimal if any decimal is incorporated into it with any operator
                    result->type = DOUBLE_TYPE;
                    result->d = operatorApplication;
                } else {
                    result->i = operatorApplication;
                }
            } else if (number->type == DOUBLE_TYPE) {
                double operatorApplication =
                    applyOperation(operation, result->i, number->d);
                result->type = DOUBLE_TYPE;
                result->d = operatorApplication;
            } else {
                evalError("wrong argument type for arithmetic function");
            }
        } else {
            if (number->type == INT_TYPE) {
                result->d = 
                    applyOperation(operation, result->d, number->i);
            } else if (number->type == DOUBLE_TYPE) {
                result->d = applyOperation(operation, result->d, number->d);
            } else {
                evalError("wrong argument type for arithmetic function");
            }
        } 
        args = cdr(args);
    }
    return result;
}

Value *primitiveAdd(Value *args) {
   	return helpArithmetic(args, '+');
}

Value *primitiveSubtract(Value *args) {
	if (length(args) == 0) {
		evalError("wrong number of args for -");
	}
    return helpArithmetic(args, '-');
}

Value *primitiveMultiply(Value *args) {
    return helpArithmetic(args, '*');
}

Value *primitiveDivide(Value *args) {
    if (length(args) == 0) {
        evalError("wrong number of args for /");
    }
    return helpArithmetic(args, '/');
}

Value *primitiveMod(Value *args) {
    if (length(args) != 2) {
        evalError("wrong number of args for modulo");
    } else if (car(args)->type != INT_TYPE ||
               car(cdr(args))->type != INT_TYPE) {
        evalError("wrong argument type in modulo");
    } 
    Value *result = makeNull();
    result->type = INT_TYPE;
    result->i = car(args)->i % car(cdr(args))->i;
	return result;
}

Value *primitiveCar(Value *args) {
    if (length(args) != 1) {
        evalError("wrong number of args for car");
    } else if (car(args)->type != CONS_TYPE) {
        evalError("car applied to non-cons type");
    }
	return car(car(args));
}

Value *primitiveCdr(Value *args) {
    if (length(args) != 1) {
        evalError("wrong number of args for cdr");
    } else if (car(args)->type != CONS_TYPE) {
        evalError("cdr applied to non-cons type");
    }
	return cdr(car(args));
}

Value *primitiveCons(Value *args) {
	if (length(args) != 2) {
		evalError("wrong number of args for cons");
	}
	Value *consCell = cons(car(args), car(cdr(args)));
    return consCell;
}

Value *primitiveNull(Value *args) {
	if(length(args) != 1){
		evalError("wrong number of args for null?");
	}
    Value *boolean = makeNull();
    boolean->type = BOOL_TYPE;
    if (car(args)->type == NULL_TYPE) {
        boolean->i = 1;
    } else {
        boolean->i = 0;
    }
    return boolean;
}

/*
* Converts all numbers to doubles and checks that args are doubles for
* use in the primitive =, >, and < function.
*/
double argToDouble(Value *arg){
	double out;
	if(arg->type == INT_TYPE){
		out = arg->i;
	} else if (arg->type == DOUBLE_TYPE){
		out = arg->d;
	} else {
		evalError("wrong arg type for number comparison");
	}
	return out;
}

Value *primitiveEqual(Value *args){
	if (length(args) != 2) {
		evalError("wrong number of args for =");
	}
	double arg1 = argToDouble(car(args));
	double arg2 = argToDouble(car(cdr(args)));
    Value *boolean = makeNull();
	boolean->type = BOOL_TYPE;
	if (arg1 == arg2) {
		boolean->i = 1;
	} else {
		boolean->i = 0;
	}
	return boolean;
}

Value *primitiveGreater(Value *args){
	if (length(args) != 2) {
		evalError("wrong number of args for >");
	}
	double arg1 = argToDouble(car(args));
	double arg2 = argToDouble(car(cdr(args)));
    Value *boolean = makeNull();
	boolean->type = BOOL_TYPE;
	if (arg1 > arg2) {
		boolean->i = 1;
	} else {
		boolean->i = 0;
	}
	return boolean; 
}

Value *primitiveLess(Value *args){
	if (length(args) != 2) {
		evalError("wrong number of args for <");
	}
	double arg1 = argToDouble(car(args));
	double arg2 = argToDouble(car(cdr(args)));
    Value *boolean = makeNull();
	boolean->type = BOOL_TYPE;
	if (arg1 < arg2) {
		boolean->i = 1;
	} else {
		boolean->i = 0;
	}
	return boolean;
}

void bindPrimitives(Frame *frame) {
    bindFn("+", primitiveAdd, frame);
	bindFn("car", primitiveCar, frame);
	bindFn("cdr", primitiveCdr, frame);
	bindFn("cons", primitiveCons, frame);
    bindFn("null?", primitiveNull, frame);
	bindFn("=", primitiveEqual, frame);
    bindFn("<", primitiveLess, frame);
	bindFn(">", primitiveGreater, frame);
	bindFn("-", primitiveSubtract, frame);
	bindFn("*", primitiveMultiply, frame);
    bindFn("/", primitiveDivide, frame);
    bindFn("modulo", primitiveMod, frame);
    return;
}

/*
* Interprets each top level S-expression in the tree
* and prints out the results.
*/
void interpret(Value *tree) {
    Frame *globalFrame = talloc(sizeof(Frame));
    globalFrame->bindings = makeNull();
    globalFrame->parent = NULL;
	bindPrimitives(globalFrame);
    while (tree->type != NULL_TYPE) {
        Value *expr = car(tree);
        printValue(eval(expr, globalFrame));
        Value *next = cdr(tree);
        tree = next;
    }
}

/* 
* Bindings are cons cells with the car as the variable and the 
* cdr as the value. Returns the value of a bound variable, if        * applicable. 
*/
Value *getBoundValue(char *symbol, Frame *frame) {
    while (frame != NULL) {
        Value *binding = frame->bindings;
        while (binding->type != NULL_TYPE) {
            if (!strcmp(car(car(binding))->s, symbol)) {
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
    Value *argCheck = ifArgs;
    if (length(ifArgs) != 3) {
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

/* In a list of value, if item is present, returns 1. If not, returns * 0. Allowed value types are STR_TYPE, SYMBOL_TYPE, INT_TYPE,        * DOUBLE_TYPE and BOOL_TYPE. Supports standard linked lists and also * binding format lists. 
*/
int contains(Value *list, Value* item) {
    while (list->type != NULL_TYPE) {
        Value *cur = car(list);
        if (cur->type == CONS_TYPE) {
            //binding list structure is different
            cur = car(cur);
        }
        if (cur->type == item->type) {
            if (cur->type == STR_TYPE || cur->type == SYMBOL_TYPE) {
                if (!strcmp(cur->s, item->s)) {
                    return 1;
                }
            } else if (cur->type == INT_TYPE 
                    || cur->type == BOOL_TYPE) {
                if (cur->i == item->i) {
                    return 1;
                }
            } else if (cur->type == DOUBLE_TYPE) {
                if (cur->d == item->d) {
                    return 1;
                }
            }
        } 
        list = cdr(list);
    }
    return 0;
}

void bindLetArg(Value *bindingList, Frame *frame, Frame *letFrame) {
	Value *binding = car(bindingList);
    if (binding->type != CONS_TYPE) {
        evalError("improper variable binding format in let");
    } else if (length(binding) != 2) {
        evalError("wrong number of items in let binding");
	}
    Value *variable = car(binding);
    if (variable->type != SYMBOL_TYPE) {
        evalError("improper variable type for binding in let");
    } else if (contains(letFrame->bindings, variable)) {
        evalError("duplicate bound variable in let");
    }
    Value *expression = eval(car(cdr(binding)), frame);
    Value *newBinding = cons(variable, expression);
    Value *temp = cons(newBinding, letFrame->bindings);
    letFrame->bindings = temp;
}

Value *evalLetBody(Value *letBody, Frame *letFrame) {
    if (letBody->type == NULL_TYPE) {
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

Value *evalLet(Value *letArgs, Frame *frame) {
    Value *bindingList;
    if (letArgs->type == CONS_TYPE) {
        bindingList = car(letArgs);
    } else if (letArgs->type == NULL_TYPE) {
        bindingList = makeNull();
    }
    if (bindingList->type != CONS_TYPE && bindingList->type != NULL_TYPE) {
        evalError("improper variable binding format in let");
    }
    //create new frame to hold let bindings
    Frame *letFrame = talloc(sizeof(Frame));
    letFrame->parent = frame;
    letFrame->bindings = makeNull();
    //set bindings
    while (bindingList->type != NULL_TYPE) {
		bindLetArg(bindingList, frame, letFrame);
        bindingList = cdr(bindingList);
    }
    Value *letBody = cdr(letArgs);
    return evalLetBody(letBody, letFrame);
}

Value *evalLetStar(Value *args, Frame *frame) {
	Value *bindingList = makeNull();
    Value *letBody = makeNull();
    if (args->type == CONS_TYPE) {
        bindingList = car(args);
        letBody = cdr(args);
    }
    if (bindingList->type != CONS_TYPE && bindingList->type != NULL_TYPE) {
        evalError("improper variable binding format in let*");
    }
    Frame *parentFrame = frame;
	while (bindingList->type != NULL_TYPE) {
        Frame *letStarFrame = talloc(sizeof(Frame));
        letStarFrame->parent = parentFrame;
        letStarFrame->bindings = makeNull();
		bindLetArg(bindingList, parentFrame, letStarFrame);
        bindingList = cdr(bindingList);
        parentFrame = letStarFrame;
	}
    return evalLetBody(letBody, parentFrame);
	
}

Value *evalLetrec(Value *args, Frame *frame) {
    Value *bindingList = makeNull();
    Value *letBody = makeNull();
    if (args->type == CONS_TYPE) {
        bindingList = car(args);
        letBody = cdr(args);
    }
    if (bindingList->type != CONS_TYPE && bindingList->type != NULL_TYPE) {
        evalError("improper variable binding format in letrec");
    }
    
	Frame *letFrame = talloc(sizeof(Frame));
    letFrame->parent = frame;
    letFrame->bindings = makeNull();

	Value *variableList = makeNull();
	Value *expressionList = makeNull();
	while(bindingList->type != NULL_TYPE){
		Value *binding = car(bindingList);
		if (binding->type != CONS_TYPE) {
        	evalError("improper variable binding format in letrec");
    	} else if (length(binding) != 2) {
        	evalError("wrong number of items in letrec binding");
		}
		Value *variable = car(binding);
		if (variable->type != SYMBOL_TYPE) {
			evalError("improper variable type for binding in letrec");
		} else if (contains(letFrame->bindings, variable)) {
			evalError("duplicate bound variable in letrec");
		}
		Value *temp1 = cons(variable, variableList);
		variableList = temp1;
		Value *expression = eval(car(cdr(binding)), letFrame);
		Value *temp2 = cons(expression, expressionList);
		expressionList = temp2;
		bindingList = cdr(bindingList);
	}
	//bind each variable to its evaluated expression
	while(variableList->type != NULL_TYPE){
		Value *newBinding = cons(car(variableList), car(expressionList));
    	Value *temp = cons(newBinding, letFrame->bindings);
    	letFrame->bindings = temp;
		variableList = cdr(variableList);
		expressionList = cdr(expressionList);
	}
	return evalLetBody(letBody, letFrame);
}

Value *evalQuote(Value *args) {
    if (args->type == NULL_TYPE || cdr(args)->type != NULL_TYPE) {
        evalError("quote has more than 1 argument");
    }
    return car(args);
}

Value *evalDefine(Value *args, Frame *frame){
    if (args->type == NULL_TYPE) {
        evalError("no arguments passed to define");
    } else if (cdr(args)->type == NULL_TYPE) {
        evalError("no value to bind variable to in define");
    } else if (car(args)->type != SYMBOL_TYPE) {
        evalError("non-symbol cannot be bound to a value in define");
    }
    Value *binding = cons(car(args), eval(car(cdr(args)), frame));
    Value *temp = cons(binding, frame->bindings);
    frame->bindings = temp;
    Value *voidVal = makeNull();
    voidVal->type = VOID_TYPE;
    return voidVal;
}

/* 
* Sets the binding of variable to newVal in the scope of frame, if 
* the binding already exists. Returns 1 if successful, 0 if not.
*/
int setBinding(Value *variable, Value *newVal, Frame *frame){
    int isBound = 0;
    do {
        Value *bindings = frame->bindings;
        while(bindings->type != NULL_TYPE){
            Value *binding = car(bindings);
            if (!strcmp(car(binding)->s, variable->s)) {
                isBound = 1;
                binding->c.cdr = newVal;
                break;
            }
            bindings = cdr(bindings);
        }
        if (frame->parent != NULL) {
            frame = frame->parent;
        } else {
            //in global frame; don't look any further
            break;
        }  
    } while (frame->parent != NULL);
    return isBound;
}

Value *evalSet(Value *args, Frame *frame){
	 if (args->type == NULL_TYPE) {
        evalError("no arguments passed to set!");
    } else if (cdr(args)->type == NULL_TYPE) {
        evalError("no value to bind variable to in set!");
    } else if (car(args)->type != SYMBOL_TYPE) {
        evalError("non-symbol cannot be bound to a value in set!");
    } 
	Value *variable = car(args);
	Value *expression = eval(car(cdr(args)),frame);
    int varWasSet = setBinding(variable, expression, frame);
    if (!varWasSet) {
        evalError("no binding to modify in set!");
    } 
    Value *voidVal = makeNull();
    voidVal->type = VOID_TYPE;
    return voidVal;
}

Value *evalLambda(Value *args, Frame *frame){
    if (args->type == NULL_TYPE) {
        evalError("no arguments following lambda");
    }
    Value *lambdaArgs = car(args);
    if (lambdaArgs->type != NULL_TYPE && lambdaArgs->type != CONS_TYPE && lambdaArgs->type != SYMBOL_TYPE) {
        evalError("improper type for lambda argument(s)");
    } else if (lambdaArgs->type == CONS_TYPE && car(lambdaArgs)->type != SYMBOL_TYPE) {
        evalError("bad format for argument in lambda argument list");
    } else if (cdr(args)->type == NULL_TYPE) {
        evalError("empty body in lambda");
    } 
    while (lambdaArgs->type != NULL_TYPE) {
        if (contains(cdr(lambdaArgs), car(lambdaArgs))) {
            evalError("duplicate argument in lambda");
        }
        lambdaArgs = cdr(lambdaArgs);
    }
    Value *newClosure = makeNull();
    newClosure->type = CLOSURE_TYPE;
    newClosure->closure.paramNames = car(args);
    newClosure->closure.fnBody = car(cdr(args));
    newClosure->closure.frame = frame;
    return newClosure;
}

Value *evalBegin(Value *args, Frame *frame) {
    while (args->type != NULL_TYPE) {
        Value *evaluation = eval(car(args), frame);
        if (cdr(args)->type == NULL_TYPE) {
            return evaluation;
        }
		args = cdr(args);
    }
    Value *voidVal = makeNull();
    voidVal->type = VOID_TYPE;
    return voidVal;
}

Value *andOrHelper(Value *args, int andOr, Frame *frame) {
	Value *boolean = makeNull();
	while(args->type != NULL_TYPE){
		boolean = eval(car(args), frame);
        if (boolean->i == andOr) {
            return boolean;
        }
        args = cdr(args);
	}
    return boolean;
}

Value *evalAnd(Value *args, Frame *frame) {
    if (length(args) < 1) {
        evalError("too few arguments in and");
    }
    return andOrHelper(args, 0, frame);
}

Value *evalOr(Value *args, Frame *frame) {
	if (length(args) < 1) {
        evalError("too few arguments in or");
    }
	return andOrHelper(args, 1, frame);
}

Value *evalCond(Value *args, Frame *frame) {
    if (length(args) == 0) {
        evalError("no arguments in cond");
    }
    while (args->type != NULL_TYPE) {
        Value *condition = car(car(args));
        if (condition->type == SYMBOL_TYPE && 
            !strcmp(condition->s, "else")) {
            if (cdr(args)->type != NULL_TYPE) {
                evalError("else is not last test in cond");
            } 
            return eval(car(cdr(car(args))), frame);
        }
        condition = eval(condition, frame);
        if (condition->type != BOOL_TYPE) {
            evalError("non-boolean condition for if");
        } else if (condition->i) {
            return eval(car(cdr(car(args))), frame);
        }
        args = cdr(args);
    }
    Value *voidVal = makeNull();
    voidVal->type = VOID_TYPE;
    return voidVal;
}

/*
* Binds the actual parameters in valueList to the formal parameters 
* in varList in the specified frame.
*/
void bindArgs(Value *varList, Value *valueList, Frame *frame){
    while(varList->type != NULL_TYPE){
        if(valueList->type == NULL_TYPE){
            evalError("wrong number of parameters passed to function");
        }
        Value *binding = cons(car(varList), car(valueList));
        Value *temp = cons(binding, frame->bindings);
        frame->bindings = temp;
        if (frame->bindings->type == NULL_TYPE) {
            printf("frame binding is null") ;
        }
        varList = cdr(varList);
        valueList = cdr(valueList);
    }
    if(valueList->type != NULL_TYPE){
        evalError("wrong number of parameters passed to function");
    }
}

/*
* Evaluates the list of arguments passed in and returns the list of 
* evaluated arguments in the same order, for use in function         * applications.
*/
Value *evalFnArgs(Value *args, Frame *frame) {
    Value *evaluatedArgs = makeNull();
    while (args->type != NULL_TYPE) {
        Value *temp = cons(eval(car(args), frame), evaluatedArgs);
        evaluatedArgs = temp;
        args = cdr(args);
    }
    return reverse(evaluatedArgs);
}

/* Applies the closure passed in to the args passed in, creating a 
* new frame whose parent is the frame pointed to by the closure. 
*/
Value *apply(Value *function, Value *args) {
    if (function->type == CLOSURE_TYPE) {
        Frame *fnFrame = talloc(sizeof(Frame));
        fnFrame->parent = function->closure.frame;
        fnFrame->bindings = makeNull();
        Value *formalParams = function->closure.paramNames;
        Value *fnBody = function->closure.fnBody;
        bindArgs(formalParams, args, fnFrame);
        return eval(fnBody, fnFrame);
    } else if (function->type == PRIMITIVE_TYPE) {
        return (function->primFn)(args);
    } else {
        evalError("incorrect type for function in apply");
        return NULL;
    }
    
}

/* Evaluates the S-expression referred to by expr
* in the given frame and returns a Value pointer to the result of 
* the evaluation. 
*/
Value *eval(Value *tree, Frame *frame) {
    valueType type = tree->type;
    if (type == INT_TYPE || type == DOUBLE_TYPE || 
        type == BOOL_TYPE || type == STR_TYPE) {
        //atomic expressions
        return tree;
    } else if (type == SYMBOL_TYPE) {
        //variables
        return getBoundValue(tree->s, frame);
    } else if (type == CLOSURE_TYPE) {
        return apply(tree, makeNull());
    } else if (type == CONS_TYPE) {
        //special forms
        Value *first = car(tree);
        Value *args = cdr(tree);
        Value *result;
        if (!strcmp(first->s,"if")) {
            result = evalIf(args, frame);
        } else if (!strcmp(first->s, "let")) {
            result = evalLet(args, frame);
        } else if (!strcmp(first->s, "quote")) {
            result = evalQuote(args);
        } else if (!strcmp(first->s, "define")) {
            result = evalDefine(args, frame);
        } else if (!strcmp(first->s, "lambda")) {
            result = evalLambda(args, frame);
        } else if (!strcmp(first->s, "let*")) {
			result = evalLetStar(args, frame);
        } else if (!strcmp(first->s, "letrec")) {
            result = evalLetrec(args, frame);
		} else if (!strcmp(first->s, "set!")) {
			result = evalSet(args, frame);
        } else if (!strcmp(first->s, "begin")) {
            result = evalBegin(args, frame);
        } else if (!strcmp(first->s, "and")) {
            result = evalAnd(args, frame);
        } else if (!strcmp(first->s, "or")) {
            result = evalOr(args, frame);
        } else if (!strcmp(first->s, "cond")) {
            result = evalCond(args, frame);    
        } else {
            //applying a function
            args = evalFnArgs(args, frame);
            Value *function = eval(first, frame);
            result = apply(function, args);
        }
        return result;
    }
    return NULL;    
}