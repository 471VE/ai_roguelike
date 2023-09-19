#pragma once

struct Position;
struct MovePos;

struct RestingBase
{
  int x = 7;
  int y = 8;
};

struct NextHealPosition
{
  int x = 0;
  int y = 0;
};

struct MovePos
{
  int x = 0;
  int y = 0;

  MovePos &operator=(const Position &rhs);
};

struct Position
{
  int x = 0;
  int y = 0;

  Position &operator=(const MovePos &rhs);
  Position() = default;
  Position(int _x, int _y) : x(_x), y(_y) {}
  Position(const RestingBase &base) : x(base.x), y(base.y) {}
  Position(const NextHealPosition &npos) : x(npos.x), y(npos.y) {}
};

inline Position &Position::operator=(const MovePos &rhs)
{
  x = rhs.x;
  y = rhs.y;
  return *this;
}

inline MovePos &MovePos::operator=(const Position &rhs)
{
  x = rhs.x;
  y = rhs.y;
  return *this;
}

inline bool operator==(const Position &lhs, const Position &rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
inline bool operator==(const Position &lhs, const MovePos &rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
inline bool operator==(const MovePos &lhs, const MovePos &rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
inline bool operator==(const MovePos &lhs, const Position &rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
inline bool operator==(const Position &lhs, const NextHealPosition &rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
inline bool operator==(const NextHealPosition &lhs, const MovePos &rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
inline bool operator==(const Position &lhs, const RestingBase &rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
inline bool operator==(const RestingBase &lhs, const MovePos &rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }


struct PatrolPos
{
  int x = 0;
  int y = 0;
};

struct Hitpoints
{
  float hitpoints = 10.f;
};

enum Actions
{
  EA_NOP = 0,
  EA_MOVE_START,
  EA_MOVE_LEFT = EA_MOVE_START,
  EA_MOVE_RIGHT,
  EA_MOVE_DOWN,
  EA_MOVE_UP,
  EA_MOVE_END,
  EA_ATTACK = EA_MOVE_END,
  EA_HEAL,
  EA_HEAL_PLAYER,
  EA_PLANT_HEAL,
  EA_SLEEP,
  EA_NUM
};

struct Action
{
  int action = 0;
};

struct NumActions
{
  int numActions = 1;
  int curActions = 0;
};

struct MeleeDamage
{
  float damage = 2.f;
};

struct EntityHealingAmount
{
  float healingAmount = 0.f;
};

struct PlayerHealingAbility
{
  float healingAmount = 200.f;
  int cooldown = 10;
};

struct PlayerHealingCooldown
{
  int cooldown = 0;
};

struct SleepTimer
{
  int timer = 5;
  int timeLeft = 0;
};

struct NumHealsPlanted
{
  int numHealsPlanted = 0;
  int numHealsNeeded = 0;
};

struct ShouldSleep {};

struct HealAmount
{
  float amount = 0.f;
};

struct PowerupAmount
{
  float amount = 0.f;
};

struct PlayerInput
{
  bool left = false;
  bool right = false;
  bool up = false;
  bool down = false;
};

struct Symbol
{
  char symb;
};

struct IsPlayer {};

struct CanHeal {};

struct Team
{
  int team = 0;
};

struct TextureSource {};

