#define FSM_IMPLEMENTATION
#include "fsm.h"

#define REGEX_SPECIAL_ALLOWED_BIT 0x01
#define REGEX_PASSTHROUGH_BIT     0x02
#define REGEX_QMARK_BIT           0x04
#define REGEX_ANY_BIT             0x08
#define REGEX_BRACKET_BIT         0x10

#define GET_BIT(n, b) (n&b)
#define SET_BIT(n, b) n |= b
#define CLEAR_BIT(n, b) n &= ~(b)

typedef struct {
  fsm_t fsm;
  uint8_t flags;
  fsm_state_t prev_state;
} regex_t;

void regex_init(regex_t *regex) {
  fsm_init(&regex->fsm, 127);
  fsm_push_empty(&regex->fsm);
}

void regex_free(regex_t regex) {
  if (fsm_initialized(regex.fsm)) fsm_free(regex.fsm);
}

bool regex_compile_bracket(regex_t *regex, const char *pattern, const char **end);
bool regex_compile_expr(regex_t *regex, const char *pattern, const char **end) {
  switch(*pattern) {
  case '?': {
    if (!GET_BIT(regex->flags, REGEX_SPECIAL_ALLOWED_BIT)) return false;
    regex->flags = 0;
    SET_BIT(regex->flags, REGEX_PASSTHROUGH_BIT);
    SET_BIT(regex->flags, REGEX_QMARK_BIT);
  } break;
  case '*': {
    if (!GET_BIT(regex->flags, REGEX_SPECIAL_ALLOWED_BIT)) return false;
    fsm_state_t state = regex->fsm.count-1;
    for (fsm_event_t i = 32; i < 127; ++i) {
      if (fsm_get(regex->fsm, state, i) != 0) fsm_set(&regex->fsm, state, i, regex->prev_state);
    }
    regex->flags = 0;
    SET_BIT(regex->flags, REGEX_PASSTHROUGH_BIT);
  } break;
  case '+': {
    if (!GET_BIT(regex->flags, REGEX_SPECIAL_ALLOWED_BIT)) return false;
    fsm_state_t new_start = regex->fsm.count;
    for (fsm_state_t it = regex->prev_state; it < new_start; ++it) fsm_duplicate(&regex->fsm, it);
    fsm_state_t end_state = regex->fsm.count-1;
    for (fsm_event_t i = 32; i < 127; ++i) {
      for (fsm_state_t j = new_start; j < end_state; ++j) {
        fsm_state_t val = fsm_get(regex->fsm, j, i);
        if (val > 0) fsm_set(&regex->fsm, j, i, new_start+val-regex->prev_state);
      }
    }

    regex->flags = 0;
    regex->prev_state = new_start;
    SET_BIT(regex->flags, REGEX_PASSTHROUGH_BIT);
  } break;
  case '.': {
    if (GET_BIT(regex->flags, REGEX_QMARK_BIT)) return false;
    fsm_state_t state = regex->prev_state;
    if (!GET_BIT(regex->flags, REGEX_PASSTHROUGH_BIT)) state = fsm_push_empty(&regex->fsm);
    for (fsm_event_t i = 32; i < 127; ++i) fsm_set(&regex->fsm, state, i, regex->fsm.count);
    regex->flags = 0;
    regex->prev_state = state;
    SET_BIT(regex->flags, REGEX_ANY_BIT);
    SET_BIT(regex->flags, REGEX_SPECIAL_ALLOWED_BIT);
  } break;
  case '(': {
    if (GET_BIT(regex->flags, REGEX_BRACKET_BIT)) return false;
    if (!regex_compile_bracket(regex, ++pattern, &pattern)) return false;
    --pattern;
  } break;
  default: {
    fsm_state_t state = regex->prev_state;
    if (GET_BIT(regex->flags, REGEX_QMARK_BIT)) {
      fsm_set(&regex->fsm, state, *pattern, regex->fsm.count+1);
      state = fsm_push_empty(&regex->fsm);
    } else if (!GET_BIT(regex->flags, REGEX_PASSTHROUGH_BIT)) state = fsm_push_empty(&regex->fsm);
    fsm_set(&regex->fsm, state, *pattern, regex->fsm.count);
    regex->flags = 0;
    SET_BIT(regex->flags, REGEX_SPECIAL_ALLOWED_BIT);
    regex->prev_state = state;
  }
  }
  *end = ++pattern;
  return true;
}

bool regex_compile_bracket(regex_t *regex, const char *pattern, const char **end) {
  SET_BIT(regex->flags, REGEX_BRACKET_BIT);

  fsm_state_t base = regex->fsm.count;
  regex->prev_state = base;
  while (*pattern && *pattern != ')') {
    if (*pattern == '|') {
      SET_BIT(regex->flags, REGEX_PASSTHROUGH_BIT);
      ++pattern;
    } else if (!regex_compile_expr(regex, pattern, &pattern)) return false;
  }
  if (*pattern != ')') return false;

  CLEAR_BIT(regex->flags, REGEX_BRACKET_BIT);
  regex->prev_state = base;
  *end = ++pattern;
  return true;
}

bool regex_compile(regex_t *regex, const char *pattern) {
  while (*pattern) if (!regex_compile_expr(regex, pattern, &pattern)) return false;
  if (GET_BIT(regex->flags, REGEX_PASSTHROUGH_BIT)) fsm_set(&regex->fsm, regex->fsm.count-1, 0, regex->fsm.count);
  return true;
}

bool regex_match(regex_t *regex, const char *text) {
  regex->fsm.state = 1;
  while (*text && regex->fsm.state > 0
      && regex->fsm.state < regex->fsm.count)
    (void)fsm_fire_event(&regex->fsm, *(text++));

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
      .pattern = "a?bc",
      .text = "abc",
      .expected = true
    },
    (test_t){
      .pattern = "a?bc",
      .text = "bc",
      .expected = true
    },
    (test_t){
      .pattern = "abc?d",
      .text = "abcd",
      .expected = true
    },
    (test_t){
      .pattern = "abc?d",
      .text = "abd",
      .expected = true
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

    (test_t){
      .pattern = "(a)bc",
      .text = "abc",
      .expected = true
    },
    (test_t){
      .pattern = "(a)bc",
      .text = "bbc",
      .expected = false
    },
    (test_t){
      .pattern = "(a)bc",
      .text = "bc",
      .expected = false
    },
    (test_t){
      .pattern = "(a)*bc",
      .text = "bc",
      .expected = true
    },
    (test_t){
      .pattern = "(a)*bc",
      .text = "aaaabc",
      .expected = true
    },
    (test_t){
      .pattern = "(ab)+c",
      .text = "abc",
      .expected = true
    },
    (test_t){
      .pattern = "(ab)+c",
      .text = "abababc",
      .expected = true
    },
    (test_t){
      .pattern = "(ab)+c",
      .text = "ac",
      .expected = false
    },
    (test_t){
      .pattern = "(ab)+c",
      .text = "bc",
      .expected = false
    },
    (test_t){
      .pattern = "(ab)*c",
      .text = "c",
      .expected = true
    },
    (test_t){
      .pattern = "(ab)*c",
      .text = "abc",
      .expected = true
    },
    (test_t){
      .pattern = "(ab)*c",
      .text = "ac",
      .expected = false
    },
    (test_t){
      .pattern = "(ab)?c",
      .text = "abc",
      .expected = true
    },
    (test_t){
      .pattern = "(ab)?c",
      .text = "c",
      .expected = true
    },
    (test_t){
      .pattern = "(ab)?c",
      .text = "ac",
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