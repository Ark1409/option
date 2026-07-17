#include <option/optional.hpp>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <type_traits>
#include <iterator>

using namespace option;

TEST(OptionalTest, InheritsTriviality) {
    EXPECT_FALSE(std::is_trivially_copy_constructible<optional<std::string>>::value);
    EXPECT_FALSE(std::is_trivially_move_constructible<optional<std::string>>::value);
    EXPECT_FALSE(std::is_trivially_destructible<optional<std::string>>::value);
    EXPECT_FALSE(std::is_trivially_copy_assignable<optional<std::string>>::value);
    EXPECT_FALSE(std::is_trivially_move_assignable<optional<std::string>>::value);

    EXPECT_TRUE(std::is_trivially_copy_constructible<optional<int>>::value);
    EXPECT_TRUE(std::is_trivially_move_constructible<optional<int>>::value);
    EXPECT_TRUE(std::is_trivially_destructible<optional<int>>::value);
    EXPECT_TRUE(std::is_trivially_copy_assignable<optional<int>>::value);
    EXPECT_TRUE(std::is_trivially_move_assignable<optional<int>>::value);

    EXPECT_TRUE(std::is_trivially_copy_constructible<optional<bool>>::value);
    EXPECT_TRUE(std::is_trivially_move_constructible<optional<bool>>::value);
    EXPECT_TRUE(std::is_trivially_destructible<optional<bool>>::value);
    EXPECT_TRUE(std::is_trivially_copy_assignable<optional<bool>>::value);
    EXPECT_TRUE(std::is_trivially_move_assignable<optional<bool>>::value);

    EXPECT_TRUE(std::is_trivially_copy_constructible<optional<int&>>::value);
    EXPECT_TRUE(std::is_trivially_move_constructible<optional<int&>>::value);
    EXPECT_TRUE(std::is_trivially_destructible<optional<int&>>::value);
    EXPECT_TRUE(std::is_trivially_copy_assignable<optional<int&>>::value);
    EXPECT_TRUE(std::is_trivially_move_assignable<optional<int&>>::value);
}

TEST(OptionalTest, SpecializationImprovesSize) {
    EXPECT_EQ(sizeof(optional<bool>), sizeof(bool));
    EXPECT_EQ(sizeof(optional<int&>), sizeof(int*));
}

TEST(OptionalTest, ShouldBeEmpty) {
    {
        optional<std::string> o{};
        EXPECT_FALSE(o.has_value());

        optional<int> o2{};
        EXPECT_FALSE(o2.has_value());

        optional<bool> o3{};
        EXPECT_FALSE(o3.has_value());

        optional<bool&> o4{};
        EXPECT_FALSE(o4.has_value());
    }
    {
        optional<std::string> o{nullopt};
        EXPECT_FALSE(o.has_value());

        optional<int> o2{nullopt};
        EXPECT_FALSE(o2.has_value());

        optional<bool> o3{nullopt};
        EXPECT_FALSE(o3.has_value());

        optional<bool&> o4{nullopt};
        EXPECT_FALSE(o4.has_value());
    }
}

TEST(OptionalTest, ShouldNotBeEmpty) {
    optional<std::string> o{"Test"};
    EXPECT_TRUE(o.has_value());
    EXPECT_EQ(*o, "Test");

    optional<int> o2{3};
    EXPECT_TRUE(o2.has_value());
    EXPECT_EQ(*o2, 3);

    optional<bool> o3{false};
    EXPECT_TRUE(o3.has_value());
    EXPECT_EQ(*o3, false);

    bool b = false;
    optional<bool&> o4{b};
    EXPECT_TRUE(o4.has_value());
    EXPECT_EQ(*o4, false);
}

TEST(OptionalTest, ShouldOnlyBeMovable) {
    struct S { S() = default; S(const S&) = delete; S(S&&) = default; };
    EXPECT_FALSE(std::is_copy_constructible<optional<S>>::value);
    EXPECT_TRUE(std::is_move_constructible<optional<S>>::value);
    EXPECT_TRUE(std::is_trivially_destructible<optional<S>>::value);
}

TEST(OptionalTest, ShouldBeNonMoveable) {
    struct S { S() = default; S(const S&) = delete; S(S&&) = delete; };
    EXPECT_FALSE(std::is_copy_constructible<optional<S>>::value);
    EXPECT_FALSE(std::is_move_constructible<optional<S>>::value);
    EXPECT_TRUE(std::is_trivially_destructible<optional<S>>::value);
}

