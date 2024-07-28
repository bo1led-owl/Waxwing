#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

#include "waxwing/str_split.hh"
#include "waxwing/str_util.hh"

using namespace std::literals;

namespace {
using waxwing::str_util::case_insensitive_eq;
using waxwing::str_util::ltrim;
using waxwing::str_util::rtrim;
using waxwing::str_util::split;
using waxwing::str_util::trim;

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

TEST(Trim, LTrim) {
    EXPECT_EQ(ltrim(" \t\nhello "), "hello ");
    EXPECT_EQ(ltrim("hello "), "hello ");
}

TEST(Trim, RTrim) {
    EXPECT_EQ(rtrim(" hello \t\n"), " hello");
    EXPECT_EQ(rtrim(" hello"), " hello");
}

TEST(Trim, Trim) {
    EXPECT_EQ(trim(" \t\nhello \t\n"), "hello");
    EXPECT_EQ(trim("hello"), "hello");
}

TEST(CaseInsensitiveEq, Basic) {
    EXPECT_TRUE(case_insensitive_eq("foo", "foo"));
    EXPECT_TRUE(case_insensitive_eq("   ", "   "));
    EXPECT_TRUE(case_insensitive_eq("FoO", "foo"));
    EXPECT_TRUE(case_insensitive_eq("FoO", "fOO"));

    EXPECT_FALSE(case_insensitive_eq("hello", "bye"));
    EXPECT_FALSE(case_insensitive_eq("foo", "bar"));
    EXPECT_FALSE(case_insensitive_eq("foo", "fo"));
}
}  // namespace
