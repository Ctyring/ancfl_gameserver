#include "scene_module.h"
#include "proto/msg_battle.pb.h"
#include <cmath>

namespace game_server {

const float SceneModule::VIEW_RADIUS = 50.0f;

SceneModule::SceneModule(LogicService* service) : service_(service) {
}

SceneModule::~SceneModule() {
}

bool SceneModule::CreateScene(int32_t scene_config_id, int32_t& scene_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 生成场景ID
    scene_id = GenerateSceneId();
    
    // 创建场景信息
    SceneInfo info;
    info.scene_id = scene_id;
    info.scene_config_id = scene_config_id;
    info.scene_name = "Scene_" + std::to_string(scene_id);
    info.max_player_count = 100;
    info.scene_type = 1;
    info.create_time = time(nullptr);
    info.is_active = true;
    
    scene_cache_[scene_id] = info;
    scene_objects_[scene_id] = std::unordered_map<uint64_t, SceneObject>();
    
    LOG_INFO("Scene created: scene_id=%d, config_id=%d", scene_id, scene_config_id);
    return true;
}

bool SceneModule::DestroyScene(int32_t scene_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 检查场景是否存在
    auto it = scene_cache_.find(scene_id);
    if (it == scene_cache_.end()) {
        LOG_ERROR("Scene not found: scene_id=%d", scene_id);
        return false;
    }
    
    // 清理场景内的所有对象
    scene_objects_.erase(scene_id);
    scene_cache_.erase(it);
    
    LOG_INFO("Scene destroyed: scene_id=%d", scene_id);
    return true;
}

bool SceneModule::GetSceneInfo(int32_t scene_id, SceneInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = scene_cache_.find(scene_id);
    if (it == scene_cache_.end()) {
        return false;
    }
    
    info = it->second;
    return true;
}

bool SceneModule::IsSceneActive(int32_t scene_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = scene_cache_.find(scene_id);
    if (it == scene_cache_.end()) {
        return false;
    }
    
    return it->second.is_active;
}

bool SceneModule::PlayerEnterScene(uint64_t role_id, int32_t scene_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 检查场景是否存在
    auto scene_it = scene_cache_.find(scene_id);
    if (scene_it == scene_cache_.end()) {
        LOG_ERROR("Scene not found: scene_id=%d", scene_id);
        return false;
    }
    
    // 检查场景是否已满
    auto& objects = scene_objects_[scene_id];
    int32_t player_count = 0;
    for (const auto& pair : objects) {
        if (pair.second.type == SceneObjectType::PLAYER) {
            player_count++;
        }
    }
    
    if (player_count >= scene_it->second.max_player_count) {
        LOG_ERROR("Scene is full: scene_id=%d, count=%d", scene_id, player_count);
        return false;
    }
    
    // 如果玩家已经在其他场景，先离开
    auto player_it = player_scene_map_.find(role_id);
    if (player_it != player_scene_map_.end()) {
        int32_t old_scene_id = player_it->second;
        if (old_scene_id != scene_id) {
            // 从旧场景移除
            auto old_objects_it = scene_objects_.find(old_scene_id);
            if (old_objects_it != scene_objects_.end()) {
                old_objects_it->second.erase(role_id);
            }
        }
    }
    
    // 创建玩家场景对象
    PlayerSceneObject player;
    player.object_id = role_id;
    player.type = SceneObjectType::PLAYER;
    player.owner_id = role_id;
    player.role_id = role_id;
    player.position_x = 0.0f;
    player.position_y = 0.0f;
    player.position_z = 0.0f;
    player.rotation_y = 0.0f;
    player.speed = 5.0f;
    player.hp = 1000;
    player.max_hp = 1000;
    player.is_alive = true;
    player.create_time = time(nullptr);
    player.is_moving = false;
    player.target_x = 0.0f;
    player.target_z = 0.0f;
    
    // 添加到场景
    objects[role_id] = player;
    player_scene_map_[role_id] = scene_id;
    
    LOG_INFO("Player entered scene: role_id=%llu, scene_id=%d", role_id, scene_id);
    return true;
}

bool SceneModule::PlayerLeaveScene(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 查找玩家所在场景
    auto player_it = player_scene_map_.find(role_id);
    if (player_it == player_scene_map_.end()) {
        LOG_ERROR("Player not in any scene: role_id=%llu", role_id);
        return false;
    }
    
    int32_t scene_id = player_it->second;
    
    // 从场景中移除
    auto objects_it = scene_objects_.find(scene_id);
    if (objects_it != scene_objects_.end()) {
        objects_it->second.erase(role_id);
    }
    
    player_scene_map_.erase(player_it);
    
    LOG_INFO("Player left scene: role_id=%llu, scene_id=%d", role_id, scene_id);
    return true;
}

bool SceneModule::GetPlayerScene(uint64_t role_id, int32_t& scene_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = player_scene_map_.find(role_id);
    if (it == player_scene_map_.end()) {
        return false;
    }
    
    scene_id = it->second;
    return true;
}

bool SceneModule::AddObject(int32_t scene_id, const SceneObject& object) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = scene_objects_.find(scene_id);
    if (it == scene_objects_.end()) {
        LOG_ERROR("Scene not found: scene_id=%d", scene_id);
        return false;
    }
    
