#define FSM_IMPLEMENTATION
#include "fsm.h"

const char *state_names[] = {
  "Locked",
  "Unlocked"
};

int main(void) {
  fsm_t fsm = {0};

  fsm_init(&fsm, 2);

  fsm_state_t state = fsm_push_empty(&fsm);
  fsm_set(&fsm, state, 0, 1);
  fsm_set(&fsm, state, 1, 0);
  state = fsm_push_empty(&fsm);
  fsm_set(&fsm, state, 0, 1);
  fsm_set(&fsm, state, 1, 0);

  while (1) {
    char cmd[128] = {0};
    if (!fgets(cmd, 128, stdin)) break;
    cmd[strlen(cmd)-1] = 0;

    if      (strcmp(cmd, "quit") == 0) break;
    else if (strcmp(cmd, "coin") == 0) fsm_fire_event(&fsm, 0);
    else if (strcmp(cmd, "push") == 0) fsm_fire_event(&fsm, 1);
    else {
      printf("Unknown command \"%s\"\n", cmd);
      continue;
    }
  }

  return 0;
}