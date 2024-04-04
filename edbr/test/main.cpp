#include <edbr/Util/OSUtil.h>
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    util::setCurrentDirToExeDir();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
