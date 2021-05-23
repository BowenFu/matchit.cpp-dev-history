#ifndef _CORE_H_
#define _CORE_H_
#include <tuple>
#include <optional>
#include <cstdint>
#include <algorithm>

template <typename... PatternPair>
class PatternPairsRetType
{
public:
    using RetType = std::common_type_t<typename PatternPair::RetType...>;
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
        using RetType = typename PatternPairsRetType<PatternPair...>::RetType;
        RetType result{};
        auto const func = [this, &result](auto const& pattern) -> bool
        {
            if (pattern.match(mValue))
            {
                result = pattern.execute();
                return true;
            }
            return false;
        };
        bool const matched = (func(patterns) || ...);
        assert(matched);
        return result;
    }
private:
    Value const& mValue;
};

template <typename Value>
MatchHelper<Value> match(Value const& value)
{
    return MatchHelper<Value>{value};
}
#endif // _CORE_H_