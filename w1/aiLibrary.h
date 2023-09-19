#pragma once

#include "stateMachine.h"

// states
State *create_attack_enemy_state();
State *create_move_to_enemy_state();
State *create_flee_from_enemy_state();
State *create_patrol_state(float patrol_dist);
State *create_nop_state();
State *create_heal_self_state();
State *create_move_to_player_state();
State *create_heal_player_state();
State *create_move_to_resting_base_state();
State *create_sleeping_state();
State *create_move_to_next_position_state();
State *create_plant_heal_state();

// transitions
StateTransition *create_enemy_available_transition(float dist);
StateTransition *create_enemy_reachable_transition();
StateTransition *create_hitpoints_less_than_transition(float thres);
StateTransition *create_negate_transition(StateTransition *in);
StateTransition *create_player_hitpoints_less_than_transition(float thres);
StateTransition *create_player_healing_cooldown_transition();
StateTransition *create_player_available_transition(float dist);
StateTransition *create_at_resting_base_transition();
StateTransition *create_at_next_position_transition();
StateTransition *create_always_true_transition();
StateTransition *create_work_done_transition();
StateTransition *create_finished_sleeping_transition();

StateTransition *create_and_transition(StateTransition *lhs, StateTransition *rhs);
StateTransition *create_and_transition(StateTransition *st1, StateTransition *st2, StateTransition *st3);
StateTransition *create_or_transition(StateTransition *lhs, StateTransition *rhs);
StateTransition *create_or_transition(StateTransition *st1, StateTransition *st2, StateTransition *st3);

