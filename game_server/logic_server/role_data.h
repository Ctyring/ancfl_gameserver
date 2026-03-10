#ifndef __ROLE_DATA_H__
#define __ROLE_DATA_H__

#include "common/shared_memory.h"

namespace game_server {

struct RoleData : public ShareObject {
    uint64_t role_id;
    uint64_t account_id;
    std::string role_name;
    int32_t level;
    int32_t exp;
    int32_t gold;
    int32_t diamond;
    int32_t job;
    int32_t gender;
    int32_t create_time;
    int32_t last_login_time;
    int32_t last_logout_time;
    int32_t online_time;
    int32_t vip_level;
    int32_t vip_exp;
    int32_t stamina;
    int32_t energy;
    int32_t reputation;
    int32_t honor;
    int32_t war_credit;
    int32_t achievement;
    int32_t fight_power;
    int32_t current_scene;
    float position_x;
    float position_y;
    float position_z;
    float rotation_y;
};

}  // namespace game_server

#endif  // __ROLE_DATA_H__
