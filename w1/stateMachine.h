#pragma once
#include <vector>
#include <flecs.h>

class State
{
public:
  virtual void enter() const = 0;
  virtual void exit() const = 0;
  virtual void act(float dt, flecs::world &ecs, flecs::entity entity) const = 0;
};

class StateTransition
{
public:
  virtual ~StateTransition() {}
  virtual bool isAvailable(flecs::world &ecs, flecs::entity entity) const = 0;
};

class StateMachine
{
  int curStateIdx = 0;
  std::vector<StateMachine*> stateMachineStates;
  std::vector<std::vector<std::pair<StateTransition*, int>>> transitions;
  State *leafState = nullptr;
public:
  StateMachine() = default;
  StateMachine(State *st) : leafState(st) {};
  StateMachine(const StateMachine &sm) = default;
  StateMachine(StateMachine &&sm) = default;

  ~StateMachine();

  StateMachine &operator=(const StateMachine &sm) = default;
  StateMachine &operator=(StateMachine &&sm) = default;
  
  void enter();
  void exit() const;
  void act(float dt, flecs::world &ecs, flecs::entity entity);

  int addState(State *st);
  int addStateFromStateMachine(StateMachine *sm);
  void addTransition(StateTransition *trans, int from, int to);
};

