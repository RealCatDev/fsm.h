#ifndef   FSM_H_
#define   FSM_H_

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t fsm_state_t;
typedef uint32_t fsm_event_t;
typedef fsm_state_t *fsm_column_t;

typedef struct {
  fsm_state_t state;
  size_t event_count;
  fsm_column_t *items;
  size_t capacity;
  size_t count;
} fsm_t;

fsm_state_t fsm_push_empty(fsm_t *fsm);
void fsm_set(fsm_t *fsm, fsm_state_t column, fsm_event_t row, fsm_state_t state);
fsm_state_t fsm_get(fsm_t fsm, fsm_state_t column, fsm_event_t row);
fsm_state_t fsm_fire_event(fsm_t *fsm, fsm_event_t event);
void fsm_dump(fsm_t fsm);
void fsm_free(fsm_t fsm);

void fsm_init(fsm_t *fsm, size_t event_count);

#ifdef FSM_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

bool fsm_initialized(fsm_t fsm) {
  return fsm.event_count > 0;
}

void fsm_init(fsm_t *fsm, size_t event_count) {
  assert(fsm);
  if (fsm->event_count != 0) return; // Already initialized
  fsm->event_count = event_count;
}

fsm_state_t fsm_push_empty(fsm_t *fsm) {
  assert(fsm);
  if (fsm->count >= fsm->capacity) {
    if (fsm->capacity == 0) fsm->capacity = 16;
    else fsm->capacity *= 2;
    fsm->items = realloc(fsm->items, sizeof(*fsm->items) * fsm->capacity);
    assert(fsm->items && "Buy more RAM lol");
  }
  fsm->items[fsm->count] = calloc(sizeof(**fsm->items), fsm->event_count);
  return fsm->count++;
}

void fsm_set(fsm_t *fsm, fsm_state_t column, fsm_event_t row, fsm_state_t state) {
  assert(fsm);
  assert(column <= fsm->count);
  assert(row <= fsm->event_count);
  fsm->items[column][row] = state;
}

fsm_state_t fsm_get(fsm_t fsm, fsm_state_t column, fsm_event_t row) {
  assert(column <= fsm.count);
  assert(row <= fsm.event_count);
  return fsm.items[column][row];
}

fsm_state_t fsm_fire_event(fsm_t *fsm, fsm_event_t event) {
  assert(event <= fsm->event_count);
  return fsm->state = fsm->items[fsm->state][event];
}

void fsm_dump(fsm_t fsm) {
  printf("fsm:\n");
  for (size_t i = 0; i < fsm.event_count; ++i) {
    printf("%3zu: ", i);
    for (size_t j = 0; j < fsm.count; ++j) {
      if (j > 0) printf(", ");
      printf("%u", fsm_get(fsm, j, i));
    }
    printf("\n");
  }
}

void fsm_free(fsm_t fsm) {
  assert(fsm.items);
  for (size_t i = 0; i < fsm.count; ++i) {
    free(fsm.items[i]);
  }
  free(fsm.items);
}

#endif // FSM_IMPLEMENTATION

#endif // FSM_H_