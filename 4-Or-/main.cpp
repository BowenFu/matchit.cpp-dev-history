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
        pattern(_) = [](int32_t){ return 111; }
    );
    static_assert(std::is_same_v<int64_t, decltype(x)>);
    if (x == expected)
    {
        printf("Passed!\n");
    }
    else
    {
        printf("Failed!\n");
    }
}

int32_t main()
{
    testMatch(1, true);
    testMatch(2, 12);
    testMatch(55, 111); // _ matched.
    testMatch(59, 12); // or_ matched.
    return 0;
}