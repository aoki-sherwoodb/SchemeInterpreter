//Cons cell-based linked list implementation for a Scheme interpreter 

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "linkedlist.h"
#include "talloc.h"

// Create a new NULL_TYPE value node.
Value *makeNull() {
  Value *nullNode = talloc(sizeof(Value));
  nullNode->type = NULL_TYPE;
  return nullNode;
}

// Create a new CONS_TYPE value node.
Value *cons(Value *newCar, Value *newCdr) {
  Value *consCell = talloc(sizeof(Value));
  consCell->type = CONS_TYPE;
  consCell->c.car = newCar;
  consCell->c.cdr = newCdr;
  return consCell;
}

//prints the contents of a value that is not a list/cons cell
void displayValue(Value *item) {
  assert(item->type != PTR_TYPE);
  assert(item->type != CONS_TYPE);
  switch (item->type) {
    case INT_TYPE:
      printf("%i", item->i);
      break;
    case DOUBLE_TYPE:
      printf("%.2f", item->d);
      break;
    case STR_TYPE:
      printf("%s", item->s);
      break;
    case PTR_TYPE:
      break;
    case CONS_TYPE:
      break;
    case NULL_TYPE:
      break;
    case OPEN_TYPE:
      printf("(");
    case CLOSE_TYPE:
      printf(")");
    case BOOL_TYPE:
      if (item->i == 0) {
        printf("#f:boolean\n");
      } else {
        printf("#t:boolean\n");
      }
      break;
    case SYMBOL_TYPE:
      printf("%s", item->s);
      break;
    case OPENBRACKET_TYPE:
      printf("[");
      break;
    case CLOSEBRACKET_TYPE:
      printf("]");
      break;
    case DOT_TYPE:
      printf(".");
      break;
    case SINGLEQUOTE_TYPE:
      printf("'");
      break;
    case VOID_TYPE:
      break;
    case CLOSURE_TYPE:
      break;
    case PRIMITIVE_TYPE:
      break;
  }
}

// Display the contents of the linked list to the screen in some kind of readable format
void display(Value *list) {
  printf("(");
  if (list->type != NULL_TYPE) {
    displayValue(car(list));
    while (cdr(list)->type == CONS_TYPE) {
      list = cdr(list);
      printf(" ");
      displayValue(car(list));
    }
    if (!isNull(cdr(list))) {
      printf(" . ");
      displayValue(cdr(list));
    }
  }
  printf(")\n");
}

// Return a new list that is the reverse of the one that is passed in. No stored
// data within the linked list should be duplicated; rather, a new linked list
// of CONS_TYPE nodes should be created, that point to items in the original
// list.
Value *reverse(Value *list) {
  Value *reversedList = makeNull();
  if (list->type == NULL_TYPE) {
    return reversedList;
  }
  reversedList = cons(car(list), reversedList);
  while (!isNull(cdr(list))) {
    list = cdr(list);
    reversedList = cons(car(list), reversedList);
  }
  return reversedList;
}

// Utility to make it less typing to get car value. Use assertions to make sure that this is a legitimate operation.
Value *car(Value *list) {
  assert(list != NULL);
  assert(list->type == CONS_TYPE);
  return list->c.car;
}

// Utility to make it less typing to get cdr value. Use assertions to make sure that this is a legitimate operation.
Value *cdr(Value *list)  {
  assert(list != NULL);
  assert(list->type == CONS_TYPE);
  return list->c.cdr;
}

// Utility to check if pointing to a NULL_TYPE value. Use assertions to make sure that this is a legitimate operation.
bool isNull(Value *value) {
  assert(value != NULL);
  if (value->type == NULL_TYPE) {
    return 1;
  } else {
    return 0;
  }
}

// Measure length of list. Use assertions to make sure that this is a legitimate operation.
int length(Value *value) {
  assert(value != NULL);
  int length = 0;
  if (value->type == NULL_TYPE) {
    return length;
  }
  length++;
  assert(value->type == CONS_TYPE);
  while (!isNull(cdr(value))) {
    length++;
    value = cdr(value);
  }
  return length;
}

// int main() {
//   Value *car1 = malloc(sizeof(Value));
//   car1->type = INT_TYPE;
//   car1->i = 2;
//   Value *car2 = malloc(sizeof(Value));
//   car2->type = DOUBLE_TYPE;
//   car2->d = 3.0;
//   Value *car3 = malloc(sizeof(Value));
//   car3->type = STR_TYPE;
//   car3->s = malloc(sizeof(char) * 9);
//   strcpy(car3->s, "Segfault");
//   Value *head = cons(car1, cons(car2, cons(car3, makeNull())));
//   display(head);
//   Value *reverseHead = reverse(head);
//   display(reverseHead);
//   printf("%i\n",length(head));
//   cleanup(head);
//   cleanup(reverseHead);
// }