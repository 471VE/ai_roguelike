# NOTE: flecs dymanic library should be copied into root directory!

# Week 7 notes
Use **LMB** and **RMB** to set end points of a route. You can zoom in and out on the map with your mouse wheel.

# Week 4 notes
Press **E** key to automatically explore dungeon. Numbers being displayed on tiles are values of mage's Dijkstra's map.

# Pathfinding notes
Press `1` or `2` on your keyboard to switch between ARA* and A* path finding algorithms.

# Description
Learning materials for the course "AI for videogames" based on simple roguelike mechanics.
* w1 - FSM
* w2 - Behaviour Trees + blackboard
* w3 - Utility functions
* w4 - Emergent behaviour
* w5 - Goal Oriented Action Planning

## Dependencies
This project uses:
* bgfx for week1 project
* raylib for week2 and later project
* flecs for ECS

## Building

To build you first need to update submodules:
```
git submodule sync
git submodule update --init --recursive
```

Then you need to build using cmake:
```
cmake -B build
cmake --build build
```
