#include "ecsTypes.h"
#include "dmapFollower.h"
#include <cmath>

void process_dmap_followers(flecs::world &ecs, bool processPlayer)
{
  static auto processDmapFollowers = ecs.query<const Position, Action, const DmapTransform>();
  static auto dungeonDataQuery = ecs.query<const DungeonData>();

  auto get_dmap_at = [&](const DijkstraMapData &dmap, const DungeonData &dd, size_t x, size_t y)
  {
    return dmap.map[y * dd.width + x];
  };
  dungeonDataQuery.each([&](const DungeonData &dd)
  {
    processDmapFollowers.each([&](flecs::entity e, const Position& pos, Action &act, const DmapTransform &wt)
    {
      if (e.has<IsPlayer>() != processPlayer)
        return;
      if (e.has<IsPlayer>() && act.action != EA_EXPLORE)
        return;
      float moveWeights[EA_MOVE_END];
      for (size_t i = 0; i < EA_MOVE_END; ++i)
        moveWeights[i] = 0.f;
      for (const auto &pair : wt.transform)
      {
        ecs.entity(pair.first.c_str()).get([&](const DijkstraMapData &dmap)
        {
          moveWeights[EA_NOP]         += pair.second(e, get_dmap_at(dmap, dd, pos.x+0, pos.y+0));
          moveWeights[EA_MOVE_LEFT]   += pair.second(e, get_dmap_at(dmap, dd, pos.x-1, pos.y+0));
          moveWeights[EA_MOVE_RIGHT]  += pair.second(e, get_dmap_at(dmap, dd, pos.x+1, pos.y+0));
          moveWeights[EA_MOVE_UP]     += pair.second(e, get_dmap_at(dmap, dd, pos.x+0, pos.y-1));
          moveWeights[EA_MOVE_DOWN]   += pair.second(e, get_dmap_at(dmap, dd, pos.x+0, pos.y+1));
        });
      }
      float minWt = moveWeights[EA_NOP];
      for (size_t i = 0; i < EA_MOVE_END; ++i)
        if (moveWeights[i] < minWt)
        {
          minWt = moveWeights[i];
          act.action = i;
        }
    });
  });
}

