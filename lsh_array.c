#ifndef lsh_array_h
#define lsh_array_h

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "lsh_types.h"

typedef struct {
  u16 el_size;
  u32 cap;
  u32 inc;
  u32 length;
  void* data;
} Array;

void array_fill(Array *a, u32 length, void* data) {
  a->data = malloc(a->cap * a->el_size); 
  a->length = length;
  // check increase
  if (a->length > a->cap) {
    for (; a->length > a->cap; a->cap += a->inc);
    a->data = realloc(a->data, a->cap * a->el_size);
  }
  memcpy(a->data, data, a->length * a->el_size);
}

void array_add(Array *a, u32 length, void* data) { 
  a->length += length;
  // check increase
  if (a->length > a->cap) {
    for (; a->length > a->cap; a->cap += a->inc);
    a->data = realloc(a->data, a->cap * a->el_size);
  }
  memcpy(a->data, data, a->length * a->el_size);
}

void* array_get(Array *a, u32 idx) {
  return a->data + idx * a->el_size;
}

typedef struct {
  u8 age;
  char name[10];
} Person;

int main() {
  Person people[2] = {{27, "Julia"}, {41, "Lukas"}};
  printf("sizeof Person: %d\n", sizeof(Person));
  for (int i = 0; i < 2; i++) {
    printf("%s is %d years old.\n", people[i].name, people[i].age);
  }
  Array people_array = {sizeof(Person), 10, 5};
  array_fill(&people_array, 2, &people);
  
  for (int i = 0; i < 2; i++) {
    //Person *p = (Person*) (people_array.data + i);
    Person *p = (Person*) array_get(&people_array, i);
    printf("%s is %d years old.\n", p->name, p->age);
  }
  Person p = {31, "Gregor"};
  array_add(&people_array, 1, &p);
  for (int i = 0; i < people_array.length; i++) {
    //Person *p = (Person*) (people_array.data + i);
    Person *p = (Person*) array_get(&people_array, i);
    printf("%s is %d years old.\n", p->name, p->age);
  }
}

#ifdef LSH



#endif
#endif
