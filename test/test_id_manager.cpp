#include "id_manager.h"
#include <gtest/gtest.h>
#include "test_header.h"
#include "benchmark_header.h"

namespace levin {

class IdManagerTest : public ::testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(IdManagerTest, test_init) {
    levin::Timer timer;
    bool succ = IdManager::GetInstance().init();
    std::cout << "IdManager init spent(us)=" << timer.get_time_us() << std::endl;
    EXPECT_TRUE(succ);
    sleep(1);
}

}  // namespace levin

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
