#include "aiLibrary.h"
#include <flecs.h>
#include "ecsTypes.h"
#include "raylib.h"
#include <cfloat>
#include <cmath>

class AttackEnemyState : public State
{
public:
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, flecs::world &/*ecs*/, flecs::entity /*entity*/) const override {}
};

template<typename T>
T sqr(T a){ return a*a; }

template<typename T, typename U>
static float dist_sq(const T &lhs, const U &rhs) { return float(sqr(lhs.x - rhs.x) + sqr(lhs.y - rhs.y)); }

template<typename T, typename U>
static float dist(const T &lhs, const U &rhs) { return sqrtf(dist_sq(lhs, rhs)); }

template<typename T, typename U>
static int move_towards(const T &from, const U &to)
{
  int deltaX = to.x - from.x;
  int deltaY = to.y - from.y;
  if (abs(deltaX) > abs(deltaY))
    return deltaX > 0 ? EA_MOVE_RIGHT : EA_MOVE_LEFT;
  return deltaY < 0 ? EA_MOVE_UP : EA_MOVE_DOWN;
}

static int inverse_move(int move)
{
  return move == EA_MOVE_LEFT ? EA_MOVE_RIGHT :
         move == EA_MOVE_RIGHT ? EA_MOVE_LEFT :
         move == EA_MOVE_UP ? EA_MOVE_DOWN :
         move == EA_MOVE_DOWN ? EA_MOVE_UP : move;
}


template<typename Callable>
static void on_closest_enemy_pos(flecs::world &ecs, flecs::entity entity, Callable c)
{
  static auto enemiesQuery = ecs.query<const Position, const Team>();
  entity.set([&](const Position &pos, const Team &t, Action &a)
  {
    flecs::entity closestEnemy;
    float closestDist = FLT_MAX;
    Position closestPos;
    enemiesQuery.each([&](flecs::entity enemy, const Position &epos, const Team &et)
    {
      if (t.team == et.team)
        return;
      float curDist = dist(epos, pos);
      if (curDist < closestDist)
      {
        closestDist = curDist;
        closestPos = epos;
        closestEnemy = enemy;
      }
    });
    if (ecs.is_valid(closestEnemy))
      c(a, pos, closestPos);
  });
}

template<typename Callable>
static void on_player_pos(flecs::world &ecs, flecs::entity entity, Callable c)
{
  entity.set([&](const Position &pos, Action &a)
  {
    ecs.query<const IsPlayer, const Position>().each([&](flecs::entity player, const IsPlayer&, const Position &ppos)
    {
    if (ecs.is_valid(player))
      c(a, pos, ppos);
    });
  });
}

class MoveToEnemyState : public State
{
public:
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, flecs::world &ecs, flecs::entity entity) const override
  {
    on_closest_enemy_pos(ecs, entity, [&](Action &a, const Position &pos, const Position &enemy_pos)
    {
      a.action = move_towards(pos, enemy_pos);
    });
  }
};

class FleeFromEnemyState : public State
{
public:
  FleeFromEnemyState() {}
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, flecs::world &ecs, flecs::entity entity) const override
  {
    on_closest_enemy_pos(ecs, entity, [&](Action &a, const Position &pos, const Position &enemy_pos)
    {
      a.action = inverse_move(move_towards(pos, enemy_pos));
    });
  }
};

class PatrolState : public State
{
  float patrolDist;
public:
  PatrolState(float dist) : patrolDist(dist) {}
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, flecs::world &ecs, flecs::entity entity) const override
  {
    entity.set([&](const Position &pos, const PatrolPos &ppos, Action &a)
    {
      if (dist(pos, ppos) > patrolDist)
        a.action = move_towards(pos, ppos); // do a recovery walk
      else
      {
        // do a random walk
        a.action = GetRandomValue(EA_MOVE_START, EA_MOVE_END - 1);
      }
    });
  }
};

class NopState : public State
{
public:
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, flecs::world &ecs, flecs::entity entity) const override {}
};

class HealSelfState : public State
{
public:
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, flecs::world &, flecs::entity entity) const override
  {
    entity.set([&](Action &a)
    {
      a.action = EA_HEAL;
    });
  }
};

class MoveToPlayerState : public State
{
public:
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, flecs::world &ecs, flecs::entity entity) const override
  {
    on_player_pos(ecs, entity, [&](Action &a, const Position &pos, const Position &player_pos)
    {
      a.action = move_towards(pos, player_pos);
    });
  }
};

class HealPlayerState : public State
{
public:
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, flecs::world &, flecs::entity entity) const override
  {
    entity.set([&](Action &a)
    {
      a.action = EA_HEAL_PLAYER;
    });
  }
};

class MoveToRestingBaseState : public State
{
public:
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, flecs::world &, flecs::entity entity) const override
  {
    entity.set([&](Action &a, const Position &pos, const RestingBase &base)
    {
      a.action = move_towards(pos, Position(base));
    });
  }
};

class SleepingState : public State
{
public:
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, flecs::world &, flecs::entity entity) const override
  {
    entity.set([&](Action &a)
    {
      a.action = EA_SLEEP;
    });
  }
};