TEST(OptionalTest, ShouldOnlyBeMoveAssignable) {
    struct S { S() = default; S(const S&) = default; S(S&&) = default; S& operator=(const S&) = delete; S& operator=(S&&) = default; };
    EXPECT_TRUE(std::is_copy_constructible<optional<S>>::value);
    EXPECT_TRUE(std::is_move_constructible<optional<S>>::value);
    EXPECT_FALSE(std::is_copy_assignable<optional<S>>::value);
    EXPECT_TRUE(std::is_move_assignable<optional<S>>::value);
    EXPECT_TRUE(std::is_trivially_destructible<optional<S>>::value);
}


TEST(OptionalTest, ShouldBeNonMoveAssignable) {
    struct S { S() = default; S(const S&) = default; S(S&&) = default; S& operator=(const S&) = delete; S& operator=(S&&) = delete; };
    EXPECT_TRUE(std::is_copy_constructible<optional<S>>::value);
    EXPECT_TRUE(std::is_move_constructible<optional<S>>::value);
    EXPECT_FALSE(std::is_copy_assignable<optional<S>>::value);
    EXPECT_FALSE(std::is_move_assignable<optional<S>>::value);
    EXPECT_TRUE(std::is_trivially_destructible<optional<S>>::value);
}

TEST(OptionalTest, ShouldBeEmptyIterator) {
    optional<int> o{};
    EXPECT_EQ(o.begin(), o.end());

    optional<bool> o1{};
    EXPECT_EQ(o1.begin(), o1.end());

    optional<bool&> o2{};
    EXPECT_EQ(o2.begin(), o2.end());
}

TEST(OptionalTest, ShouldBeNonEmptyIterator) {
    optional<int> o{1};
    EXPECT_NE(o.begin(), o.end());

    optional<bool> o1{false};
    EXPECT_NE(o1.begin(), o1.end());

    bool b;
    optional<bool&> o2{b};
    EXPECT_NE(o2.begin(), o2.end());
}

template<typename T>
using remove_cvref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

TEST(OptionalTest, ShouldBeRandomAccessIterator) {
    using OptionT = optional<int>;
    EXPECT_TRUE((std::is_same<remove_cvref_t<std::random_access_iterator_tag>, remove_cvref_t<typename std::iterator_traits<typename OptionT::iterator>::iterator_category>>::value
            || std::is_base_of<remove_cvref_t<std::random_access_iterator_tag>, remove_cvref_t<typename std::iterator_traits<typename OptionT::iterator>::iterator_category>>::value));
}

#if __cplusplus >= 202002L
TEST(OptionalTest, ShouldBeContiguousIterator) {
    using OptionT = optional<int>;
    EXPECT_TRUE((std::is_same<remove_cvref_t<std::contiguous_iterator_tag>, remove_cvref_t<typename std::iterator_traits<typename OptionT::iterator>::iterator_category>>::value
            || std::is_base_of<remove_cvref_t<std::contiguous_iterator_tag>, remove_cvref_t<typename std::iterator_traits<typename OptionT::iterator>::iterator_category>>::value));
}
#endif

TEST(OptionalTest, ShouldGetValue) {
    int i;
    optional<int&> o{i};
    EXPECT_EQ(&o.value(), &i);
}

TEST(OptionalTest, ShouldGetDefaultValue) {
    optional<int> o{};
    EXPECT_EQ(o.value_or(1), 1);

    optional<bool> o1{};
    EXPECT_EQ(o1.value_or(true), true);

    bool b=true;
    optional<bool&> o2{};
    EXPECT_EQ(o2.value_or(b), b);
}

TEST(OptionalTest, ShouldReset) {
    optional<int> o{1};
    EXPECT_TRUE(o.has_value());
    o.reset();
    EXPECT_FALSE(o.has_value());

    optional<bool> o1{false};
    EXPECT_TRUE(o1.has_value());
    o1.reset();
    EXPECT_FALSE(o1.has_value());

    bool b;
    optional<bool&> o2{b};
    EXPECT_TRUE(o2.has_value());
    o2.reset();
    EXPECT_FALSE(o2.has_value());
}

TEST(OptionalTest, ShouldSwap) {
    {
        int a = 1, b = 2;
        optional<int> o1{a};
        optional<int> o2{b};
        swap(o1, o2);
        EXPECT_EQ(o1, b);
        EXPECT_EQ(o2, a);
    }

    {
        bool a = false, b = true;
        optional<bool> o1{a};
        optional<bool> o2{b};
        swap(o1, o2);
        EXPECT_EQ(o1, b);
        EXPECT_EQ(o2, a);
    }

    {
        bool a = false, b = true;
        optional<bool&> o1{a};
        optional<bool&> o2{b};
        swap(o1, o2);
        EXPECT_EQ(a, true);
        EXPECT_EQ(b, false);
        EXPECT_EQ(&o1.value(), &a);
        EXPECT_EQ(&o2.value(), &b);
    }
}

