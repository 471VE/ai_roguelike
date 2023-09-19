#include "roguelike.h"
#include "ecsTypes.h"
#include "raylib.h"
#include "stateMachine.h"
#include "aiLibrary.h"

#define BERSEKER_COLOR GetColor(0xff2222ff) // bright red
#define MONSTER_HEALER_COLOR GetColor(0x005500ff) // dark green
#define SWORDSMAN_HEALER_COLOR GetColor(0x00ffffff) // cyan
#define CRAFTER_COLOR GetColor(0xffa000ff) // orange

#define HP_THRESHOLD 60.f
#define SIGHT_NEIGHBORHOOD 3.f
#define SIGHT_LOSING_DISTANCE 5.f

enum AvailableTeams
{
  PLAYER_TEAM,
  ENEMY_TEAM
};

static void add_berserker(flecs::entity entity)
{
  entity.get([](StateMachine &sm)
  {
    int patrol = sm.addState(create_patrol_state(SIGHT_NEIGHBORHOOD));
    int moveToEnemy = sm.addState(create_move_to_enemy_state());

    sm.addTransition(create_enemy_available_transition(SIGHT_NEIGHBORHOOD), patrol, moveToEnemy);
    sm.addTransition(create_negate_transition(
      create_or_transition(create_enemy_available_transition(SIGHT_LOSING_DISTANCE), create_hitpoints_less_than_transition(HP_THRESHOLD))
      ), moveToEnemy, patrol);
  });
}

static void add_monster_healer(flecs::entity entity)
{
  entity.get([](StateMachine &sm)
  {
    int patrol = sm.addState(create_patrol_state(SIGHT_NEIGHBORHOOD));
    int moveToEnemy = sm.addState(create_move_to_enemy_state());
    int heal = sm.addState(create_heal_self_state());

    sm.addTransition(create_enemy_available_transition(SIGHT_NEIGHBORHOOD), patrol, moveToEnemy);
    sm.addTransition(create_negate_transition(create_enemy_available_transition(SIGHT_LOSING_DISTANCE)), moveToEnemy, patrol);

    sm.addTransition(create_hitpoints_less_than_transition(HP_THRESHOLD), moveToEnemy, heal);
    sm.addTransition(create_negate_transition(
      create_or_transition(create_enemy_available_transition(SIGHT_LOSING_DISTANCE), create_hitpoints_less_than_transition(HP_THRESHOLD))
    ), heal, patrol);
    sm.addTransition(create_and_transition(
      create_negate_transition(create_hitpoints_less_than_transition(HP_THRESHOLD)), create_enemy_available_transition(5.f)
    ), heal, moveToEnemy);
  });
}

static void add_swordsman_healer(flecs::entity entity)
{
  entity.get([](StateMachine &sm)
  {
    int moveToPlayer = sm.addState(create_move_to_player_state());
    int moveToEnemy = sm.addState(create_move_to_enemy_state());
    int healPlayer = sm.addState(create_heal_player_state());

    sm.addTransition(create_enemy_available_transition(SIGHT_NEIGHBORHOOD), moveToPlayer, moveToEnemy);
    sm.addTransition(create_negate_transition(create_enemy_available_transition(SIGHT_LOSING_DISTANCE)), moveToEnemy, moveToPlayer);

    sm.addTransition(create_and_transition(
      create_player_hitpoints_less_than_transition(HP_THRESHOLD),
      create_player_available_transition(1.f),
      create_negate_transition(create_player_healing_cooldown_transition())), moveToPlayer, healPlayer);
    sm.addTransition(create_or_transition(
      create_negate_transition(create_player_hitpoints_less_than_transition(HP_THRESHOLD)),
      create_negate_transition(create_player_available_transition(1.f)),
      create_player_healing_cooldown_transition()), healPlayer, moveToPlayer);
  });
}

