#include "core.h"
#include "patterns.h"
#include <variant>
#include <array>
#include <any>


template <typename V, typename U>
void compare(V const &result, U const &expected)
{
    if (result == expected)
    {
        printf("Passed!\n");
    }
    else
    {
        printf("Failed!\n");
        if constexpr(std::is_same_v<U, int>)
        {
            std::cout << result << " != " << expected << std::endl;
        }
    }
}

template <typename V, typename U, typename Func>
void testMatch(V const &input, U const &expected, Func matchFunc)
{
    auto const x = matchFunc(input);
    compare(x, expected);
}

bool func1()
{
    return true;
}

int64_t func2()
{
    return 12;
}

void test1()
{
    auto const matchFunc = [](int32_t input)
    {
        Id<int> ii;
        // ii.match(5);
        return match(input)(
            pattern(1) = func1,
            pattern(2) = func2,
            pattern(or_(56, 59)) = func2,
            // pattern(when([](auto&& x){return x < 0; })) = [](int32_t){ return -1; },
            pattern(_ < 0) = []{ return -1; },
            pattern(_ < 10) = []{ return -10; },
            pattern(and_(_ < 17, _ > 15)) = []{ return 16; },
            pattern(app([](int32_t x){return x*x; }, when([](auto&& x){return x > 1000; }))) = []{ return 1000; },
            pattern(app([](int32_t x){return x*x; }, ii)) = [&ii]{ return ii.value() + 0; },
            pattern(ii) = [&ii]{ return ii.value() + 1; },
            pattern(_) = []{ return 111; }
        );
    };
    testMatch(1, true, matchFunc);
    testMatch(2, 12, matchFunc);
    testMatch(11, 121, matchFunc); // Id matched.
    testMatch(59, 12, matchFunc); // or_ matched.
    testMatch(-5, -1, matchFunc); // when matched.
    testMatch(10, 100, matchFunc); // app matched.
    testMatch(100, 1000, matchFunc); // app > when matched.
    testMatch(5, -10, matchFunc); // _ < 10 matched.
    testMatch(16, 16, matchFunc); // and_ matched.
}

void test2()
{
    auto const matchFunc = [](std::tuple<char, int, int> const& input)
    {
        Id<int> i;
        Id<int> j;
        auto [op, lhs, rhs] = input;
        return match(op, lhs, rhs)(
        // return match(input)(
            pattern(ds('/', 1, 1)) = []{ return 1; },
            pattern(ds('/', 0, _)) = []{ return 0; },
            // pattern(ds('/', _)) = []{ return 0; },
            pattern(ds('*', i, j)) = [&i, &j]{ return i.value() * j.value(); },
            pattern(ds('+', i, j)) = [&i, &j]{ return i.value() + j.value(); },
            pattern(_) = [&i, &j]{ return -1; }
        );
    };
    testMatch(std::make_tuple('/', 1, 1), 1, matchFunc);
    testMatch(std::make_tuple('+', 2, 1), 3, matchFunc);
    testMatch(std::make_tuple('/', 0, 1), 0, matchFunc);
    testMatch(std::make_tuple('*', 2, 1), 2, matchFunc);
    testMatch(std::make_tuple('/', 2, 1), -1, matchFunc);
}

struct A
{
    int a;
    int b;
};
bool operator == (A const lhs, A const rhs)
{
    return lhs.a == rhs.a && lhs.b == rhs.b;
}
void test3()
{
    auto const matchFunc = [](A const& input)
    {
        Id<int> i;
        Id<int> j;
        Id<A> a;
        // compose patterns for destructuring struct A.
        auto const dsA = [](Id<int>& x)
        {
            return and_(app(&A::a, x), app(&A::b, 1));
        };
        return match(input)(
            pattern(dsA(i)) = [&i]{ return i.value(); },
            pattern(_) = []{ return -1; }
        );
    };
    testMatch(A{3, 1}, 3, matchFunc);
    testMatch(A{2, 2}, -1, matchFunc);
}

enum class Kind
{
    kZERO,
    kONE,
    kTWO
};

