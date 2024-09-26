#define FSM_IMPLEMENTATION
#include "fsm.h"

#define REGEX_SPECIAL_ALLOWED_BIT 0x01
#define REGEX_PASSTHROUGH_BIT     0x02
#define REGEX_ANY_BIT             0x04

#define GET_BIT(n, b) (n&b)
#define SET_BIT(n, b) n |= b
#define CLEAR_BIT(n, b) n &= ~(b)

bool regex_compile(fsm_t *fsm, const char *pattern) {
  fsm_init(fsm, 127);
  fsm_push_empty(fsm);

  uint8_t flags = 0;
  while (*pattern) {
    switch(*pattern) {
    case '?': {
      if (!GET_BIT(flags, REGEX_SPECIAL_ALLOWED_BIT)) return false;
      // fsm_set()
      flags = 0;
      SET_BIT(flags, REGEX_PASSTHROUGH_BIT);
    } break;
    case '*': {
      if (!GET_BIT(flags, REGEX_SPECIAL_ALLOWED_BIT)) return false;
      fsm_state_t state = fsm->count-1;
      for (fsm_event_t i = 32; i < 127; ++i) if (fsm_get(*fsm, state, i) != 0) fsm_set(fsm, state, i, state);
      flags = 0;
      SET_BIT(flags, REGEX_PASSTHROUGH_BIT);
    } break;
    case '+': {
      if (!GET_BIT(flags, REGEX_SPECIAL_ALLOWED_BIT)) return false;
      fsm_state_t prev_state = fsm->count-1;
      fsm_state_t new_state = fsm_push_empty(fsm);
      for (fsm_event_t i = 32; i < 127; ++i) if (fsm_get(*fsm, prev_state, i) != 0) fsm_set(fsm, new_state, i, new_state);
      flags = 0;
      SET_BIT(flags, REGEX_PASSTHROUGH_BIT);
    } break;
    case '.': {
      fsm_state_t state = fsm_push_empty(fsm);
      for (fsm_event_t i = 32; i < 127; ++i) fsm_set(fsm, state, i, fsm->count);
      flags = 0;
      SET_BIT(flags, REGEX_ANY_BIT);
      SET_BIT(flags, REGEX_SPECIAL_ALLOWED_BIT);
    } break;
    default: {
      fsm_state_t state = fsm->count-1;
      if (!GET_BIT(flags, REGEX_PASSTHROUGH_BIT)) state = fsm_push_empty(fsm);
      fsm_set(fsm, state, *pattern, fsm->count);
      flags = 0;
      SET_BIT(flags, REGEX_SPECIAL_ALLOWED_BIT);
    }
    }

    ++pattern;
  }
  if (GET_BIT(flags, REGEX_PASSTHROUGH_BIT)) fsm_set(fsm, fsm->count-1, 0, fsm->count);

  return true;
}

bool regex_match(fsm_t *fsm, const char *text) {
  fsm->state = 1;
  while (*text && fsm->state > 0 && fsm->state < fsm->count) (void)fsm_fire_event(fsm, *(text++));

  if (fsm->state == 0 || *text) return false;
  else if (fsm->state >= fsm->count) return true;
  else (void)fsm_fire_event(fsm, 0);

  return fsm->state >= fsm->count;
}

bool match(const char *pattern, const char *text, fsm_t *fsm) {
  if (fsm_initialized(*fsm)) fsm_free(*fsm);
  memset(fsm, 0, sizeof(*fsm));
  if (!regex_compile(fsm, pattern)) {
    fprintf(stderr, "Failed to compile regex!\n");
    return false;
  }

  return regex_match(fsm, text);
}

int main(void) {
  typedef struct {
    const char *pattern;
    const char *text;
    bool expected;
  } test_t;
  test_t tests[] = {
    (test_t){
      .pattern = "abc",
      .text = "abc",
      .expected = true
    },
    (test_t){
      .pattern = "abc",
      .text = "ab",
      .expected = false
    },
    (test_t){
      .pattern = "abc",
      .text = "abd",
      .expected = false
    },

    (test_t){
      .pattern = "abc?",
      .text = "abc",
      .expected = true
    },
    (test_t){
      .pattern = "abc?",
      .text = "ab",
      .expected = true
    },
    (test_t){
      .pattern = "abc?",
      .text = "abcd",
      .expected = false
    },

    (test_t){
      .pattern = "a*",
      .text = "a",
      .expected = true
    },
    (test_t){
      .pattern = "a*",
      .text = "aaaaa",
      .expected = true
    },
    (test_t){
      .pattern = "a*",
      .text = "b",
      .expected = false
    },
    (test_t){
      .pattern = "a*bc",
      .text = "abc",
      .expected = true
    },
    (test_t){
      .pattern = "a*bc",
      .text = "bbc",
      .expected = false
    },
    (test_t){
      .pattern = "a*bc",
      .text = "aaaaabc",
      .expected = true
    },
    (test_t){
      .pattern = "a*bc",
      .text = "aaaaac",
      .expected = false
    },

    (test_t){
      .pattern = "a+",
      .text = "a",
      .expected = true
    },
    (test_t){
      .pattern = "a+",
      .text = "aaaaa",
      .expected = true
    },
    (test_t){
      .pattern = "a+",
      .text = "",
      .expected = false
    },
    (test_t){
      .pattern = "a+",
      .text = "ab",
      .expected = false
    },
    (test_t){
      .pattern = "a+bc",
      .text = "abc",
      .expected = true
    },
    (test_t){
      .pattern = "a+bc",
      .text = "bc",
      .expected = false
    },
    (test_t){
      .pattern = "a+bc",
      .text = "aaaaabc",
      .expected = true
    },
    (test_t){
      .pattern = "a+bc",
      .text = "aaaaac",
      .expected = false
    },
  };
  size_t test_count = sizeof(tests)/sizeof(tests[0]);

  fsm_t fsm = {0};
  for (size_t i = 0; i < test_count; ++i) {
    test_t test = tests[i];
    bool actual = match(test.pattern, test.text, &fsm);
    if (actual != test.expected) fsm_dump(fsm);
    printf("(%zu/%zu): ", i+1, test_count);
    if (actual == test.expected) printf("Success!\n");
    else {
      printf("Failed!\n");
      printf("Expected %d but got %d\n", test.expected, actual);
      return 1;
    }
  }

  return 0;
}