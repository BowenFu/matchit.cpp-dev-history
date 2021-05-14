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
    Value const& mValue;
};

template <typename Value>
MatchHelper<Value> match(Value const& value)
{
    return MatchHelper<Value>{value};
}

template <typename Pattern>
class PatternTraits;

template <typename Pattern, typename Func>
class PatternPair
{
public:
    template <typename Value>
    using RetType = std::invoke_result_t<Func, Value>;

    PatternPair(Pattern const& pattern, Func const& func)
        : mPattern{pattern}
        , mHandler{func}
    {
    }
    template <typename Value>
    bool match(Value const& value) const
    {
        return PatternTraits<Pattern>::match(mPattern, value);
    }
    template <typename Value>
    auto execute(Value const& value) const
    {
        return mHandler(value);
    }
private:
    Pattern const& mPattern;
    Func const& mHandler;
};

template <typename Pattern>
class PatternHelper
{
public:
    explicit PatternHelper(Pattern const& pattern)
        : mPattern{pattern}
        {}
    template <typename Func>
    auto operator=(Func const& func)
    {
        return PatternPair<Pattern, Func>{mPattern, func};
    }
private:
    Pattern const& mPattern;
};

template <typename Pattern>
PatternHelper<Pattern> pattern(Pattern const& p)
{
    return PatternHelper<Pattern>{p};
}

class WildCard{};
constexpr WildCard _;

template <typename Pattern>
class PatternTraits
{
public:
    template <typename Value>
    static bool match(Pattern const& pattern, Value const& value)
    {
        return pattern == value;
    }
};

template <>
class PatternTraits<WildCard>
{
public:
    using Pattern = WildCard;
    template <typename Value>
    static bool match(Pattern const&, Value const&)
    {
        return true;
    }
};
#endif // _CORE_H_