#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include "tokenizer.h"
#include "talloc.h"
#include "linkedlist.h"

int isParens(char target) {
    char parens[] = {'(', ')', '[', ']'};
    int length = sizeof(parens);
    for (int i = 0; i < length; i++) {
        if (parens[i] == target) {
            return 1;
        }
    }
    return 0;
}

int isMiscSymbol(char target) {
    char miscSymbols[] = {'!', '$', '%', '&', '*', '/', ':', '<',
                            '=', '>', '?', '~', '_', '^'};
    int length = sizeof(miscSymbols);
    for (int i = 0; i < length; i++) {
        if (miscSymbols[i] == target) {
            return 1;
        }
    }
    return 0;
}

void tokenizationError(const char *message, char *token) {
    printf("Syntax error %s: %s\n", message, token);
    texit(1);
}

Value *addTokenToList(Value *head, valueType tokenType, char *token) {
    /* token types:
    boolean, integer, double, string, symbol, open, close*/
    Value *newVal = talloc(sizeof(Value));
    newVal->type = tokenType;
    if (tokenType == STR_TYPE || tokenType == SYMBOL_TYPE) {
        newVal->s = token;
    } else if (tokenType == INT_TYPE || tokenType == BOOL_TYPE) {
        newVal->i = (int) strtol(token, (char **)NULL, 10);
    } else if (tokenType == DOUBLE_TYPE) {
        newVal->d = (double) strtod(token, (char**)NULL);
    } else {
        newVal->s = token;
    }
    Value *temp = head;
    head = cons(newVal, temp);
    return head;
}

Value *addStringToken(Value *head) {
    char *string = talloc(301);
    string[0] = '"';
    char nextChar = (char)fgetc(stdin);
    int i = 1;
    while(nextChar != '"'){
        string[i] = nextChar;
        i++;
        if (nextChar == EOF){
            string[i] = '\0';
            tokenizationError("reached end of file while tokenizing string", string);
            break;
        }
        nextChar = (char)fgetc(stdin);
    }
    string[i] = '"';
    string[i+1] = '\0';
    head = addTokenToList(head, STR_TYPE, string);
    return head;
}

Value *addNumberToken(Value *head) {
    char nextChar = (char)fgetc(stdin);
    char *number = talloc(301);
    int endOfNumber = 0;
    int isDouble = 0;
    int i = 0;
    while (!endOfNumber) {
        if (nextChar == '.') {
            isDouble = 1;
        }
        number[i] = nextChar;
        i++;
        nextChar = (char)fgetc(stdin);
        if (isParens(nextChar) || 
        isspace(nextChar) || nextChar == EOF) {
            ungetc(nextChar, stdin);
            break;
        } else if (!isdigit(nextChar) && nextChar != '.') {
            number[i] = '\0';
            tokenizationError("invalid character for double or int token", number);
            break;
        }
    }
    number[i] = '\0';
    valueType tokenType;
    if (isDouble) {
        tokenType = DOUBLE_TYPE;
    } else {
        tokenType = INT_TYPE;
    }
    head = addTokenToList(head, tokenType, number);
    return head;
}

Value *addSymbolToken(Value *head) {
    char nextChar = (char)fgetc(stdin);
    char *symbol = talloc(301);
    int endOfSymbol = 0;
    int i = 0;
    while (endOfSymbol != 1) {
        symbol[i] = nextChar;
        i++;
        nextChar = (char)fgetc(stdin);
        if (isParens(nextChar) || 
        isspace(nextChar) || nextChar == EOF) {
            break;
        }
    }
    symbol[i] = '\0';
    ungetc(nextChar, stdin);
    head = addTokenToList(head, SYMBOL_TYPE, symbol);
    return head;
}

Value *addBoolToken(Value *head) {
    char nextChar = (char)fgetc(stdin);
    char *intBool = talloc(2);
    if (nextChar == 't' || nextChar == 'T') {
        intBool[0] = '1';
    } else if (nextChar == 'f' || nextChar == 'F') {
        intBool[0] = '0';
    } else {
        char *badChar = talloc(2);
        badChar[0] = nextChar;
        badChar[1] = '\0';
        tokenizationError("invalid character for boolean token", badChar);
    }
    intBool[1] = '\0';
    nextChar = (char)fgetc(stdin);
    ungetc(nextChar, stdin);
    if (!isspace(nextChar) && nextChar != ')') {
        char *badChar = talloc(2);
        badChar[0] = nextChar;
        badChar[1] = '\0';
        tokenizationError("non-whitespace character detected after boolean token", badChar);
    }
    head = addTokenToList(head, BOOL_TYPE, intBool);
    return head;
}

