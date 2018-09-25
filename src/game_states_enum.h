#pragma once
enum GameStates{
#define GAME_STATE(name) GAME_STATE_##name,
#include "game_states.h"
};
