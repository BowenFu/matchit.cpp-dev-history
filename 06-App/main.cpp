#include "core.h"
#include "patterns.h"

bool func1(int32_t v)
{
    return true;
}

int64_t func2(int32_t v)
{
    return 12;
}

template <typename V, typename U>
void testMatch(V const &input, U const &expected)
{
    auto x = match(input)(
        pattern(1) = func1,
        pattern(2) = func2,
        pattern(or_(56, 59)) = func2,
        // pattern(when([](auto&& x){return x < 0; })) = [](int32_t){ return -1; },
        pattern(_ < 0) = [](int32_t){ return -1; },
        pattern(_ < 10) = [](int32_t){ return -10; },
        pattern(app([](int32_t x){return x*x; }, 100)) = [](int32_t){ return 100; },
        pattern(app([](int32_t x){return x*x; }, when([](auto&& x){return x > 1000; }))) = [](int32_t){ return 1000; },
        pattern(_) = [](int32_t){ return 111; }
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
    testMatch(11, 111); // _ matched.
    testMatch(59, 12); // or_ matched.
    testMatch(-5, -1); // when matched.
    testMatch(10, 100); // app matched.
    testMatch(100, 1000); // app > when matched.
    testMatch(5, -10); // _ < 10 matched.
    return 0;
}