static void add_crafter(flecs::entity entity)
{
  entity.get([](StateMachine &sm)
  {
    StateMachine *workStateMachine = new StateMachine();
    {
      int moveToNextPosition = workStateMachine->addState(create_move_to_next_position_state());
      int plantHeal = workStateMachine->addState(create_plant_heal_state());

      workStateMachine->addTransition(create_at_next_position_transition(), moveToNextPosition, plantHeal);
      workStateMachine->addTransition(create_always_true_transition(), plantHeal, moveToNextPosition);
    }

    StateMachine *restStateMachine = new StateMachine();
    {
      int moveToRestingBase = restStateMachine->addState(create_move_to_resting_base_state());
      int sleeping = restStateMachine->addState(create_sleeping_state());

      restStateMachine->addTransition(create_at_resting_base_transition(), moveToRestingBase, sleeping);
    }

    StateMachine *peacefulLifeStateMachine = new StateMachine();
    {
      int work = peacefulLifeStateMachine->addStateFromStateMachine(workStateMachine);
      int rest = peacefulLifeStateMachine->addStateFromStateMachine(restStateMachine);

      peacefulLifeStateMachine->addTransition(create_work_done_transition(), work, rest);
      peacefulLifeStateMachine->addTransition(create_finished_sleeping_transition(), rest, work);
    }

    int fleeFromEnemy = sm.addState(create_flee_from_enemy_state());
    int peacefulLife = sm.addStateFromStateMachine(peacefulLifeStateMachine);

    sm.addTransition(create_enemy_available_transition(SIGHT_NEIGHBORHOOD), peacefulLife, fleeFromEnemy);
    sm.addTransition(create_negate_transition(create_enemy_available_transition(SIGHT_LOSING_DISTANCE)), fleeFromEnemy, peacefulLife);
  });
}

static void add_patrol_attack_flee_sm(flecs::entity entity)
{
  entity.get([](StateMachine &sm)
  {
    int patrol = sm.addState(create_patrol_state(3.f));
    int moveToEnemy = sm.addState(create_move_to_enemy_state());
    int fleeFromEnemy = sm.addState(create_flee_from_enemy_state());

    sm.addTransition(create_enemy_available_transition(3.f), patrol, moveToEnemy);
    sm.addTransition(create_negate_transition(create_enemy_available_transition(5.f)), moveToEnemy, patrol);

    sm.addTransition(create_and_transition(create_hitpoints_less_than_transition(60.f), create_enemy_available_transition(5.f)),
                     moveToEnemy, fleeFromEnemy);
    sm.addTransition(create_and_transition(create_hitpoints_less_than_transition(60.f), create_enemy_available_transition(3.f)),
                     patrol, fleeFromEnemy);

    sm.addTransition(create_negate_transition(create_enemy_available_transition(7.f)), fleeFromEnemy, patrol);
  });
}

static void add_patrol_flee_sm(flecs::entity entity)
{
  entity.get([](StateMachine &sm)
  {
    int patrol = sm.addState(create_patrol_state(3.f));
    int fleeFromEnemy = sm.addState(create_flee_from_enemy_state());

    sm.addTransition(create_enemy_available_transition(3.f), patrol, fleeFromEnemy);
    sm.addTransition(create_negate_transition(create_enemy_available_transition(5.f)), fleeFromEnemy, patrol);
  });
}

static void add_attack_sm(flecs::entity entity)
{
  entity.get([](StateMachine &sm)
  {
    sm.addState(create_move_to_enemy_state());
  });
}

static flecs::entity create_monster(flecs::world &ecs, int x, int y, Color color, AvailableTeams team)
{
  return ecs.entity()
    .set(Position{x, y})
    .set(MovePos{x, y})
    .set(PatrolPos{x, y})
    .set(Hitpoints{100.f})
    .set(Action{EA_NOP})
    .set(Color{color})
    .set(StateMachine{})
    .set(Team{team})
    .set(NumActions{1, 0})
    .set(MeleeDamage{20.f});
}

static void create_player(flecs::world &ecs, int x, int y)
{
  ecs.entity("player")
    .set(Position{x, y})
    .set(MovePos{x, y})
    .set(Hitpoints{100.f})
    .set(GetColor(0xeeeeeeff))
    .set(Action{EA_NOP})
    .add<IsPlayer>()
    .set(Team{0})
    .set(PlayerInput{})
    .set(NumActions{2, 0})
    .set(MeleeDamage{50.f})
    .set(PlayerHealingCooldown{0});
}

static void create_heal(flecs::world &ecs, int x, int y, float amount)
{
  ecs.entity()
    .set(Position{x, y})
    .set(HealAmount{amount})
    .set(GetColor(0x44ff44ff));
}

static void create_powerup(flecs::world &ecs, int x, int y, float amount)
{
  ecs.entity()
    .set(Position{x, y})
    .set(PowerupAmount{amount})
    .set(Color{255, 255, 0, 255});
}