class MoveToNextPositionState : public State
{
public:
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, flecs::world &, flecs::entity entity) const override
  {
    entity.set([&](Action &a, const Position &pos, const NextHealPosition &npos)
    {
      a.action = move_towards(pos, Position(npos));
    });
  }
};

class PlantHealState : public State
{
public:
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, flecs::world &, flecs::entity entity) const override
  {
    entity.set([&](Action &a, const Position &pos, NextHealPosition &npos)
    {
      if (pos != npos)
        return;
      a.action = EA_PLANT_HEAL;
      npos = {GetRandomValue(5, 10), GetRandomValue(5, 10)};
    });
  }
};

class EnemyAvailableTransition : public StateTransition
{
  float triggerDist;
public:
  EnemyAvailableTransition(float in_dist) : triggerDist(in_dist) {}
  bool isAvailable(flecs::world &ecs, flecs::entity entity) const override
  {
    static auto enemiesQuery = ecs.query<const Position, const Team>();
    bool enemiesFound = false;
    entity.get([&](const Position &pos, const Team &t)
    {
      enemiesQuery.each([&](flecs::entity enemy, const Position &epos, const Team &et)
      {
        if (t.team == et.team)
          return;
        float curDist = dist(epos, pos);
        enemiesFound |= curDist <= triggerDist;
      });
    });
    return enemiesFound;
  }
};

class HitpointsLessThanTransition : public StateTransition
{
  float threshold;
public:
  HitpointsLessThanTransition(float in_thres) : threshold(in_thres) {}
  bool isAvailable(flecs::world &ecs, flecs::entity entity) const override
  {
    bool hitpointsThresholdReached = false;
    entity.get([&](const Hitpoints &hp)
    {
      hitpointsThresholdReached |= hp.hitpoints < threshold;
    });
    return hitpointsThresholdReached;
  }
};

class PlayerAvailableTransition : public StateTransition
{
  float triggerDist;
public:
  PlayerAvailableTransition(float in_dist) : triggerDist(in_dist) {}
  bool isAvailable(flecs::world &ecs, flecs::entity entity) const override
  {
    bool playerFound = false;
    entity.get([&](const Position &pos)
    {
      ecs.query<const IsPlayer, const Position>().each([&](const IsPlayer &, const Position &ppos)
      {
        float curDist = dist(ppos, pos);
        playerFound |= curDist <= triggerDist;
      });
    });
    return playerFound;
  }
};

class PlayerHitpointsLessThanTransition : public StateTransition
{
  float threshold;
public:
  PlayerHitpointsLessThanTransition(float in_thres) : threshold(in_thres) {}
  bool isAvailable(flecs::world &ecs, flecs::entity) const override
  {
    bool hitpointsThresholdReached = false;
    ecs.query<const IsPlayer, const Hitpoints>().each([&](const IsPlayer &, const Hitpoints &hp)
    {
      hitpointsThresholdReached |= hp.hitpoints < threshold;
    });
    return hitpointsThresholdReached;
  }
};

class PlayerHealingCooldownTransition : public StateTransition
{
public:
  PlayerHealingCooldownTransition() {}
  bool isAvailable(flecs::world &ecs, flecs::entity) const override
  {
    bool waitForCooldown = false;
    ecs.query<const IsPlayer, const PlayerHealingCooldown>().each([&](const IsPlayer &, const PlayerHealingCooldown &cooldown)
    {
      waitForCooldown = cooldown.cooldown > 0;
    });
    return waitForCooldown;
  }
};

class AtRestingBaseTransition : public StateTransition
{
public:
  AtRestingBaseTransition() {}
  bool isAvailable(flecs::world &, flecs::entity entity) const override
  {
    bool atRestingBase = false;
    entity.get([&](const Position &pos, const RestingBase &npos)
    {
      atRestingBase = pos == npos;
      if (atRestingBase)
        entity.set([&](SleepTimer &timer)
        {
          if (!timer.timeLeft) // meaning that if sleep was interrupted, we do not reset sleep timer
            timer.timeLeft = timer.timer;
          entity.add<ShouldSleep>();
        });
    });
    return atRestingBase;
  }
};

class AtNextHealPositionTransition : public StateTransition
{
public:
  AtNextHealPositionTransition() {}
  bool isAvailable(flecs::world &, flecs::entity entity) const override
  {
    bool atNextHealPosition = false;
    entity.get([&](const Position &pos, const NextHealPosition &npos)
    {
      atNextHealPosition = pos == npos;
    });
    return atNextHealPosition;
  }
};

class AlwaysTrueTransition : public StateTransition
{
public:
  AlwaysTrueTransition() {}
  bool isAvailable(flecs::world &, flecs::entity) const override { return true; }
};

class WorkDoneTransition : public StateTransition
{
public:
  WorkDoneTransition() {}
  bool isAvailable(flecs::world &, flecs::entity entity) const override
  {
    bool workDone = false;
    entity.get([&](const NumHealsPlanted &nm)
    {
      workDone = nm.numHealsNeeded == nm.numHealsPlanted;
    });
    return workDone;
  }
};

