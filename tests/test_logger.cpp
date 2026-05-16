#include "utils/logger.hpp"
#include <gtest/gtest.h>
#include <sstream>

namespace to_gif {

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::instance().set_level(LogLevel::kDebug);
        Logger::instance().set_output(std::cerr);
    }

    void TearDown() override {
        Logger::instance().set_output(std::cerr);
    }
};

TEST_F(LoggerTest, SingletonReturnsSameInstance) {
    Logger& a = Logger::instance();
    Logger& b = Logger::instance();
    EXPECT_EQ(&a, &b);
}

TEST_F(LoggerTest, LogOutputCanBeRedirected) {
    std::ostringstream oss;
    Logger::instance().set_output(oss);
    Logger::instance().set_level(LogLevel::kInfo);

    LOG(kInfo) << "test_message_42";

    std::string output = oss.str();
    EXPECT_NE(output.find("test_message_42"), std::string::npos);
    EXPECT_NE(output.find("INFO"), std::string::npos);
}

TEST_F(LoggerTest, LevelBelowThresholdIsFiltered) {
    std::ostringstream oss;
    Logger::instance().set_output(oss);
    Logger::instance().set_level(LogLevel::kWarn);

    LOG(kInfo) << "should_not_appear";

    EXPECT_EQ(oss.str().find("should_not_appear"), std::string::npos);
}

TEST_F(LoggerTest, LevelAtThresholdIsLogged) {
    std::ostringstream oss;
    Logger::instance().set_output(oss);
    Logger::instance().set_level(LogLevel::kWarn);

    LOG(kWarn) << "should_appear";

    EXPECT_NE(oss.str().find("should_appear"), std::string::npos);
}

TEST_F(LoggerTest, FatalLevelIsLogged) {
    std::ostringstream oss;
    Logger::instance().set_output(oss);
    Logger::instance().set_level(LogLevel::kFatal);

    LOG(kFatal) << "fatal_msg";

    EXPECT_NE(oss.str().find("fatal_msg"), std::string::npos);
    EXPECT_EQ(oss.str().find("info_msg"), std::string::npos);
}

TEST_F(LoggerTest, LogFormatContainsExpectedFields) {
    std::ostringstream oss;
    Logger::instance().set_output(oss);
    Logger::instance().set_level(LogLevel::kDebug);

    LOG(kError) << "format_check";

    std::string output = oss.str();
    EXPECT_NE(output.find("ERROR"), std::string::npos);
    EXPECT_NE(output.find("format_check"), std::string::npos);
    EXPECT_NE(output.find("["), std::string::npos);
    EXPECT_NE(output.find("]"), std::string::npos);
}

TEST_F(LoggerTest, OffLevelFiltersAllMessages) {
    std::ostringstream oss;
    Logger::instance().set_output(oss);
    Logger::instance().set_level(LogLevel::kOff);

    LOG(kDebug) << "should_not_appear_debug";
    LOG(kInfo) << "should_not_appear_info";
    LOG(kWarn) << "should_not_appear_warn";
    LOG(kError) << "should_not_appear_error";
    LOG(kFatal) << "should_not_appear_fatal";

    EXPECT_EQ(oss.str().find("should_not_appear"), std::string::npos);
}

} // namespace to_gif