    it->second[object.object_id] = object;
    
    LOG_INFO("Object added to scene: scene_id=%d, object_id=%llu, type=%d", scene_id, object.object_id, static_cast<int32_t>(object.type));
    return true;
}

bool SceneModule::RemoveObject(int32_t scene_id, uint64_t object_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = scene_objects_.find(scene_id);
    if (it == scene_objects_.end()) {
        LOG_ERROR("Scene not found: scene_id=%d", scene_id);
        return false;
    }
    
    it->second.erase(object_id);
    
    LOG_INFO("Object removed from scene: scene_id=%d, object_id=%llu", scene_id, object_id);
    return true;
}

bool SceneModule::GetObject(int32_t scene_id, uint64_t object_id, SceneObject& object) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = scene_objects_.find(scene_id);
    if (it == scene_objects_.end()) {
        return false;
    }
    
    auto obj_it = it->second.find(object_id);
    if (obj_it == it->second.end()) {
        return false;
    }
    
    object = obj_it->second;
    return true;
}

bool SceneModule::UpdateObject(int32_t scene_id, const SceneObject& object) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = scene_objects_.find(scene_id);
    if (it == scene_objects_.end()) {
        LOG_ERROR("Scene not found: scene_id=%d", scene_id);
        return false;
    }
    
    it->second[object.object_id] = object;
    return true;
}

bool SceneModule::GetSceneObjects(int32_t scene_id, std::vector<SceneObject>& objects) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = scene_objects_.find(scene_id);
    if (it == scene_objects_.end()) {
        return false;
    }
    
    objects.clear();
    for (const auto& pair : it->second) {
        objects.push_back(pair.second);
    }
    
    return true;
}

bool SceneModule::GetScenePlayers(int32_t scene_id, std::vector<PlayerSceneObject>& players) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = scene_objects_.find(scene_id);
    if (it == scene_objects_.end()) {
        return false;
    }
    
    players.clear();
    for (const auto& pair : it->second) {
        if (pair.second.type == SceneObjectType::PLAYER) {
            players.push_back(static_cast<const PlayerSceneObject&>(pair.second));
        }
    }
    
    return true;
}

bool SceneModule::GetSceneMonsters(int32_t scene_id, std::vector<MonsterSceneObject>& monsters) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = scene_objects_.find(scene_id);
    if (it == scene_objects_.end()) {
        return false;
    }
    
    monsters.clear();
    for (const auto& pair : it->second) {
        if (pair.second.type == SceneObjectType::MONSTER) {
            monsters.push_back(static_cast<const MonsterSceneObject&>(pair.second));
        }
    }
    
    return true;
}

bool SceneModule::PlayerMove(uint64_t role_id, float target_x, float target_z) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 查找玩家所在场景
    auto player_it = player_scene_map_.find(role_id);
    if (player_it == player_scene_map_.end()) {
        LOG_ERROR("Player not in any scene: role_id=%llu", role_id);
        return false;
    }
    
    int32_t scene_id = player_it->second;
    
    // 获取玩家对象
    auto objects_it = scene_objects_.find(scene_id);
    if (objects_it == scene_objects_.end()) {
        return false;
    }
    
    auto obj_it = objects_it->second.find(role_id);
    if (obj_it == objects_it->second.end()) {
        return false;
    }
    
    // 更新移动目标
    PlayerSceneObject& player = static_cast<PlayerSceneObject&>(obj_it->second);
    player.is_moving = true;
    player.target_x = target_x;
    player.target_z = target_z;
    
    // 计算朝向
    float dx = target_x - player.position_x;
    float dz = target_z - player.position_z;
    player.rotation_y = atan2f(dx, dz) * 180.0f / 3.14159f;
    
    LOG_INFO("Player move: role_id=%llu, target=(%.2f, %.2f)", role_id, target_x, target_z);
    return true;
}

bool SceneModule::PlayerStopMove(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 查找玩家所在场景
    auto player_it = player_scene_map_.find(role_id);
    if (player_it == player_scene_map_.end()) {
        LOG_ERROR("Player not in any scene: role_id=%llu", role_id);
        return false;
    }
    
    int32_t scene_id = player_it->second;
    
    // 获取玩家对象
    auto objects_it = scene_objects_.find(scene_id);
    if (objects_it == scene_objects_.end()) {
        return false;
    }
    
    auto obj_it = objects_it->second.find(role_id);
    if (obj_it == objects_it->second.end()) {
        return false;
    }
    
    // 停止移动
    PlayerSceneObject& player = static_cast<PlayerSceneObject&>(obj_it->second);
    player.is_moving = false;
    
    LOG_INFO("Player stop move: role_id=%llu", role_id);
    return true;
}