static void register_roguelike_systems(flecs::world &ecs)
{
  ecs.system<PlayerInput, Action, const IsPlayer>()
    .each([&](PlayerInput &inp, Action &a, const IsPlayer)
    {
      bool left = IsKeyDown(KEY_LEFT);
      bool right = IsKeyDown(KEY_RIGHT);
      bool up = IsKeyDown(KEY_UP);
      bool down = IsKeyDown(KEY_DOWN);
      if (left && !inp.left)
        a.action = EA_MOVE_LEFT;
      if (right && !inp.right)
        a.action = EA_MOVE_RIGHT;
      if (up && !inp.up)
        a.action = EA_MOVE_UP;
      if (down && !inp.down)
        a.action = EA_MOVE_DOWN;
      inp.left = left;
      inp.right = right;
      inp.up = up;
      inp.down = down;
    });
  ecs.system<const Position, const Color>()
    .term<TextureSource>(flecs::Wildcard).not_()
    .each([&](const Position &pos, const Color color)
    {
      const Rectangle rect = {float(pos.x), float(pos.y), 1, 1};
      DrawRectangleRec(rect, color);
    });
  ecs.system<const Position, const Color>()
    .term<TextureSource>(flecs::Wildcard)
    .each([&](flecs::entity e, const Position &pos, const Color color)
    {
      const auto textureSrc = e.target<TextureSource>();
      DrawTextureQuad(*textureSrc.get<Texture2D>(),
          Vector2{1, 1}, Vector2{0, 0},
          Rectangle{float(pos.x), float(pos.y), 1, 1}, color);
    });
}


void init_roguelike(flecs::world &ecs)
{
  register_roguelike_systems(ecs);

  add_patrol_attack_flee_sm(create_monster(ecs, 5, 5, GetColor(0xee00eeff), ENEMY_TEAM));
  add_patrol_attack_flee_sm(create_monster(ecs, 10, -5, GetColor(0xee00eeff), ENEMY_TEAM));
  add_patrol_flee_sm(create_monster(ecs, -5, -5, GetColor(0x111111ff), ENEMY_TEAM));
  add_attack_sm(create_monster(ecs, -5, 5, GetColor(0x880000ff), ENEMY_TEAM));
  add_berserker(create_monster(ecs, -2, 5, BERSEKER_COLOR, ENEMY_TEAM));
  add_monster_healer(create_monster(ecs, 2, 5, MONSTER_HEALER_COLOR, ENEMY_TEAM)
    .set(EntityHealingAmount{50.f})
    .add<CanHeal>()
  );
  add_swordsman_healer(create_monster(ecs, 0, -1, SWORDSMAN_HEALER_COLOR, PLAYER_TEAM)
    .set(PlayerHealingAbility{200.f, 10})
    .add<CanHeal>()
  );
  add_crafter(create_monster(ecs, 10, 0, CRAFTER_COLOR, ENEMY_TEAM)
    .set(RestingBase{7, 0})
    .set(SleepTimer{5, 0})
    .set(NextHealPosition{10, 5})
    .set(NumHealsPlanted{0, 3})
  );

  create_player(ecs, 0, 0);

  create_powerup(ecs, 7, 7, 10.f);
  create_powerup(ecs, 10, -6, 10.f);
  create_powerup(ecs, 10, -4, 10.f);

  create_heal(ecs, -5, -5, 50.f);
  create_heal(ecs, -5, 5, 50.f);
}

static bool is_player_acted(flecs::world &ecs)
{
  static auto processPlayer = ecs.query<const IsPlayer, const Action>();
  bool playerActed = false;
  processPlayer.each([&](const IsPlayer, const Action &a)
  {
    playerActed = a.action != EA_NOP;
  });
  return playerActed;
}

static bool upd_player_actions_count(flecs::world &ecs)
{
  static auto updPlayerActions = ecs.query<const IsPlayer, NumActions>();
  bool actionsReached = false;
  updPlayerActions.each([&](const IsPlayer, NumActions &na)
  {
    na.curActions = (na.curActions + 1) % na.numActions;
    actionsReached |= na.curActions == 0;
  });
  return actionsReached;
}

static Position move_pos(Position pos, int action)
{
  if (action == EA_MOVE_LEFT)
    pos.x--;
  else if (action == EA_MOVE_RIGHT)
    pos.x++;
  else if (action == EA_MOVE_UP)
    pos.y--;
  else if (action == EA_MOVE_DOWN)
    pos.y++;
  return pos;
}

