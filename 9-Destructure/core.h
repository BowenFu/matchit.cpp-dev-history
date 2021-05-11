#ifndef _CORE_H_
#define _CORE_H_
#include <tuple>
#include <optional>
#include <cstdint>
#include <algorithm>

template <typename Value, typename... PatternPair>
class PatternPairsRetType
{
public:
    using RetType = std::common_type_t<typename PatternPair::template RetType<Value>...>;
};

template <typename Value>
class MatchHelper
{
public:
    explicit MatchHelper(Value const& value)
        : mValue{value}
        {}
    template <typename... PatternPair>
    auto operator()(PatternPair const&... patterns)
    {
        using RetType = typename PatternPairsRetType<Value, PatternPair...>::RetType;
        RetType result{};
        auto const func = [this, &result](auto const& pattern) -> bool
        {
            if (pattern.match(mValue))
            {
                result = pattern.execute(mValue);
                return true;
            }
            return false;
        };
        bool const matched = (func(patterns) || ...);
        assert(matched);
        return result;
    }
private:
    Value const mValue;
};

template <typename Value>
MatchHelper<Value> match(Value const& value)
{
    return MatchHelper<Value>{value};
}

template <typename... T>
class Debug;

template <typename First, typename... Values>
auto match(First const& first, Values const&... values)
{
    std::tuple<First, Values...> const x = std::make_tuple(first, values...);
    return MatchHelper<decltype(x)>{x};
}
#endif // _CORE_H_