bool SceneModule::UpdatePlayerPosition(uint64_t role_id, float x, float y, float z, float rotation_y) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 查找玩家所在场景
    auto player_it = player_scene_map_.find(role_id);
    if (player_it == player_scene_map_.end()) {
        return false;
    }
    
    int32_t scene_id = player_it->second;
    
    // 获取玩家对象
    auto objects_it = scene_objects_.find(scene_id);
    if (objects_it == scene_objects_.end()) {
        return false;
    }
    
    auto obj_it = objects_it->second.find(role_id);
    if (obj_it == objects_it->second.end()) {
        return false;
    }
    
    // 更新位置
    obj_it->second.position_x = x;
    obj_it->second.position_y = y;
    obj_it->second.position_z = z;
    obj_it->second.rotation_y = rotation_y;
    
    return true;
}

bool SceneModule::GetPlayerPosition(uint64_t role_id, float& x, float& y, float& z, float& rotation_y) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 查找玩家所在场景
    auto player_it = player_scene_map_.find(role_id);
    if (player_it == player_scene_map_.end()) {
        return false;
    }
    
    int32_t scene_id = player_it->second;
    
    // 获取玩家对象
    auto objects_it = scene_objects_.find(scene_id);
    if (objects_it == scene_objects_.end()) {
        return false;
    }
    
    auto obj_it = objects_it->second.find(role_id);
    if (obj_it == objects_it->second.end()) {
        return false;
    }
    
    // 获取位置
    x = obj_it->second.position_x;
    y = obj_it->second.position_y;
    z = obj_it->second.position_z;
    rotation_y = obj_it->second.rotation_y;
    
    return true;
}

bool SceneModule::GetVisibleObjects(uint64_t role_id, std::vector<SceneObject>& objects) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 查找玩家所在场景
    auto player_it = player_scene_map_.find(role_id);
    if (player_it == player_scene_map_.end()) {
        return false;
    }
    
    int32_t scene_id = player_it->second;
    
    // 获取玩家位置
    auto objects_it = scene_objects_.find(scene_id);
    if (objects_it == scene_objects_.end()) {
        return false;
    }
    
    auto obj_it = objects_it->second.find(role_id);
    if (obj_it == objects_it->second.end()) {
        return false;
    }
    
    float player_x = obj_it->second.position_x;
    float player_z = obj_it->second.position_z;
    
    // 获取视野内的对象
    objects.clear();
    for (const auto& pair : objects_it->second) {
        if (pair.first == role_id) {
            continue;
        }
        
        float dx = pair.second.position_x - player_x;
        float dz = pair.second.position_z - player_z;
        float distance = sqrtf(dx * dx + dz * dz);
        
        if (distance <= VIEW_RADIUS) {
            objects.push_back(pair.second);
        }
    }
    
    return true;
}

bool SceneModule::UpdateVisibility(int32_t scene_id) {
    // TODO: 实现视野更新逻辑
    return true;
}

void SceneModule::OnUpdate() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 更新所有场景中移动的玩家
    for (auto& scene_pair : scene_objects_) {
        for (auto& obj_pair : scene_pair.second) {
            if (obj_pair.second.type == SceneObjectType::PLAYER) {
                PlayerSceneObject& player = static_cast<PlayerSceneObject&>(obj_pair.second);
                
                if (player.is_moving) {
                    // 计算移动
                    float dx = player.target_x - player.position_x;
                    float dz = player.target_z - player.position_z;
                    float distance = sqrtf(dx * dx + dz * dz);
                    
                    if (distance < 0.1f) {
                        // 到达目标
                        player.is_moving = false;
                        player.position_x = player.target_x;
                        player.position_z = player.target_z;
                    } else {
                        // 继续移动
                        float move_distance = player.speed * 0.1f; // 假设每100ms更新一次
                        if (move_distance > distance) {
                            move_distance = distance;
                        }
                        
                        player.position_x += (dx / distance) * move_distance;
                        player.position_z += (dz / distance) * move_distance;
                    }
                }
            }
        }
    }
}

void SceneModule::OnTimer() {
    // 定期清理不活跃的场景
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    time_t now = time(nullptr);
    std::vector<int32_t> scenes_to_destroy;
    
    for (const auto& pair : scene_cache_) {
        // 检查场景是否为空且创建时间超过一定时间
        auto objects_it = scene_objects_.find(pair.first);
        if (objects_it != scene_objects_.end() && objects_it->second.empty()) {
            if (now - pair.second.create_time > 300) { // 5分钟
                scenes_to_destroy.push_back(pair.first);
            }
        }
    }
    
    // 销毁空场景
    for (int32_t scene_id : scenes_to_destroy) {
        scene_objects_.erase(scene_id);
        scene_cache_.erase(scene_id);
        LOG_INFO("Auto destroyed empty scene: scene_id=%d", scene_id);
    }
}

bool SceneModule::LoadSceneData(int32_t scene_id) {
    // TODO: 从数据库加载场景数据
    return true;
}

bool SceneModule::SaveSceneData(int32_t scene_id) {
    // TODO: 保存场景数据到数据库
    return true;
}

uint64_t SceneModule::GenerateObjectId() {
    static uint64_t next_id = time(nullptr) * 10000 + rand() % 10000;
    return next_id++;
}

int32_t SceneModule::GenerateSceneId() {
    static int32_t next_id = 1000;
    return next_id++;
}

} // namespace game_server