int skipComment() {
    char nextChar = (char)fgetc(stdin);
    while (nextChar != '\n' && nextChar != EOF) {
        nextChar = (char)fgetc(stdin);
    }
    if (nextChar == EOF) {
        return 1;
    } else {
        return 0;
    }
}

// Read all of the input from stdin, and return a linked list consisting of the
// tokens.
Value *tokenize() {
    char charRead;
    Value *list = makeNull();
    charRead = (char)fgetc(stdin);
    while (charRead != EOF) {
        if (charRead == '"') {
            list = addStringToken(list);
        } else if (charRead == ';') {
            if (skipComment()) {
                break;
            }
        } else if (charRead == '(') {
            char *character = talloc(2);
            character[0] = charRead;
            list = addTokenToList(list, OPEN_TYPE, character);
        } else if (charRead == ')') {
            char *character = talloc(2);
            character[0] = charRead;
            list = addTokenToList(list, CLOSE_TYPE, character);
        } else if (charRead == '[') {
            char *character = talloc(2);
            character[0] = charRead;
            list = addTokenToList(list, OPENBRACKET_TYPE, character);
        } else if (charRead == ']') {
            char *character = talloc(2);
            character[0] = charRead;
            list = addTokenToList(list, CLOSEBRACKET_TYPE, character);
        } else if (charRead == '+' || charRead == '-') {
            char nextChar = (char)fgetc(stdin);
            ungetc(nextChar, stdin);
            ungetc(charRead, stdin);
            if (isspace(nextChar) || nextChar == ')') {
                list = addSymbolToken(list);
            } else {
                list = addNumberToken(list);
            }
        } else if (isalpha(charRead) || isMiscSymbol(charRead)) {
            ungetc(charRead, stdin);
            list = addSymbolToken(list); 
        } else if (isdigit(charRead) || charRead == '.') {
            char nextChar = (char)fgetc(stdin);
            ungetc(nextChar, stdin);
            if (isspace(nextChar) && charRead == '.') {
                char *character = talloc(2);
                character[0] = charRead;
                list = addTokenToList(list, DOT_TYPE, character);
            } else {
                ungetc(charRead, stdin);
                list = addNumberToken(list);
            }
        } else if (charRead == '\'') {
            char nextChar = (char)fgetc(stdin);
            ungetc(nextChar, stdin);
            if (isspace(nextChar)) {
                tokenizationError("whitespace after single quote", "\' ");
            }
            char *character = talloc(2);
            character[0] = charRead;
            character[1] = '\0';
            list = addTokenToList(list, SINGLEQUOTE_TYPE, character);
        } else if (charRead == '#') {
            list = addBoolToken(list);
        } else if (isspace(charRead)) {
           // do nothing
        } else {
            char *character = talloc(2);
            character[0] = charRead;
            character[1] = '\0';
            tokenizationError("character does not match starting characters for any token type", character);
        }
        charRead = (char)fgetc(stdin);   
    }
    Value *revList = reverse(list);
    return revList;
}

//use gets(*char) to get a line from stdin--we can preset a string to length 301 and be safe

// Displays the contents of the linked list as tokens, with type information
void displayTokens(Value *list) {
    while (!isNull(list)) {
        Value *tokenValue = car(list);
        switch(tokenValue->type) {
            case BOOL_TYPE:
                if (tokenValue->i == 0) {
                    printf("#f:boolean\n"); 
                } else {
                    printf("#t:boolean\n");
                }
                break;
            case INT_TYPE:
                printf("%d:integer\n", tokenValue->i);
                break;
            case DOUBLE_TYPE:
                printf("%f:double\n", tokenValue->d);
                break;
            case STR_TYPE:
                printf("%s:string\n", tokenValue->s);
                break;
            case SYMBOL_TYPE:
                printf("%s:symbol\n", tokenValue->s);
                break;
            case OPEN_TYPE:
                printf("(:open\n");
                break;
            case CLOSE_TYPE:
                printf("):close\n");
                break;
            case OPENBRACKET_TYPE:
                printf("[:openbracket\n");
                break;
            case CLOSEBRACKET_TYPE:
                printf("]:closebracket\n");
                break;
            case DOT_TYPE:
                printf(".:dot\n");
                break;
            case SINGLEQUOTE_TYPE:
                printf("':singlequote\n");
                break;
            case CONS_TYPE:
                break;  
            case PTR_TYPE:
                break;  
            case NULL_TYPE:
                break;
            case VOID_TYPE:
                break;
            case CLOSURE_TYPE:
                break;
            case PRIMITIVE_TYPE:
                break;
        }
        Value *temp = list;
        list = cdr(temp);
    }
}
