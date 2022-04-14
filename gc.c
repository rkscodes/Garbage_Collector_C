#include <stdio.h>
#include <stdlib.h>

#define STACK_MAX 120
#define INITIAL_GC_THRESHOLD 8

typedef enum { OBJ_INT, OBJ_PAIR } ObjectType;

typedef struct sObject {
  ObjectType type;
  unsigned char marked;
  struct sObject *next;
  union {
    int value;
    struct {
      struct sObject *head;
      struct sObject *tail;
    };
  };
} Object;

typedef struct {
  Object *stack[STACK_MAX];
  int stacksize;

  Object *firstObject;

  int noOfObjects;
  int maxNoOfObjects;
} VM;

void gc(VM *vm);
void mark(Object *object);

void assert(int condition, const char *message) {
  if (!condition) {
    printf("%s\n", message);
    exit(1);
  }
}

VM *newVM() {
  VM *vm = malloc(sizeof(VM));
  vm->stacksize = 0;
  vm->firstObject = NULL;
  vm->noOfObjects = 0;
  vm->maxNoOfObjects = INITIAL_GC_THRESHOLD;
  return vm;
}

void push(VM *vm, Object *obj) {
  assert(vm->stacksize < STACK_MAX, "Stack Oveflow");
  vm->stack[vm->stacksize++] = obj;
}

Object *pop(VM *vm) {
  assert(vm->stacksize > 0, "Stack Underflow");
  return vm->stack[--vm->stacksize];
}

Object *newObj(VM *vm, ObjectType type) {
  if (vm->noOfObjects == vm->maxNoOfObjects) gc(vm);
  Object *obj = malloc(sizeof(Object));
  obj->type = type;
  obj->marked = 0;
  obj->next = vm->firstObject;
  vm->firstObject = obj;
  vm->noOfObjects++;
  return obj;
}

void pushInt(VM *vm, int intVal) {
  Object *obj = newObj(vm, OBJ_INT);
  obj->value = intVal;
  push(vm, obj);
}

// the idea is that you push key first and then value which you want to make
// pair of and then do this operation to make them pair.
Object *pushPair(VM *vm) {
  Object *obj = newObj(vm, OBJ_PAIR);
  obj->type = OBJ_PAIR;
  obj->tail = pop(vm);
  obj->head = pop(vm);
  push(vm, obj);
  return obj;
}

// WE will implement mark-sweep algo to clean the memory

void markAll(VM *vm) {
  for (int i = 0; i < vm->stacksize; i++) {
    mark(vm->stack[i]);
  }
}

void mark(Object *object) {
  // if we already have marked it, this will make sure will not end in loop
  if (object->marked == 1) return;
  object->marked = 1;
  if (object->type == OBJ_PAIR) {
    mark(object->head);
    mark(object->tail);
  }
}

// sweep algorithm

void sweep(VM *vm) {
  Object **object = &vm->firstObject;
  while (*object) {
    if (!(*object)->marked) {
      Object *unreached = *object;
      *object = unreached->next;
      free(unreached);
      vm->noOfObjects--;
    } else {
      // this object was marked so unmark it for next GC
      (*object)->marked = 0;
      object = &(*object)->next;
    }
  }
}

// let's wrap the whole process above in single part

void gc(VM *vm) {
  int numOfObjects = vm->noOfObjects;
  markAll(vm);
  sweep(vm);
  vm->maxNoOfObjects =
      vm->noOfObjects == 0 ? INITIAL_GC_THRESHOLD : vm->noOfObjects * 2;
  if (vm->maxNoOfObjects > STACK_MAX) vm->maxNoOfObjects = STACK_MAX;
  // This will atleast try to collect garbage when max size is hit

  printf("Collected %d ojbects, %d remaining.\n",
         numOfObjects - vm->noOfObjects, vm->noOfObjects);
}

void objectPrint(Object *obj) {
  switch (obj->type) {
    case OBJ_INT:
      printf("%d", obj->value);
      break;
    case OBJ_PAIR:
      printf("(");
      objectPrint(obj->head);
      printf(", ");
      objectPrint(obj->tail);
      printf(")");
      break;
  }
}

void freeVM(VM *vm) {
  // this would make sure that mark doesn't markAll doesn't mark any object and
  // will be sweeped by sweep
  vm->stacksize = 0;
  gc(vm);
  free(vm);
}

void test1() {
  printf("Test 1: Objects on stack are preserved. \n");
  VM *vm = newVM();
  pushInt(vm, 1);
  pushInt(vm, 2);
  gc(vm);
  assert(vm->noOfObjects == 2, "Should have preserved ojbects.");
  freeVM(vm);
}

void test2() {
  printf("Test 2: Unreached objects are collected.\n");
  VM *vm = newVM();
  pushInt(vm, 1);
  pushInt(vm, 2);
  pop(vm);
  pop(vm);
  gc(vm);
  assert(vm->noOfObjects == 0, "Should have collected objects.");
  freeVM(vm);
}

void test3() {
  printf("Test 3: Reach nested objects.\n");
  VM *vm = newVM();
  pushInt(vm, 1);
  pushInt(vm, 2);
  pushPair(vm);
  pushInt(vm, 3);
  pushInt(vm, 4);
  pushPair(vm);
  pushPair(vm);

  gc(vm);
  assert(vm->noOfObjects == 7, "Should have reached objects.");
  freeVM(vm);
}

void test4() {
  printf("Test 4: Handle Cycles.\n");
  VM *vm = newVM();
  pushInt(vm, 1);
  pushInt(vm, 2);
  Object *a = pushPair(vm);
  pushInt(vm, 3);
  pushInt(vm, 4);
  Object *b = pushPair(vm);
  a->tail = b;
  b->tail = a;
  gc(vm);
  assert(vm->noOfObjects == 4, "Should have collected objects.\n");
  freeVM(vm);
}

void perfTest() {
  printf("Performance Test.\n");
  VM *vm = newVM();

  for (int i = 0; i < 1000; i++) {
    for (int j = 0; j < 20; j++) {
      pushInt(vm, i);
    }

    for (int k = 0; k < 20; k++) {
      pop(vm);
    }
  }
  freeVM(vm);
}

int main(int argc, const char *argv[]) {
  test1();
  test2();
  test3();
  test4();
  perfTest();
  return 0;
}