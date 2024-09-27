#define FSM_IMPLEMENTATION
#include "fsm.h"

#define REGEX_SPECIAL_ALLOWED_BIT 0x01
#define REGEX_PASSTHROUGH_BIT     0x02
#define REGEX_ANY_BIT             0x04

#define GET_BIT(n, b) (n&b)
#define SET_BIT(n, b) n |= b
#define CLEAR_BIT(n, b) n &= ~(b)

typedef struct {
  fsm_t fsm;
  uint8_t flags;
  fsm_state_t pass_state;
} regex_t;

void regex_init(regex_t *regex) {
  fsm_init(&regex->fsm, 127);
  fsm_push_empty(&regex->fsm);
}

void regex_free(regex_t regex) {
  if (fsm_initialized(regex.fsm)) fsm_free(regex.fsm);
}

bool regex_compile(regex_t *regex, const char *pattern) {
  while (*pattern) {
    switch(*pattern) {
    case '?': {
      if (!GET_BIT(regex->flags, REGEX_SPECIAL_ALLOWED_BIT)) return false;
      regex->flags = 0;
      SET_BIT(regex->flags, REGEX_PASSTHROUGH_BIT);
    } break;
    case '*': {
      if (!GET_BIT(regex->flags, REGEX_SPECIAL_ALLOWED_BIT)) return false;
      fsm_state_t state = regex->fsm.count-1;
      for (fsm_event_t i = 32; i < 127; ++i) if (fsm_get(regex->fsm, state, i) != 0) fsm_set(&regex->fsm, state, i, state);
      regex->flags = 0;
      SET_BIT(regex->flags, REGEX_PASSTHROUGH_BIT);
    } break;
    case '+': {
      if (!GET_BIT(regex->flags, REGEX_SPECIAL_ALLOWED_BIT)) return false;
      fsm_state_t prev_state = regex->fsm.count-1;
      fsm_state_t new_state = fsm_push_empty(&regex->fsm);
      for (fsm_event_t i = 32; i < 127; ++i) if (fsm_get(regex->fsm, prev_state, i) != 0) fsm_set(&regex->fsm, new_state, i, new_state);
      regex->flags = 0;
      SET_BIT(regex->flags, REGEX_PASSTHROUGH_BIT);
    } break;
    case '.': {
      fsm_state_t state = fsm_push_empty(&regex->fsm);
      for (fsm_event_t i = 32; i < 127; ++i) fsm_set(&regex->fsm, state, i, regex->fsm.count);
      regex->flags = 0;
      SET_BIT(regex->flags, REGEX_ANY_BIT);
      SET_BIT(regex->flags, REGEX_SPECIAL_ALLOWED_BIT);
    } break;
    default: {
      fsm_state_t state = regex->fsm.count-1;
      if (!GET_BIT(regex->flags, REGEX_PASSTHROUGH_BIT)) state = fsm_push_empty(&regex->fsm);
      fsm_set(&regex->fsm, state, *pattern, regex->fsm.count);
      regex->flags = 0;
      SET_BIT(regex->flags, REGEX_SPECIAL_ALLOWED_BIT);
    }
    }

    ++pattern;
  }
  if (GET_BIT(regex->flags, REGEX_PASSTHROUGH_BIT)) fsm_set(&regex->fsm, regex->fsm.count-1, 0, regex->fsm.count);

  return true;
}

bool regex_match(regex_t *regex, const char *text) {
  regex->fsm.state = 1;
  while (*text && regex->fsm.state > 0 && regex->fsm.state < regex->fsm.count) (void)fsm_fire_event(&regex->fsm, *(text++));

  if (regex->fsm.state == 0 || *text) return false;
  else if (regex->fsm.state >= regex->fsm.count) return true;
  else (void)fsm_fire_event(&regex->fsm, 0);

  return regex->fsm.state >= regex->fsm.count;
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

  for (size_t i = 0; i < test_count; ++i) {
    test_t test = tests[i];

    regex_t regex = {0};
    regex_init(&regex);
    if (!regex_compile(&regex, test.pattern)) {
      fprintf(stderr, "Failed to compile pattern %s\n", test.pattern);
      return 1;
    }

    bool actual = regex_match(&regex, test.text);
    if (actual != test.expected) fsm_dump(regex.fsm);
    printf("(%zu/%zu): ", i+1, test_count);
    if (actual == test.expected) printf("Success!\n");
    else {
      printf("Failed!\n");
      printf("Expected %d but got %d\n", test.expected, actual);
      return 1;
    }

    regex_free(regex);
  }

  return 0;
}