class FinishedSleepingTransition : public StateTransition
{
public:
  FinishedSleepingTransition() {}
  bool isAvailable(flecs::world &, flecs::entity entity) const override
  {
    bool finishedSleeping = false;
    if (entity.has<ShouldSleep>())
    {
      entity.set([&](NumHealsPlanted &nm, const SleepTimer &timer)
      {
        finishedSleeping = timer.timeLeft == 0;
        if (finishedSleeping)
        {
          nm.numHealsPlanted = 0;
          entity.remove<ShouldSleep>();
        }
    });
    }
    return finishedSleeping;
  }
};

class EnemyReachableTransition : public StateTransition
{
public:
  bool isAvailable(flecs::world &ecs, flecs::entity entity) const override
  {
    return false;
  }
};

class NegateTransition : public StateTransition
{
  const StateTransition *transition; // we own it
public:
  NegateTransition(const StateTransition *in_trans) : transition(in_trans) {}
  ~NegateTransition() override { delete transition; }

  bool isAvailable(flecs::world &ecs, flecs::entity entity) const override
  {
    return !transition->isAvailable(ecs, entity);
  }
};

class AndTransition : public StateTransition
{
  const StateTransition *lhs; // we own it
  const StateTransition *rhs; // we own it
public:
  AndTransition(const StateTransition *in_lhs, const StateTransition *in_rhs) : lhs(in_lhs), rhs(in_rhs) {}
  ~AndTransition() override
  {
    delete lhs;
    delete rhs;
  }

  bool isAvailable(flecs::world &ecs, flecs::entity entity) const override
  {
    return lhs->isAvailable(ecs, entity) && rhs->isAvailable(ecs, entity);
  }
};

class OrTransition : public StateTransition
{
  const StateTransition *lhs; // we own it
  const StateTransition *rhs; // we own it
public:
  OrTransition(const StateTransition *in_lhs, const StateTransition *in_rhs) : lhs(in_lhs), rhs(in_rhs) {}
  ~OrTransition() override
  {
    delete lhs;
    delete rhs;
  }

  bool isAvailable(flecs::world &ecs, flecs::entity entity) const override
  {
    return lhs->isAvailable(ecs, entity) || rhs->isAvailable(ecs, entity);
  }
};


// states
State *create_attack_enemy_state()
{
  return new AttackEnemyState();
}
State *create_move_to_enemy_state()
{
  return new MoveToEnemyState();
}

State *create_flee_from_enemy_state()
{
  return new FleeFromEnemyState();
}


State *create_patrol_state(float patrol_dist)
{
  return new PatrolState(patrol_dist);
}

State *create_nop_state()
{
  return new NopState();
}

State *create_heal_self_state()
{
  return new HealSelfState();
}

State *create_move_to_player_state()
{
  return new MoveToPlayerState();
}

State *create_heal_player_state()
{
  return new HealPlayerState();
}

State *create_move_to_resting_base_state()
{
  return new MoveToRestingBaseState();
}

State *create_sleeping_state()
{
  return new SleepingState();
}

State *create_move_to_next_position_state()
{
  return new MoveToNextPositionState();
}

State *create_plant_heal_state()
{
  return new PlantHealState();
}

StateTransition *create_enemy_available_transition(float dist)
{
  return new EnemyAvailableTransition(dist);
}

StateTransition *create_enemy_reachable_transition()
{
  return new EnemyReachableTransition();
}

StateTransition *create_hitpoints_less_than_transition(float thres)
{
  return new HitpointsLessThanTransition(thres);
}

StateTransition *create_player_hitpoints_less_than_transition(float thres)
{
  return new PlayerHitpointsLessThanTransition(thres);
}

StateTransition *create_player_healing_cooldown_transition()
{
  return new PlayerHealingCooldownTransition();
}

StateTransition *create_player_available_transition(float dist)
{
  return new PlayerAvailableTransition(dist);
}

StateTransition *create_at_resting_base_transition()
{
  return new AtRestingBaseTransition();
}

StateTransition *create_at_next_position_transition()
{
  return new AtNextHealPositionTransition();
}

StateTransition *create_always_true_transition()
{
  return new AlwaysTrueTransition();
}

StateTransition *create_work_done_transition()
{
  return new WorkDoneTransition();
}

StateTransition *create_finished_sleeping_transition()
{
  return new FinishedSleepingTransition();
}

StateTransition *create_negate_transition(StateTransition *in)
{
  return new NegateTransition(in);
}

StateTransition *create_and_transition(StateTransition *lhs, StateTransition *rhs)
{
  return new AndTransition(lhs, rhs);
}

StateTransition *create_and_transition(StateTransition *st1, StateTransition *st2, StateTransition *st3)
{
  return create_and_transition(create_and_transition(st1, st2), st3);
}

StateTransition *create_or_transition(StateTransition *lhs, StateTransition *rhs)
{
  return new OrTransition(lhs, rhs);
}

StateTransition *create_or_transition(StateTransition *st1, StateTransition *st2, StateTransition *st3)
{
  return create_or_transition(create_or_transition(st1, st2), st3);
}
