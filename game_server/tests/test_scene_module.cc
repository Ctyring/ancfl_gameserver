#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "logic_server/scene_module.h"

using namespace game_server;
using namespace testing;

class MockLogicServiceForScene : public LogicService {
public:
    MOCK_METHOD(bool, SendToClient, (uint64_t role_id, int32_t msg_id, const std::string& data), (override));
    MOCK_METHOD(bool, BroadcastToScene, (int32_t scene_id, int32_t msg_id, const std::string& data), (override));
};

class SceneModuleTest : public Test {
protected:
    void SetUp() override {
        mock_service_ = new MockLogicServiceForScene();
        scene_module_ = new SceneModule(mock_service_);
        test_role_id_ = 12345;
    }
    
    void TearDown() override {
        delete scene_module_;
        delete mock_service_;
    }
    
    MockLogicServiceForScene* mock_service_;
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

TEST_F(SceneModuleTest, EnterScene) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    
    EXPECT_TRUE(scene_module_->EnterScene(test_role_id_, scene_id));
    
    int32_t current_scene = 0;
    EXPECT_TRUE(scene_module_->GetRoleScene(test_role_id_, current_scene));
    EXPECT_EQ(current_scene, scene_id);
}

TEST_F(SceneModuleTest, LeaveScene) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    scene_module_->EnterScene(test_role_id_, scene_id);
    
    EXPECT_TRUE(scene_module_->LeaveScene(test_role_id_));
    
    int32_t current_scene = 0;
    EXPECT_FALSE(scene_module_->GetRoleScene(test_role_id_, current_scene));
}

TEST_F(SceneModuleTest, GetSceneObjects) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    scene_module_->EnterScene(test_role_id_, scene_id);
    
    std::vector<SceneObject> objects;
    EXPECT_TRUE(scene_module_->GetSceneObjects(scene_id, objects));
    EXPECT_EQ(objects.size(), 1);
}

TEST_F(SceneModuleTest, MoveObject) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    scene_module_->EnterScene(test_role_id_, scene_id);
    
    Position pos;
    pos.x = 100.0f;
    pos.y = 200.0f;
    pos.z = 0.0f;
    
    EXPECT_TRUE(scene_module_->MoveObject(test_role_id_, pos));
    
    SceneObject obj;
    scene_module_->GetSceneObject(test_role_id_, obj);
    EXPECT_FLOAT_EQ(obj.pos.x, 100.0f);
    EXPECT_FLOAT_EQ(obj.pos.y, 200.0f);
}

TEST_F(SceneModuleTest, CreateNpc) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    
    uint64_t npc_id = 0;
    EXPECT_TRUE(scene_module_->CreateNpc(scene_id, 5001, Position{100, 100, 0}, npc_id));
    EXPECT_NE(npc_id, 0);
}

TEST_F(SceneModuleTest, RemoveNpc) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    
    uint64_t npc_id = 0;
    scene_module_->CreateNpc(scene_id, 5001, Position{100, 100, 0}, npc_id);
    
    EXPECT_TRUE(scene_module_->RemoveNpc(scene_id, npc_id));
}

TEST_F(SceneModuleTest, GetSceneInfo) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    
    SceneInfo info;
    EXPECT_TRUE(scene_module_->GetSceneInfo(scene_id, info));
    EXPECT_EQ(info.scene_config_id, 1001);
}

TEST_F(SceneModuleTest, GetObjectCount) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    scene_module_->EnterScene(test_role_id_, scene_id);
    
    EXPECT_EQ(scene_module_->GetObjectCount(scene_id), 1);
}

TEST_F(SceneModuleTest, BroadCastToScene) {
    int32_t scene_id = 0;
    scene_module_->CreateScene(1001, scene_id);
    scene_module_->EnterScene(test_role_id_, scene_id);
    
    EXPECT_CALL(*mock_service_, BroadcastToScene(scene_id, testing::_, testing::_))
        .Times(1);
    
    scene_module_->BroadcastToScene(scene_id, 1001, "test_data");
}
