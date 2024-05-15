#include <gtest/gtest.h>

#include "str_split.hh"
#include "str_util.hh"

using namespace std::literals;

namespace {
using waxwing::str_util::split;
TEST(Split, CharacterSeparatorBasic) {
    auto iter = split("hello world", ' ');
    const std::vector<std::string_view> words{"hello", "world"};

    size_t i = 0;
    for (std::optional<std::string_view> word = iter.next(); word.has_value();
         word = iter.next()) {
        ASSERT_LT(i, words.size());
        EXPECT_EQ(*word, words[i]);
        i++;
    }
    EXPECT_EQ(i, words.size());
}

TEST(Split, CharacterSeparatorEmptyWords) {
    auto iter = split(" hello  world ", ' ');
    const std::vector<std::string_view> words{"", "hello", "", "world", ""};

    size_t i = 0;
    for (std::optional<std::string_view> word = iter.next(); word.has_value();
         word = iter.next()) {
        ASSERT_LT(i, words.size());
        EXPECT_EQ(*word, words[i]);
        i++;
    }
    EXPECT_EQ(i, words.size());
}

TEST(Split, CharacterSeparatorSingleItem) {
    auto iter = split("hello", ' ');
    const std::vector<std::string_view> words{"hello"};

    size_t i = 0;
    for (std::optional<std::string_view> word = iter.next(); word.has_value();
         word = iter.next()) {
        ASSERT_LT(i, words.size());
        EXPECT_EQ(*word, words[i]);
        i++;
    }
    EXPECT_EQ(i, words.size());
}

TEST(Split, MultiCharacterSeparatorBasic) {
    auto iter = split("helloabaworld", "aba");
    const std::vector<std::string_view> words{"hello", "world"};

    size_t i = 0;
    for (std::optional<std::string_view> word = iter.next(); word.has_value();
         word = iter.next()) {
        ASSERT_LT(i, words.size());
        EXPECT_EQ(*word, words[i]);
        i++;
    }
    EXPECT_EQ(i, words.size());
}

TEST(Split, MultiCharacterSeparatorEmptyWords) {
    auto iter = split("abahelloabaabaworldaba", "aba");
    const std::vector<std::string_view> words{"", "hello", "", "world", ""};

    size_t i = 0;
    for (std::optional<std::string_view> word = iter.next(); word.has_value();
         word = iter.next()) {
        ASSERT_LT(i, words.size());
        EXPECT_EQ(*word, words[i]);
        i++;
    }
    EXPECT_EQ(i, words.size());
}

TEST(Split, MultiCharacterSeparatorSingleItem) {
    auto iter = split("hello", "  ");
    const std::vector<std::string_view> words{"hello"};

    size_t i = 0;
    for (std::optional<std::string_view> word = iter.next(); word.has_value();
         word = iter.next()) {
        ASSERT_LT(i, words.size());
        EXPECT_EQ(*word, words[i]);
        i++;
    }
    EXPECT_EQ(i, words.size());
}

TEST(Split, CharacterSeparatorRemaining) {
    auto iter = split("The quick brown fox jumps over the lazy dog", ' ');
    iter.next();
    iter.next();
    EXPECT_EQ(iter.remaining(), "brown fox jumps over the lazy dog"sv);
}

TEST(Split, MultiCharacterSeparatorRemaining) {
    auto iter =
        split("The  quick  brown  fox  jumps  over  the  lazy  dog", "  ");
    iter.next();
    iter.next();
    EXPECT_EQ(iter.remaining(), "brown  fox  jumps  over  the  lazy  dog"sv);
}

using waxwing::str_util::ltrim;
using waxwing::str_util::rtrim;
using waxwing::str_util::trim;
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

using waxwing::str_util::to_lower;
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
