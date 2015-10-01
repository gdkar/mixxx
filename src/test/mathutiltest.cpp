#include <gtest/gtest.h>
#include "util/math.h"

#include <QtDebug>

namespace {

class MathUtilTest : public testing::Test {
  protected:

    MathUtilTest() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }

    static const int MIN;
    static const int MAX;

    static const int VALUE_MIN;
    static const int VALUE_MAX;
};

const int MathUtilTest::MIN = -10;
const int MathUtilTest::MAX = 10;

const int MathUtilTest::VALUE_MIN = 2 * MathUtilTest::MIN;
const int MathUtilTest::VALUE_MAX = 2  * MathUtilTest::MAX;

TEST_F(MathUtilTest, MathClampUnsafe) {
    for (int i = VALUE_MIN; i <= VALUE_MAX; ++i) {
        EXPECT_LE(MIN, clamp(i, MIN, MAX));
        EXPECT_GE(MAX, clamp(i, MIN, MAX));
        EXPECT_EQ(MIN, clamp(i, MIN, MIN));
        EXPECT_EQ(MAX, clamp(i, MAX, MAX));
        if (MIN >= i) {
            EXPECT_EQ(MIN, clamp(i, MIN, MAX));
        }
        if (MAX <= i) {
            EXPECT_EQ(MAX, clamp(i, MIN, MAX));
        }
    }
}

}  // namespace
