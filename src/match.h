#include <tuple>
#include <optional>
#include <cstdint>

using std::int32_t;

template <typename... Types>
using FirstType = std::tuple_element_t<0, std::tuple<Types...>>;

template <typename Value>
class MatchHelper
{
public:
    explicit MatchHelper(Value const& value)
        : mValue{value}
        {}
    template <typename... PatternHelper>
    auto operator()(PatternHelper const&... patterns)
    {
        // TODO: common type of all.
        typename FirstType<PatternHelper...>::template RetType<Value> result;
        auto const func = [this, &result](auto const& pattern)
        {
            result = pattern.conditionalExecute(mValue);
            return result;
        };
        (func(patterns) || ...);
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

// default value type == pattern type
template <typename Value>
class PatternPolicy
{
public:
    static bool check(Value const& pattern, Value const& value)
    {
        return pattern == value;
    }

    // Avoid any implicit casts.
    template <typename Value2>
    static bool check(Value const& pattern, Value2 const&)
    {
        return false;
    }
};

template <typename Value>
class Wildcard
{
};

template <typename WildType>
class PatternPolicy<Wildcard<WildType>>
{
public:
    static bool check(Wildcard<WildType> const&, WildType const&)
    {
        return true;
    }
    template <typename Value>
    static bool check(Wildcard<WildType> const&, Value const&)
    {
        return false;
    }
};

template <typename Pattern, typename Func>
class PatternPair
{
public:
    template <typename Value>
    using RetType = std::optional<std::result_of_t<Func(Value)>>;

    PatternPair(Pattern const& pattern, Func const& func)
        : mPattern{pattern}
        , mHandler{func}
    {
    }
    template <typename Value>
    auto conditionalExecute(Value const& value) const -> RetType<Value>
    {
        if (check(value))
        {
            return std::optional{execute(value)};
        }
        return {};
    }
private:
    template <typename Value>
    bool check(Value const& value) const
    {
        return PatternPolicy<Pattern>::check(mPattern, value);
    }
    template <typename Value>
    auto execute(Value const& value) const
    {
        return mHandler(value);
    }
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