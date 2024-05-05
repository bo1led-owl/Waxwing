#include <gtest/gtest.h>

#include "str_split.hh"
#include "str_util.hh"

namespace {
using namespace http::str_util;

TEST(Split, CharacterSeparatorBasic) {
    auto iter = split("hello world", ' ');
    const std::vector<std::string_view> words {"hello", "world"};

    int i = 0;
    for (auto word : iter) {
        EXPECT_EQ(word, words[i]);
        i++;
    }
}

TEST(Split, CharacterSeparatorEmptyWords) {
    auto iter = split(" hello  world ", ' ');
    const std::vector<std::string_view> words {"", "hello", "", "world", ""};

    int i = 0;
    for (auto word : iter) {
        EXPECT_EQ(word, words[i]);
        i++;
    }
}

TEST(Split, CharacterSeparatorSingleItem) {
    auto iter = split("hello", ' ');
    const std::vector<std::string_view> words {"hello"};  

    int i = 0;
    for (auto word : iter) {
        EXPECT_EQ(word, words[i]);
        i++;
    }
}

TEST(Split, MultiCharacterSeparatorBasic) {
    auto iter = split("helloabaworld", "aba");
    const std::vector<std::string_view> words {"hello", "world"};

    int i = 0;
    for (auto word : iter) {
        EXPECT_EQ(word, words[i]);
        i++;
    }
}

TEST(Split, MultiCharacterSeparatorEmptyWords) {
    auto iter = split("abahelloabaabaworldaba", "aba");
    const std::vector<std::string_view> words {"", "hello", "", "world", ""};

    int i = 0;
    for (auto word : iter) {
        EXPECT_EQ(word, words[i]);
        i++;
    }
}

TEST(Split, MultiCharacterSeparatorSingleItem) {
    auto iter = split("hello", "  ");
    const std::vector<std::string_view> words {"hello"};

    int i = 0;
    for (auto word : iter) {
        EXPECT_EQ(word, words[i]);
        i++;
    }
}

TEST(Trim, LTrim) {
    const std::vector<std::pair<std::string_view, std::string_view>> tests{
        {ltrim(" \t\nhello "), "hello "},
        {ltrim("hello "), "hello "},
    };
    for (const auto& test : tests) {
        EXPECT_EQ(test.first, test.second);
    }
}

TEST(Trim, RTrim) {
    const std::vector<std::pair<std::string_view, std::string_view>> tests{
        {rtrim(" hello \t\n"), " hello"},
        {rtrim(" hello"), " hello"},
    };
    for (const auto& test : tests) {
        EXPECT_EQ(test.first, test.second);
    }
}

TEST(Trim, Trim) {
    const std::vector<std::pair<std::string_view, std::string_view>> tests{
        {trim(" \t\nhello \t\n"), "hello"},
        {trim("hello"), "hello"},
    };
    for (const auto& test : tests) {
        EXPECT_EQ(test.first, test.second);
    }
}

TEST(ToLower, Basic) {
    const std::vector<std::pair<std::string, std::string_view>> tests{
        {to_lower("hElLo"), "hello"},
        {to_lower("hello \t\n"), "hello \t\n"},
    };
    for (const auto& test : tests) {
        EXPECT_EQ(test.first, test.second);
    }
}
}  // namespace