static void process_actions(flecs::world &ecs)
{
  static auto processActions = ecs.query<Action, Position, MovePos, const MeleeDamage, const Team>();
  static auto checkAttacks = ecs.query<const MovePos, Hitpoints, const Team>();
  static auto playerCooldown = ecs.query<const IsPlayer, Hitpoints, PlayerHealingCooldown>();
  static auto healing = ecs.query<const Action, const CanHeal>();
  static auto healPlanting = ecs.query<const Action, const Position, const NextHealPosition, NumHealsPlanted>();
  static auto sleeping = ecs.query<const Action, SleepTimer>();
  // Process all actions
  ecs.defer([&]
  {
    // process sleep first
    sleeping.each([&](flecs::entity entity, const Action &a, SleepTimer &timer)
    {
      if (a.action == EA_SLEEP)
        timer.timeLeft--;
    });
    // then planting heals
    healPlanting.each([&](flecs::entity entity, const Action &a, const Position &pos, const NextHealPosition &, NumHealsPlanted &numheals)
    {
      if (a.action == EA_PLANT_HEAL)
      {
        create_heal(ecs, pos.x, pos.y, 50.f);
        numheals.numHealsPlanted++;
      }
    });
    // then attacks
    processActions.each([&](flecs::entity entity, Action &a, Position &pos, MovePos &mpos, const MeleeDamage &dmg, const Team &team)
    {      
      Position nextPos = move_pos(pos, a.action);
      bool blocked = false;
      checkAttacks.each([&](flecs::entity enemy, const MovePos &epos, Hitpoints &hp, const Team &enemy_team)
      {
        if (entity != enemy && epos == nextPos)
        {
          blocked = true;
          if (team.team != enemy_team.team)
            hp.hitpoints -= dmg.damage;
        }
      });
      if (blocked)
        a.action = EA_NOP;
      else
        mpos = nextPos;
    });
    // then healings
    healing.each([&](flecs::entity entity, const Action &a, const CanHeal &)
    {
      if (a.action == EA_HEAL)
        entity.set([&](const EntityHealingAmount &heal, Hitpoints &hp)
        {
          hp.hitpoints += heal.healingAmount;
        });
      else if (a.action == EA_HEAL_PLAYER)
        entity.set([&](const PlayerHealingAbility &heal)
        {
          playerCooldown.each([&](const IsPlayer&, Hitpoints &hp, PlayerHealingCooldown &cooldown)
          {
            hp.hitpoints += heal.healingAmount;
            cooldown.cooldown += heal.cooldown;
          });
        });
    });
    // now move
    processActions.each([&](flecs::entity entity, Action &a, Position &pos, MovePos &mpos, const MeleeDamage &, const Team&)
    {
      pos = mpos;
      a.action = EA_NOP;
    });
  });

  static auto deleteAllDead = ecs.query<const Hitpoints>();
  ecs.defer([&]
  {
    deleteAllDead.each([&](flecs::entity entity, const Hitpoints &hp)
    {
      if (hp.hitpoints <= 0.f)
        entity.destruct();
    });
  });

  static auto playerPickup = ecs.query<const IsPlayer, const Position, Hitpoints, MeleeDamage>();
  static auto healPickup = ecs.query<const Position, const HealAmount>();
  static auto powerupPickup = ecs.query<const Position, const PowerupAmount>();
  ecs.defer([&]
  {
    playerPickup.each([&](const IsPlayer&, const Position &pos, Hitpoints &hp, MeleeDamage &dmg)
    {
      healPickup.each([&](flecs::entity entity, const Position &ppos, const HealAmount &amt)
      {
        if (pos == ppos)
        {
          hp.hitpoints += amt.amount;
          entity.destruct();
        }
      });
      powerupPickup.each([&](flecs::entity entity, const Position &ppos, const PowerupAmount &amt)
      {
        if (pos == ppos)
        {
          dmg.damage += amt.amount;
          entity.destruct();
        }
      });
    });
  });
}

void process_turn(flecs::world &ecs)
{
  static auto stateMachineAct = ecs.query<StateMachine>();
  if (is_player_acted(ecs))
  {
    ecs.query<const IsPlayer, PlayerHealingCooldown>().each([](const IsPlayer&, PlayerHealingCooldown &cooldown)
    {
      if (cooldown.cooldown)
        cooldown.cooldown--;
    });
    if (upd_player_actions_count(ecs))
    {
      // Plan action for NPCs
      ecs.defer([&]
      {
        stateMachineAct.each([&](flecs::entity e, StateMachine &sm)
        {
          sm.act(0.f, ecs, e);
        });
      });
    }
    process_actions(ecs);
  }
}

void print_stats(flecs::world &ecs)
{
  static auto playerStatsQuery = ecs.query<const IsPlayer, const Hitpoints, const MeleeDamage, const PlayerHealingCooldown>();
  playerStatsQuery.each([&](const IsPlayer &, const Hitpoints &hp, const MeleeDamage &dmg, const PlayerHealingCooldown &cooldown)
  {
    DrawText(TextFormat("hp: %d", int(hp.hitpoints)), 20, 20, 20, WHITE);
    DrawText(TextFormat("power: %d", int(dmg.damage)), 20, 40, 20, WHITE);
    if (cooldown.cooldown)
      DrawText(TextFormat("healing cooldown: %d", cooldown.cooldown), 20, 60, 20, WHITE);
  });
}

