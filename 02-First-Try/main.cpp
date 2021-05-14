#include "core.h"

bool func1(int32_t v)
{
    return true;
}

bool func2(int32_t v)
{
    return false;
}

template <typename V, typename U>
void testMatch(V const &input, U const &expected)
{
    auto x = match(input)(
        pattern(1) = func1,
        pattern(2) = func2
    );
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
    testMatch(2, false);
    return 0;
}