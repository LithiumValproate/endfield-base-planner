#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace doctest {
struct TestCase {
    std::string name;
    std::function<void()> function;
};

inline std::vector<TestCase>& Registry() {
    static std::vector<TestCase> registry;
    return registry;
}

struct Registrar {
    Registrar(std::string name, std::function<void()> function) {
        Registry().push_back({std::move(name), std::move(function)});
    }
};

class TestFailure : public std::runtime_error {
public:
    explicit TestFailure(const std::string& message)
        : std::runtime_error(message) {}
};

inline int RunTests() {
    int failedCount = 0;
    for (const TestCase& testCase : Registry()) {
        try {
            testCase.function();
            std::cout << "[pass] " << testCase.name << '\n';
        } catch (const std::exception& exception) {
            ++failedCount;
            std::cerr << "[fail] " << testCase.name << ": " << exception.what() << '\n';
        }
    }
    return failedCount == 0 ? 0 : 1;
}
}  // namespace doctest

#define DOCTEST_DETAIL_JOIN_IMPL(left, right) left##right
#define DOCTEST_DETAIL_JOIN(left, right) DOCTEST_DETAIL_JOIN_IMPL(left, right)

#define TEST_CASE(name)                                                                                               \
    static void DOCTEST_DETAIL_JOIN(test_case_, __LINE__)();                                                           \
    static ::doctest::Registrar DOCTEST_DETAIL_JOIN(registrar_, __LINE__)(name, DOCTEST_DETAIL_JOIN(test_case_, __LINE__)); \
    static void DOCTEST_DETAIL_JOIN(test_case_, __LINE__)()

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            throw ::doctest::TestFailure(std::string("CHECK failed: ") + #condition);                                 \
        }                                                                                                              \
    } while (false)

#define REQUIRE(condition) CHECK(condition)

#define CHECK_EQ(left, right)                                                                                          \
    do {                                                                                                               \
        const auto& leftValue = (left);                                                                                \
        const auto& rightValue = (right);                                                                              \
        if (!(leftValue == rightValue)) {                                                                              \
            std::ostringstream stream;                                                                                 \
            stream << "CHECK_EQ failed: " << #left << " != " << #right;                                               \
            throw ::doctest::TestFailure(stream.str());                                                                \
        }                                                                                                              \
    } while (false)

#define CHECK_NE(left, right)                                                                                          \
    do {                                                                                                               \
        const auto& leftValue = (left);                                                                                \
        const auto& rightValue = (right);                                                                              \
        if (leftValue == rightValue) {                                                                                 \
            std::ostringstream stream;                                                                                 \
            stream << "CHECK_NE failed: " << #left << " == " << #right;                                               \
            throw ::doctest::TestFailure(stream.str());                                                                \
        }                                                                                                              \
    } while (false)

#ifdef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
int main() {
    return ::doctest::RunTests();
}
#endif
