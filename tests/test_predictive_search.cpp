#include "optimizer/predictive_search.hpp"
#include <gtest/gtest.h>

namespace to_gif {

TEST(PredictiveSearchTest, InvalidRatioReturnsHalfParam) {
    PredictiveSearchStrategy strategy;

    EXPECT_EQ(strategy.predict_next(100, 0.0, 10, 200), 50);
    EXPECT_EQ(strategy.predict_next(100, -1.0, 10, 200), 50);
    EXPECT_EQ(strategy.predict_next(20, 0.0, 10, 200), 10);  // clamped to minimum
}

TEST(PredictiveSearchTest, RatioLessThanOneReducesParam) {
    PredictiveSearchStrategy strategy;

    EXPECT_EQ(strategy.predict_next(100, 0.5, 10, 200), 50);
}

TEST(PredictiveSearchTest, RatioGreaterThanOneIncreasesParam) {
    PredictiveSearchStrategy strategy;

    EXPECT_EQ(strategy.predict_next(100, 2.0, 10, 200), 200);  // clamped to maximum
}

TEST(PredictiveSearchTest, ResultClampedToMin) {
    PredictiveSearchStrategy strategy;

    EXPECT_EQ(strategy.predict_next(100, 0.0316, 10, 200), 10);
}

TEST(PredictiveSearchTest, ResultClampedToMax) {
    PredictiveSearchStrategy strategy;

    EXPECT_EQ(strategy.predict_next(100, 31.6, 10, 200), 200);
}

TEST(PredictiveSearchTest, UnitRatioReturnsSameParam) {
    PredictiveSearchStrategy strategy;

    EXPECT_EQ(strategy.predict_next(100, 1.0, 10, 200), 100);
}

} // namespace to_gif
