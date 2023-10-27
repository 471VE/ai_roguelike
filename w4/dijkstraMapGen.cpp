#include "dijkstraMapGen.h"
#include "ecsTypes.h"
#include "dungeonUtils.h"
#include <queue>

template<typename Callable>
static void query_dungeon_data(flecs::world &ecs, Callable c)
{
  static auto dungeonDataQuery = ecs.query<const DungeonData>();

  dungeonDataQuery.each(c);
}

template<typename Callable>
static void query_characters_positions(flecs::world &ecs, Callable c)
{
  static auto characterPositionQuery = ecs.query<const Position, const Team>();

  characterPositionQuery.each(c);
}

constexpr float invalid_tile_value = 1e5f;

static void init_tiles(std::vector<float> &map, const DungeonData &dd)
{
  map.resize(dd.width * dd.height);
  for (float &v : map)
    v = invalid_tile_value;
}

// scan version, could be implemented as Dijkstra version as well
static void process_dmap(std::vector<float> &map, const DungeonData &dd)
{
  bool done = false;
  auto getMapAt = [&](size_t x, size_t y, float def)
  {
    if (x < dd.width && y < dd.width && dd.tiles[y * dd.width + x] == dungeon::floor)
      return map[y * dd.width + x];
    return def;
  };
  auto getMinNei = [&](size_t x, size_t y)
  {
    float val = map[y * dd.width + x];
    val = std::min(val, getMapAt(x - 1, y + 0, val));
    val = std::min(val, getMapAt(x + 1, y + 0, val));
    val = std::min(val, getMapAt(x + 0, y - 1, val));
    val = std::min(val, getMapAt(x + 0, y + 1, val));
    return val;
  };
  while (!done)
  {
    done = true;
    for (size_t y = 0; y < dd.height; ++y)
      for (size_t x = 0; x < dd.width; ++x)
      {
        const size_t i = y * dd.width + x;
        if (dd.tiles[i] != dungeon::floor)
          continue;
        const float myVal = getMapAt(x, y, invalid_tile_value);
        const float minVal = getMinNei(x, y);
        if (minVal < myVal - 1.f)
        {
          map[i] = minVal + 1.f;
          done = false;
        }
      }
  }
}

void dmaps::gen_player_approach_map(flecs::world &ecs, std::vector<float> &map)
{
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    init_tiles(map, dd);
    query_characters_positions(ecs, [&](const Position &pos, const Team &t)
    {
      if (t.team == 0) // player team hardcode
        map[pos.y * dd.width + pos.x] = 0.f;
    });
    process_dmap(map, dd);
  });
}

static float visibility_value(const std::vector<float> &map, const DungeonData &dd, const Position &pos, const Position &npos)
{
  int dir_x = 2 * (pos.x > npos.x) - 1;
  int dir_y = 2 * (pos.y > npos.y) - 1;
  float new_val = 0;
  if (abs(pos.x - npos.x) > abs(pos.y - npos.y))
    new_val = map[npos.y * dd.width + (npos.x + dir_x)] + 1 + 
              (dd.tiles[pos.y * dd.width + (pos.x - dir_x)] == dungeon::wall) * invalid_tile_value;
  else if (abs(pos.x - npos.x) == abs(pos.y - npos.y))
    new_val = map[(npos.y + dir_y) * dd.width + (npos.x + dir_x)] + 2 +
              (dd.tiles[(pos.y - dir_y) * dd.width + (pos.x - dir_x)] == dungeon::wall) * invalid_tile_value;
  else
    new_val = map[(npos.y + dir_y) * dd.width + npos.x] + 1 +
              (dd.tiles[(pos.y - dir_y) * dd.width + pos.x] == dungeon::wall) * invalid_tile_value;
  return std::min(new_val, invalid_tile_value);
}

using tail = std::pair<float, std::pair<int, int>>;

