#include "stateMachine.h"

StateMachine::~StateMachine()
{
  for (StateMachine* stateMachineState : stateMachineStates)
    delete stateMachineState;
  stateMachineStates.clear();
  for (auto &transList : transitions)
    for (auto &transition : transList)
      delete transition.first;
  transitions.clear();
}

void StateMachine::enter()  {
  curStateIdx = 0; // reset to default state
  if (leafState != nullptr)
    leafState->enter();
  else 
    stateMachineStates[curStateIdx]->enter();
}

void StateMachine::exit() const {
  if (leafState != nullptr)
    leafState->exit();
  else 
    stateMachineStates[curStateIdx]->exit();
}

void StateMachine::act(float dt, flecs::world &ecs, flecs::entity entity)
{
  if (leafState != nullptr) {
    leafState->act(dt, ecs, entity);
    return;
  }
  if (curStateIdx < stateMachineStates.size())
  {
    for (const std::pair<StateTransition*, int> &transition : transitions[curStateIdx])
      if (transition.first->isAvailable(ecs, entity))
      {
        stateMachineStates[curStateIdx]->exit();
        curStateIdx = transition.second;
        stateMachineStates[curStateIdx]->enter();
        break;
      }
    stateMachineStates[curStateIdx]->act(dt, ecs, entity);
  }
  else
    curStateIdx = 0;
}

int StateMachine::addState(State *st)
{
  int idx = stateMachineStates.size();
  stateMachineStates.push_back(new StateMachine(st));
  transitions.push_back(std::vector<std::pair<StateTransition*, int>>());
  return idx;
}

int StateMachine::addStateFromStateMachine(StateMachine *sm)
{
  int idx = stateMachineStates.size();
  stateMachineStates.push_back(sm);
  transitions.push_back(std::vector<std::pair<StateTransition*, int>>());
  return idx;
}

void StateMachine::addTransition(StateTransition *trans, int from, int to)
{
  transitions[from].push_back(std::make_pair(trans, to));
}

