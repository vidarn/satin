#include "engine.h"
#define GAME_STATE(update_fn, init_fn) void update_fn(int,struct InputState,\
    struct GameData*);\
    void init_fn(struct GameData*);
#include "game_states.h"
#undef GAME_STATE

struct GameState game_states[] = {
#define GAME_STATE(update_fn, init_fn) {update_fn,init_fn},
#include "game_states.h"
#undef GAME_STATE
};


const int num_game_states = sizeof(game_states)/sizeof(*game_states);

