#include "core.h"
#include "patterns.h"
#include <string>
#include <iostream>

bool func1()
{
    return true;
}

int64_t func2()
{
    return 12;
}

template <typename V, typename U>
void testMatch(V const &input, U const &expected)
{
    Id<int> ii;
    ii.match(5);
    auto x = match(input)(
        pattern(1) = func1,
        pattern(2) = func2,
        pattern(or_(56, 59)) = func2,
        // pattern(when([](auto&& x){return x < 0; })) = [](int32_t){ return -1; },
        pattern(_ < 0) = []{ return -1; },
        pattern(_ < 10) = []{ return -10; },
        pattern(and_(_ < 17, _ > 15)) = []{ return 16; },
        pattern(app([](int32_t x){return x*x; }, when([](auto&& x){return x > 1000; }))) = []{ return 1000; },
        pattern(app([](int32_t x){return x*x; }, ii)) = [&ii]{ return ii.value(); },
        pattern(ii) = [&ii]{ return ii.value() + 1; },
        pattern(_) = []{ return 111; }
    );
    static_assert(std::is_same_v<int64_t, decltype(x)>);
    if (x == expected)
    {
        printf("Passed!\n");
    }
    else
    {
        printf("Failed! result: %lld, expected: %d\n", x, expected);
    }
}

int32_t main()
{
    testMatch(1, true);
    testMatch(2, 12);
    testMatch(11, 121); // Id matched.
    testMatch(59, 12); // or_ matched.
    testMatch(-5, -1); // when matched.
    testMatch(10, 100); // app matched.
    testMatch(100, 1000); // app > when matched.
    testMatch(5, -10); // _ < 10 matched.
    testMatch(16, 16); // and_ matched.

    Id<int> n;
    auto s = match(10000)(
        pattern(0) = [] { return ""; }, // nothing to say
        pattern(1) = [] { return "A rabbit is nosing around in the clover."; },
        pattern(and_(n, _ <= 1000)) = [&n] { return "There are" + std::to_string(*n) + "rabbits hopping about in the meadow"; },
        pattern(app([](int i) { return i / 1000; }, n)) = [&n] { return "There are " + std::to_string(*n) + " kilo rabbits hopping about in the meadow"; });
    std::cout << s;
    return 0;
}