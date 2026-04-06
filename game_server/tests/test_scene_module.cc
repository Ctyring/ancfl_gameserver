#include <gtest/gtest.h>
#include "logic_server/scene_module.h"
#include "logic_server/logic_service.h"

using namespace game_server;

class SceneModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        service_ = nullptr;
        scene_module_ = new SceneModule(service_);
        test_role_id_ = 12345;
    }
    
    void TearDown() override {
        delete scene_module_;
    }
    
    LogicService* service_;
    SceneModule* scene_module_;
    uint64_t test_role_id_;
};

TEST_F(SceneModuleTest, CreateScene) {
    int32_t scene_id = 0;
    EXPECT_TRUE(scene_module_->CreateScene(1001, scene_id));
    EXPECT_NE(scene_id, 0);
}

TEST_F(SceneModuleTest, DestroyScene) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    
    EXPECT_TRUE(scene_module_->DestroyScene(scene_id));
}

TEST_F(SceneModuleTest, GetSceneInfo) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    
    SceneInfo info;
    EXPECT_TRUE(scene_module_->GetSceneInfo(scene_id, info));
    EXPECT_EQ(info.scene_config_id, 1001);
}

TEST_F(SceneModuleTest, PlayerEnterScene) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    
    EXPECT_TRUE(scene_module_->PlayerEnterScene(test_role_id_, scene_id));
    
    int32_t current_scene = 0;
    EXPECT_TRUE(scene_module_->GetPlayerScene(test_role_id_, current_scene));
    EXPECT_EQ(current_scene, scene_id);
}

TEST_F(SceneModuleTest, PlayerLeaveScene) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    scene_module_->PlayerEnterScene(test_role_id_, scene_id);
    
    EXPECT_TRUE(scene_module_->PlayerLeaveScene(test_role_id_));
}

TEST_F(SceneModuleTest, GetSceneObjects) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    scene_module_->PlayerEnterScene(test_role_id_, scene_id);
    
    std::vector<SceneObject> objects;
    EXPECT_TRUE(scene_module_->GetSceneObjects(scene_id, objects));
}

TEST_F(SceneModuleTest, PlayerMove) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    scene_module_->PlayerEnterScene(test_role_id_, scene_id);
    
    EXPECT_TRUE(scene_module_->PlayerMove(test_role_id_, 100.0f, 200.0f));
}

TEST_F(SceneModuleTest, UpdatePlayerPosition) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    scene_module_->PlayerEnterScene(test_role_id_, scene_id);
    
    EXPECT_TRUE(scene_module_->UpdatePlayerPosition(test_role_id_, 100.0f, 0.0f, 200.0f, 0.0f));
}

TEST_F(SceneModuleTest, GetPlayerPosition) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    scene_module_->PlayerEnterScene(test_role_id_, scene_id);
    scene_module_->UpdatePlayerPosition(test_role_id_, 100.0f, 0.0f, 200.0f, 0.0f);
    
    float x, y, z, rotation_y;
    EXPECT_TRUE(scene_module_->GetPlayerPosition(test_role_id_, x, y, z, rotation_y));
    EXPECT_FLOAT_EQ(x, 100.0f);
    EXPECT_FLOAT_EQ(z, 200.0f);
}

TEST_F(SceneModuleTest, AddObject) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    
    SceneObject obj;
    obj.object_id = 99999;
    obj.type = SceneObjectType::MONSTER;
    obj.position_x = 100.0f;
    obj.position_y = 0.0f;
    obj.position_z = 100.0f;
    
    EXPECT_TRUE(scene_module_->AddObject(scene_id, obj));
}

TEST_F(SceneModuleTest, RemoveObject) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    
    SceneObject obj;
    obj.object_id = 99999;
    obj.type = SceneObjectType::MONSTER;
    scene_module_->AddObject(scene_id, obj);
    
    EXPECT_TRUE(scene_module_->RemoveObject(scene_id, 99999));
}

TEST_F(SceneModuleTest, GetVisibleObjects) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    scene_module_->PlayerEnterScene(test_role_id_, scene_id);
    
    std::vector<SceneObject> objects;
    EXPECT_TRUE(scene_module_->GetVisibleObjects(test_role_id_, objects));
}
