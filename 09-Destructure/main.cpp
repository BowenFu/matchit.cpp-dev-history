#include "core.h"
#include "patterns.h"

bool func1()
{
    return true;
}

int64_t func2()
{
    return 12;
}

template <typename V, typename U, typename Func>
void testMatch(V const &input, U const &expected, Func matchFunc)
{
    auto const x = matchFunc(input);
    if (x == expected)
    {
        printf("Passed!\n");
    }
    else
    {
        printf("Failed!\n");
    }
}

int32_t test1()
{
    auto const matchFunc = [](int32_t input)
    {
        Id<int> ii;
        ii.match(5);
        return match(input)(
            pattern(1) = func1,
            pattern(2) = func2,
            pattern(or_(56, 59)) = func2,
            // pattern(when([](auto&& x){return x < 0; })) = []{ return -1; },
            pattern(_ < 0) = []{ return -1; },
            pattern(_ < 10) = []{ return -10; },
            pattern(and_(_ < 17, _ > 15)) = []{ return 16; },
            pattern(app([](int32_t x){return x*x; }, when([](auto&& x){return x > 1000; }))) = []{ return 1000; },
            pattern(app([](int32_t x){return x*x; }, ii)) = [&ii]{ return ii.value(); },
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
    return 0;
}

int32_t test2()
{
    auto const matchFunc = [](std::tuple<char, int, int> const& input)
    {
        Id<int> i;
        Id<int> j;
        auto [op, lhs, rhs] = input;
        return match(op, lhs, rhs)(
        // return match(input)(
            pattern(ds('/', 0, _)) = []{ return 0; },
            pattern(ds('*', i, j)) = [&i, &j]{ return i.value() * j.value(); },
            pattern(ds('+', i, j)) = [&i, &j]{ return i.value() + j.value(); },
            pattern(_) = [&i, &j]{ return -1; }
        );
    };
    testMatch(std::make_tuple('+', 2, 1), 3, matchFunc);
    testMatch(std::make_tuple('/', 0, 1), 0, matchFunc);
    testMatch(std::make_tuple('*', 2, 1), 2, matchFunc);
    testMatch(std::make_tuple('/', 2, 1), -1, matchFunc);
    return 0;
}

struct A
{
    int a;
    int b;
};

int32_t test3()
{
    auto const matchFunc = [](A const& input)
    {
        Id<int> i;
        Id<int> j;
        // compose patterns for destructuring struct A.
        auto const dsA = and_(app(&A::a, i), app(&A::b, 1));
        return match(input)(
            // pattern(and_(app(&A::a, i), app(&A::b, j))) = [&i, &j]{ return i.value() + j.value(); },
            pattern(dsA) = [&i]{ return i.value(); },
            pattern(_) = []{ return -1; }
        );
    };
    testMatch(A{2, 1}, 2, matchFunc);
    testMatch(A{2, 2}, -1, matchFunc);
    return 0;
}


int main()
{
    test1();
    test2();
    test3();
    return 0;
}