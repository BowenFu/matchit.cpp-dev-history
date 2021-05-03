#include "match.h"
#include <iostream>

template <typename U>
std::string toString(std::optional<U> const &op)
{
    if (op)
    {
        return std::to_string(op.value());
    }
    else
    {
        return std::string("no value");
    }
}

template <typename V, typename U>
void testMatch(V const &input, std::optional<U> const &expected)
{
    auto x = match(input)(
        pattern(1) = [](int32_t v) -> int32_t { std::cout << "pattern 1" << std::endl;  return 2* v; },
        pattern(2) = [](int32_t v) -> int32_t { std::cout << "pattern 2" << std::endl;  return v+1; },
        pattern(Wildcard<int32_t>{}) = [](int32_t v) -> int32_t { std::cout << "pattern 3" << std::endl;  return v >> 1; },
        pattern(Wildcard<bool>{}) = [](bool v) -> int32_t { std::cout << "pattern 4" << std::endl; return !v; });
    if (x != expected)
    {
        std::cout << "Failure: result = " << toString(x) << ", expected = " << toString(expected) << "!" << std::endl;
    }
    else
    {
        std::cout << "Test passes!" << std::endl;
    }
}

int32_t main()
{
    testMatch(1, std::optional<int32_t>(2));
    testMatch(2, std::optional<int32_t>(3));
    testMatch(true, std::optional<bool>(false));
    testMatch(false, std::optional<bool>(true));
    testMatch(100, std::optional<int32_t>(50));
    testMatch(5l, std::optional<int32_t>());
    return 0;
}