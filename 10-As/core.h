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

template <typename Value, bool byRef>
class ValueType
{
public:
    using ValueT = Value const;
};

// TODO, use a special type for match generate tuples.
// So that we do not copy any values.
template <typename Value>
class ValueType<Value, true>
{
public:
    using ValueT = Value const&;
};

template <typename Value, bool byRef>
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
    typename ValueType<Value, byRef>::ValueT mValue;
};

template <typename Value>
auto match(Value const& value)
{
    return MatchHelper<Value, true>{value};
}

template <typename First, typename... Values>
auto match(First const& first, Values const&... values)
{
    auto const x = std::forward_as_tuple(first, values...);
    return MatchHelper<decltype(x), false>{x};
}
#endif // _CORE_H_