class K
{
public:
    virtual ~K() = default;
    virtual Kind kind() const
    {
        return Kind::kZERO;
    }
};

class One : public K
{
public:
    Kind kind() const override
    {
        return Kind::kONE;
    }
    int get() const
    {
        return 1;
    }
};

class Two : public K
{
public:
    Kind kind() const override
    {
        return Kind::kTWO;
    }
    int get() const
    {
        return 2;
    }
};

bool operator==(One const&, One const&)
{
    return true;
}

bool operator==(Two const&, Two const&)
{
    return true;
}

template <Kind k>
auto const kind = app(&K::kind, k);

template <typename T>
auto const cast = [](auto const& input){
    return static_cast<T const&>(input);
}; 

template <typename T, Kind k>
auto const as = [](Id<T> const& id)
{
    return and_(kind<k>, app(cast<T>, id));
};

void test4()
{
    auto const matchFunc = [](K const& input)
    {
        Id<One> one;
        Id<Two> two;
        return match(input)(
            pattern(as<One, Kind::kONE>(one)) = [&one]{ return one.value().get(); },
            pattern(kind<Kind::kTWO>) = []{ return 2; },
            pattern(_) = []{return 3;}
        );
    };
    testMatch(One{}, 1, matchFunc);
    testMatch(Two{}, 2, matchFunc);
}

void test5()
{
    auto const matchFunc = [](std::pair<int32_t, int32_t> ij)
    {
        return match(ij.first%3, ij.second%5)(
            pattern(0, 0) = []{return 1;},
            pattern(0, _ > 2) = []{return 2;},
            pattern(_, _ > 2) = []{return 3;},
            pattern(_) = []{return 4;}
        );
    };
    testMatch(std::make_pair(3, 5), 1, matchFunc);
    testMatch(std::make_pair(3, 4), 2, matchFunc);
    testMatch(std::make_pair(4, 4), 3, matchFunc);
    testMatch(std::make_pair(4, 1), 4, matchFunc);
}

int32_t fib(int32_t n)
{
    assert(n>0);
    return match(n)
    (
        pattern(1) = []{return 1;},
        pattern(2) = []{return 1;},
        pattern(_) = [n]{return fib(n-1) + fib(n-2);}
    );
}

void test6()
{
    compare(fib(1), 1);
    compare(fib(2), 1);
    compare(fib(3), 2);
    compare(fib(4), 3);
    compare(fib(5), 5);
}

void test7()
{
    auto const matchFunc = [](std::pair<int32_t, int32_t> ij)
    {
        Id<std::tuple<int32_t const&, int32_t const&>> id;
        // delegate at to and_
        auto const at = [](auto&& id, auto&& pattern)
        {
            return and_(id, pattern);
        };
        return match(ij.first%3, ij.second%5)(
            pattern(0, _ > 2) = []{return 2;},
            pattern(ds(1, _ > 2)) = []{return 3;},
            pattern(at(id, ds(_, 2))) = [&id]{assert(std::get<1>(id.value()) == 2); return 4;},
            pattern(_) = []{return 5;}
        );
    };
    testMatch(std::make_pair(4, 2), 4, matchFunc);
}

void test8()
{
    auto const equal = [](std::pair<int32_t, std::pair<int32_t, int32_t>> ijk)
    {
        Id<int32_t> x;
        return match(ijk)(
            pattern(ds(x, ds(_, x))) = []{return true;},
            pattern(_) = []{return false;}
        );
    };
    testMatch(std::make_pair(2, std::make_pair(1, 2)), true, equal);
    testMatch(std::make_pair(2, std::make_pair(1, 3)), false, equal);
}