static void gen_player_vision_map(flecs::world &ecs, std::vector<float> &map)
{
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    init_tiles(map, dd);
    query_characters_positions(ecs, [&](const Position &ppos, const Team &t)
    {
      if (t.team == 0) // player team hardcode
      {
        map[ppos.y * dd.width + ppos.x] = 0.f;
        // Dijkstra's algorithm
        std::priority_queue<tail, std::vector<tail>, std::greater<tail>> queue;
        std::vector<bool> visited(map.size(), false);
        queue.push({0., {ppos.x, ppos.y}});
        auto atPos = [&](int x, int y) { return y * dd.width + x; };
        while (!queue.empty()) 
        {
          auto [valAtPos, pos] = queue.top();
          auto [pos_x, pos_y] = pos;
          queue.pop();

          if (visited[atPos(pos_x, pos_y)])
            continue;

          visited[atPos(pos_x, pos_y)] = true;
          map[atPos(pos_x, pos_y)] = valAtPos;

          if (pos_x > 0 && !visited[atPos(pos_x - 1, pos_y)] &&  dd.tiles[atPos(pos_x - 1, pos_y)] == dungeon::floor)
          {
            float val = visibility_value(map, dd, ppos, {pos_x - 1, pos_y});
            if (map[atPos(pos_x - 1, pos_y)] > val)
            {
              map[atPos(pos_x - 1, pos_y)] = val;  
              queue.push({val, {pos_x - 1, pos_y}});
            }
          }

          if (pos_x < dd.width - 1 && !visited[atPos(pos_x + 1, pos_y)] && dd.tiles[atPos(pos_x + 1, pos_y)] == dungeon::floor)
          {
            float val = visibility_value(map, dd, ppos, {pos_x + 1, pos_y});
            if (map[atPos(pos_x + 1, pos_y)] > val)
            {
              map[atPos(pos_x + 1, pos_y)] = val;  
              queue.push({val, {pos_x + 1, pos_y}});
            }
          }

          if (pos_y > 0 && !visited[atPos(pos_x, pos_y - 1)] && dd.tiles[atPos(pos_x, pos_y - 1)] == dungeon::floor)
          {
            float val = visibility_value(map, dd, ppos, {pos_x, pos_y - 1});
            if (map[atPos(pos_x, pos_y - 1)] > val)
            {
              map[atPos(pos_x, pos_y - 1)] = val;  
              queue.push({val, {pos_x, pos_y - 1}});
            }
          }

          if (pos_y < dd.height - 1 && !visited[atPos(pos_x, pos_y + 1)] && dd.tiles[atPos(pos_x, pos_y + 1)] == dungeon::floor)
          {
            float val = visibility_value(map, dd, ppos, {pos_x, pos_y + 1});
            if (map[atPos(pos_x, pos_y + 1)] > val)
            {
              map[atPos(pos_x, pos_y + 1)] = val;  
              queue.push({val, {pos_x, pos_y + 1}});
            }
          }
        }
      }
    });
  });
}

void dmaps::gen_player_flee_map(flecs::world &ecs, std::vector<float> &map)
{
  ecs.entity("approach_map").get([&](const DijkstraMapData &dmap) {
    map = dmap.map;
  });
  for (float &v : map)
    if (v < invalid_tile_value)
      v *= -1.2f;
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    process_dmap(map, dd);
  });
}

void dmaps::gen_hive_pack_map(flecs::world &ecs, std::vector<float> &map)
{
  static auto hiveQuery = ecs.query<const Position, const Hive>();
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    init_tiles(map, dd);
    hiveQuery.each([&](const Position &pos, const Hive &)
    {
      map[pos.y * dd.width + pos.x] = 0.f;
    });
    process_dmap(map, dd);
  });
}

void dmaps::gen_mage_map(flecs::world &ecs, std::vector<float> &map)
{
  ecs.entity("approach_map").get([&](const DijkstraMapData &dmap) {
    map = dmap.map;
  });
  std::vector<float> visionMap;
  gen_player_vision_map(ecs, visionMap);
  for (size_t i = 0; i < map.size(); i++)
  {
    float approachValue = map[i];
    float visionValue = visionMap[i];
    if (approachValue < invalid_tile_value)
    {
      if (visionValue < invalid_tile_value)
        // Player not seeing mage is the same as mage not seeing player
        map[i] = abs(approachValue - 4.f);
    }
  }
}

void dmaps::gen_ally_map(flecs::world &ecs, std::vector<float> &map)
{
  // static auto allyQuery = ecs.query<const Position, const Team>();
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    init_tiles(map, dd);
    query_characters_positions(ecs, [&](flecs::entity e, const Position &pos, const Team &t) {
      if (t.team == 1 // hardcoding enemy team
        && !e.has<ShootDamage>()) // and not a mage
      {
        map[pos.y * dd.width + pos.x] = 0.f;
      }
    });
    process_dmap(map, dd);
  });
}

void dmaps::gen_exploration_map(flecs::world &ecs, std::vector<float> &map)
{
  static auto tileQuery = ecs.query<const Position, const BackgroundTile, const ExplorationStatus>();
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    init_tiles(map, dd);
    tileQuery.each([&](const Position &pos, const BackgroundTile, const ExplorationStatus &status) {
      if (!status.explored)
        map[pos.y * dd.width + pos.x] = 0.f;
    });
    process_dmap(map, dd);
  });
}
