#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "talloc.h"

Value *head = NULL;

// Create a new NULL_TYPE value node.
Value *tMakeNull() {
  Value *nullNode = malloc(sizeof(Value));
  nullNode->type = NULL_TYPE;
  return nullNode;
}

// Create a new CONS_TYPE value node.
Value *tCons(Value *newCar, Value *newCdr) {
  Value *consCell = malloc(sizeof(Value));
  consCell->type = CONS_TYPE;
  consCell->c.car = newCar;
  consCell->c.cdr = newCdr;
  return consCell;
}

Value *tCar(Value *list) {
  assert(list != NULL);
  assert(list->type == CONS_TYPE);
  return list->c.car;
}

// Utility to make it less typing to get cdr value. Use assertions to make sure that this is a legitimate operation.
Value *tCdr(Value *list)  {
  assert(list != NULL);
  assert(list->type == CONS_TYPE);
  return list->c.cdr;
}

// Utility to check if pointing to a NULL_TYPE value. Use assertions to make sure that this is a legitimate operation.
bool tIsNull(Value *value) {
  assert(value != NULL);
  if (value->type == NULL_TYPE) {
    return 1;
  } else {
    return 0;
  }
}

// Replacement for malloc that stores the pointers allocated. It should store
// the pointers in some kind of list; a linked list would do fine, but insert
// here whatever code you'll need to do so; don't call functions in the
// pre-existing linkedlist.h. Otherwise you'll end up with circular
// dependencies, since you're going to modify the linked list to use talloc.
void *talloc(size_t size){
  if (head == NULL) {
    head = tMakeNull();
  }
  void *item = malloc(size);
  Value *pointer = malloc(sizeof(Value));
  pointer->type = PTR_TYPE;
  pointer->p = item;
  Value *temp = head;
  head = tCons(pointer, temp);
  return item;
}

// Free all pointers allocated by talloc, as well as whatever memory you
// allocated in lists to hold those pointers.
void tfree(){
  Value *current = head;
  while(!tIsNull(current)){

    Value *pointerValue = tCar(current);
    free(pointerValue->p);
    free(pointerValue);
    Value *temp = tCdr(current);
    free(current);
    current = temp;
  }
  free(current);
  head = NULL;
}

// Replacement for the C function "exit", that consists of two lines: it calls
// tfree before calling exit. It's useful to have later on; if an error happens,
// you can exit your program, and all memory is automatically cleaned up.
void texit(int status) {
  tfree();
  exit(status);
}