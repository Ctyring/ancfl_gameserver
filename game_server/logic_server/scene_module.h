#ifndef __SCENE_MODULE_H__
#define __SCENE_MODULE_H__

#include <unordered_map>
#include <vector>
#include "logic_service.h"

namespace game_server {

// 场景对象类型
enum class SceneObjectType {
    NONE = 0,
    PLAYER = 1,
    MONSTER = 2,
    NPC = 3,
    ITEM = 4,
    BULLET = 5,
    SKILL = 6
};

// 场景对象基础信息
struct SceneObject {
    uint64_t object_id;
    SceneObjectType type;
    uint64_t owner_id;
    float position_x;
    float position_y;
    float position_z;
    float rotation_y;
    float speed;
    int32_t hp;
    int32_t max_hp;
    bool is_alive;
    time_t create_time;
};

// 玩家场景对象
struct PlayerSceneObject : public SceneObject {
    uint64_t role_id;
    std::string role_name;
    int32_t level;
    int32_t job;
    int32_t camp;
    bool is_moving;
    float target_x;
    float target_z;
};

// 怪物场景对象
struct MonsterSceneObject : public SceneObject {
    int32_t monster_config_id;
    int32_t level;
    int32_t camp;
    bool is_patrol;
    float patrol_center_x;
    float patrol_center_z;
    float patrol_radius;
};

// 场景信息
struct SceneInfo {
    int32_t scene_id;
    int32_t scene_config_id;
    std::string scene_name;
    int32_t max_player_count;
    int32_t scene_type;
    time_t create_time;
    bool is_active;
};

// 场景模块类
class SceneModule {
   public:
    SceneModule(LogicService* service);
    ~SceneModule();

    // 场景管理
    bool CreateScene(int32_t scene_config_id, int32_t& scene_id);
    bool DestroyScene(int32_t scene_id);
    bool GetSceneInfo(int32_t scene_id, SceneInfo& info);
    bool IsSceneActive(int32_t scene_id);

    // 玩家进入/离开场景
    bool PlayerEnterScene(uint64_t role_id, int32_t scene_id);
    bool PlayerLeaveScene(uint64_t role_id);
    bool GetPlayerScene(uint64_t role_id, int32_t& scene_id);

    // 对象管理
    bool AddObject(int32_t scene_id, const SceneObject& object);
    bool RemoveObject(int32_t scene_id, uint64_t object_id);
    bool GetObject(int32_t scene_id, uint64_t object_id, SceneObject& object);
    bool UpdateObject(int32_t scene_id, const SceneObject& object);

    // 获取场景内所有对象
    bool GetSceneObjects(int32_t scene_id, std::vector<SceneObject>& objects);
    bool GetScenePlayers(int32_t scene_id,
                         std::vector<PlayerSceneObject>& players);
    bool GetSceneMonsters(int32_t scene_id,
                          std::vector<MonsterSceneObject>& monsters);

    // 移动同步
    bool PlayerMove(uint64_t role_id, float target_x, float target_z);
    bool PlayerStopMove(uint64_t role_id);
    bool UpdatePlayerPosition(uint64_t role_id,
                              float x,
                              float y,
                              float z,
                              float rotation_y);
    bool GetPlayerPosition(uint64_t role_id,
                           float& x,
                           float& y,
                           float& z,
                           float& rotation_y);

    // 视野管理
    bool GetVisibleObjects(uint64_t role_id, std::vector<SceneObject>& objects);
    bool UpdateVisibility(int32_t scene_id);

    // 场景更新
    void OnUpdate();
    void OnTimer();

    // 场景数据加载和保存
    bool LoadSceneData(int32_t scene_id);
    bool SaveSceneData(int32_t scene_id);

   private:
    // 生成对象ID
    uint64_t GenerateObjectId();

    // 生成场景ID
    int32_t GenerateSceneId();

    // 场景缓存
    std::unordered_map<int32_t, SceneInfo> scene_cache_;
    std::unordered_map<int32_t, std::unordered_map<uint64_t, SceneObject>>
        scene_objects_;
    std::unordered_map<uint64_t, int32_t> player_scene_map_;

    // 逻辑服务指针
    LogicService* service_;

    // 互斥锁
    std::mutex cache_mutex_;

    // 视野半径
    static const float VIEW_RADIUS;

    // 移动更新间隔（毫秒）
    static const int32_t MOVE_UPDATE_INTERVAL = 100;
};

}  // namespace game_server

#endif  // __SCENE_MODULE_H__
