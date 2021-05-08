#include <tuple>
#include <optional>
#include <cstdint>
#include <algorithm>

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
        auto const pats = {patterns...};
        auto const iter = std::find_if(pats.begin(), pats.end(), [this](auto const& pat){return pat.match(mValue);});
        assert(iter != pats.end());
        return iter->execute(mValue);
    }
private:
    Value const& mValue;
};

template <typename Value>
MatchHelper<Value> match(Value const& value)
{
    return MatchHelper<Value>{value};
}

template <typename Pattern, typename Func>
class PatternPair
{
public:
    PatternPair(Pattern const& pattern, Func const& func)
        : mPattern{pattern}
        , mHandler{func}
    {
    }
    template <typename Value>
    bool match(Value const& value) const
    {
        return mPattern == value;
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