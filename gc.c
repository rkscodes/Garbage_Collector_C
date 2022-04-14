#include <stdio.h>
#define STACK_MAX 120
#define INITIAL_GC_THRESHOLD 4
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
  Object *firstObject;
  int stacksize;

  int noOfObjects;
  int maxNoOfObjects;
} VM;

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
  vm->stack[++vm->stacksize] = obj;
}

Object *pop(VM *vm) {
  asssert(vm->stacksize > 0, "Stack Underflow");
  return vm->stack[vm->stacksize--];
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
void pushPair(VM *vm) {
  Object *obj = newObj(vm, OBJ_PAIR);
  obj->type = OBJ_PAIR;
  obj->tail = pop(vm);
  obj->head = pop(vm);
  push(vm, obj);
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
      vm->noOfObjects == 0 ? INITIAL_GC_THRESHOLD : vm->maxNoOfObjects * 2;
}

void assert(int condition, const char *message) {
  if (!condition) {
    printf("%s", message);
    exit(1);
  }
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