TEST(OptionalTest, ShouldEmplace) {
    {
        optional<int> o{1};
        o.emplace(2);
        EXPECT_EQ(o,2);
    }
    {
        optional<bool> o{true};
        o.emplace(false);
        EXPECT_EQ(o,false);
    }
    {
        bool a=true, b=false;
        optional<bool&> o{a};
        o.emplace(b);
        EXPECT_EQ(o,b);
    }
}

TEST(OptionalTest, ShouldTransform) {
    {
        optional<int> o{2};
        auto t = o.transform([] (int& i) { return i == 2; } );
        EXPECT_EQ(t, true);

        optional<int> o1{};
        auto t1 = o1.transform([] (int& i) { return i == 2; } );
        EXPECT_EQ(t1, nullopt);
    }

    {
        optional<bool> o{false};
        auto t = o.transform([] (bool& i) { return i ? 33 : 1; } );
        EXPECT_EQ(t, 1);

        optional<bool> o1{};
        auto t1 = o1.transform([] (bool& i) { return i ? 33 : 1; } );
        EXPECT_EQ(t1, nullopt);
    }

    {
        bool b;
        optional<bool&> o{b};
        auto t = o.transform([] (bool& i) { return i ? 33 : 1; } );
        EXPECT_EQ(t, 1);

        optional<bool&> o1{};
        auto t1 = o1.transform([] (bool& i) { return i ? 33 : 1; } );
        EXPECT_EQ(t1, nullopt);
    }
}

TEST(OptionalTest, ShouldOrElse) {
    {
        optional<int> o{2};
        auto t = o.or_else([]  { return optional<int>{3}; } );
        EXPECT_EQ(t, 2);

        optional<int> o1{};
        auto t1 = o1.or_else([]  { return optional<int>{3}; } );
        EXPECT_EQ(t1, 3);
    }

    {
        optional<bool> o{true};
        auto t = o.or_else([]  { return optional<bool>{false}; } );
        EXPECT_EQ(t, true);

        optional<bool> o1{};
        auto t1 = o1.or_else([]  { return optional<bool>{false}; } );
        EXPECT_EQ(t1, false);
    }

    {
        bool b=false, b2=true;
        optional<bool&> o{b};
        auto t = o.or_else([&]  { return optional<bool&>{b2}; } );
        EXPECT_EQ(t, b);

        optional<bool&> o1{};
        auto t1 = o1.or_else([&]  { return optional<bool&>{b2}; } );
        EXPECT_EQ(t1, b2);
    }
}

TEST(OptionalTest, ShouldAndThen) {
    {
        optional<int> o{2};
        auto t = o.and_then([] (int& i) { return optional<int>{i}; } );
        EXPECT_EQ(t, o);

        optional<int> o1{};
        auto t1 = o1.and_then([] (int& i) { return optional<int>{i}; } );
        EXPECT_EQ(t1, nullopt);
    }

    {
        optional<bool> o{false};
        auto t = o.and_then([] (bool& i) { return optional<bool>{i}; } );
        EXPECT_EQ(t, o);

        optional<bool> o1{};
        auto t1 = o1.and_then([] (bool& i) { return optional<bool>{i}; });
        EXPECT_EQ(t1, nullopt);
    }

    {
        bool b=true;
        optional<bool&> o{b};
        auto t = o.and_then([] (bool& i) { return optional<bool&>{i}; } );
        EXPECT_EQ(t, o);

        optional<bool&> o1{};
        auto t1 = o1.and_then([] (bool& i) { return optional<bool&>{i}; });
        EXPECT_EQ(t1, nullopt);
    }
}


struct SNeq {
    bool operator==(const SNeq& o) const noexcept { if(b) { *b = 1; } return o.b == b; }
    bool operator!=(const SNeq& o) const noexcept { if(b) { *b = 2; } return o.b == b; }
    int* b;
};

template<>
struct optional_traits<SNeq> {
    constexpr static SNeq empty() noexcept { return {nullptr}; }
};

TEST(OptionalTest, EnsureProperComparisonOverload) {
    int n=0;
    optional<SNeq> s{{&n}};

    (void)(bool)s;

    EXPECT_EQ(n, 2);
}
