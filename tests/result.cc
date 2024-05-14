#include "waxwing/result.hh"

#include <gtest/gtest.h>

namespace {
using waxwing::Error;
using waxwing::Result;

Result<int, int> correct() { return 1; }

Result<int, int> error() { return Error{0}; }

int throw_on_error() { return correct().error(); }

int throw_on_value() { return error().value(); }

TEST(Result, Basic) {
    auto res = correct();
    EXPECT_EQ(res.value(), 1);
    EXPECT_THROW(throw_on_error(), std::logic_error);

    res = error();
    EXPECT_EQ(res.error(), 0);
    EXPECT_THROW(throw_on_value(), std::logic_error);
}

TEST(Result, HasValue) {
    auto c = correct();
    auto e = error();
    EXPECT_TRUE(c.has_value());
    EXPECT_FALSE(e.has_value());
}

TEST(Result, HasError) {
    auto c = correct();
    auto e = error();
    EXPECT_FALSE(c.has_error());
    EXPECT_TRUE(e.has_error());
}

TEST(Result, Conversion) {
    const std::string corr{"correct"};
    Result<std::string_view, int> sres_corr = corr;
    EXPECT_EQ(sres_corr.value(), corr);

    const std::string err{"error"};
    Result<int, std::string_view> sres_err = Error<std::string_view>{err};
    EXPECT_EQ(sres_err.error(), err);

    Result<std::string, int> sres_corr2{sres_corr};
    EXPECT_EQ(sres_corr2.value(), corr);

    Result<int, std::string> sres_err2{sres_err};
    EXPECT_EQ(sres_err2.error(), err);
}
}  // namespace