// optional
void test9()
{
    auto const optional = [](auto const& i)
    {
        Id<int32_t> x;
        auto const some = [](auto const& id)
        {
            auto deref = [](auto&& x) { return *x; };
            return and_(app(cast<bool>, true), app(deref, id));
        };
        auto const none = app(cast<bool>, false);
        return match(i)(
            pattern(some(x)) = []{return true;},
            pattern(none) = []{return false;}
        );
    };
    testMatch(std::make_unique<int32_t>(2), true, optional);
    testMatch(std::unique_ptr<int32_t>{}, false, optional);
    testMatch(std::make_optional<int32_t>(2), true, optional);
    testMatch(std::optional<int32_t>{}, false, optional);
    int32_t* p = nullptr;
    testMatch(p, false, optional);
    int a = 3;
    testMatch(&a, true, optional);
}

struct Shape { virtual ~Shape() = default; };
struct Circle : Shape {};
struct Square : Shape {};

auto const some = [](auto const& id)
{
    auto deref = [](auto&& x) { return *x; };
    return and_(app(cast<bool>, true), app(deref, id));
};
auto const none = app(cast<bool>, false);

template <typename T>
auto const dynAs = [](auto&& id)
{
    auto dynCast = [](auto&& p){return dynamic_cast<T const*>(&p); };
    return app(dynCast, some(id));
};

void test10()
{
    auto const dynCast = [](auto const& i)
    {
        return match(i)(
            pattern(some(dynAs<Circle>(_))) = []{return std::string("Circle");},
            pattern(some(dynAs<Square>(_))) = []{return std::string("Square");},
            pattern(none) = []{return std::string("None");}
        );
    };

    testMatch(std::make_unique<Square>(), "Square", dynCast);
    testMatch(std::make_unique<Circle>(), "Circle", dynCast);
    testMatch(std::unique_ptr<Circle>(), "None", dynCast);
}

template <typename T>
auto const getAs = [](auto&& id)
{
    auto getIf = [](auto&& p){return std::get_if<T>(std::addressof(p)); };
    return app(getIf, some(id));
};

void test11()
{
    auto const getIf = [](auto const& i)
    {
        return match(i)(
            pattern(getAs<Square>(_)) = []{return std::string("Square");},
            pattern(getAs<Circle>(_)) = []{return std::string("Circle");}
        );
    };

    std::variant<Square, Circle> sc;
    sc = Square{};
    testMatch(sc, "Square", getIf);
    sc = Circle{};
    testMatch(sc, "Circle", getIf);
}

void test12()
{
    auto const dsArray = [](auto const& v)
    {
        Id<int> i;
        return match(v)(
            pattern(ds(_, i)) = [&i]{return *i;},
            pattern(ds(_, _, i)) = [&i]{return *i;}
        );
    };

    testMatch(std::array<int, 2>{1, 2}, 2, dsArray);
    testMatch(std::array<int, 3>{1, 2, 3}, 3, dsArray);
}

template <size_t I>
constexpr auto get(A const& a)
{
    if constexpr(I==0)
    {
        return a.a;
    }
    else if constexpr(I==1)
    {
        return a.b;
    }
}

namespace std
{
template <>
class tuple_size<A> : public std::integral_constant<size_t, 2> {};
} // namespace std

void test13()
{
    auto const dsAgg = [](auto const& v)
    {
        Id<int> i;
        return match(v)(
            pattern(ds(1, i)) = [&i]{return *i;},
            pattern(ds(_, i)) = [&i]{return *i;}
        );
    };

    testMatch(A{1, 2}, 2, dsAgg);
    testMatch(A{2, 1}, 1, dsAgg);
}

template <typename T>
auto const anyAs = [](auto&& id)
{
    auto anyCast = [](auto&& p){return std::any_cast<T>(std::addressof(p)); };
    return app(anyCast, some(id));
};

void test14()
{
    auto const anyCast = [](auto const& i)
    {
        return match(i)(
            pattern(anyAs<Square>(_)) = []{return std::string("Square");},
            pattern(anyAs<Circle>(_)) = []{return std::string("Circle");}
        );
    };

    std::any sc;
    sc = Square{};
    testMatch(sc, "Square", anyCast);
    sc = Circle{};
    testMatch(sc, "Circle", anyCast);
}

int main()
{
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();
    test10();
    test11();
    test12();
    test13();
    test14();
    return 0;
}