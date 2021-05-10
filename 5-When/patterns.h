#ifndef _PATTERNS_H_
#define _PATTERNS_H_

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
    using Pattern = WildCard;
public:
    template <typename Value>
    static bool match(Pattern const&, Value const&)
    {
        return true;
    }
};


template <typename... Patterns>
class Or
{
public:
    explicit Or(Patterns const&... patterns)
        : mPatterns{patterns...}
        {}
    auto const& patterns() const
    {
        return mPatterns;
    }
private:
    std::tuple<Patterns const&...> mPatterns;
};

template <typename... Patterns>
auto or_(Patterns const&... patterns) -> Or<Patterns...>
{
    return Or<Patterns...>{patterns...};
}

template <typename... Patterns>
class PatternTraits<Or<Patterns...>>
{
public:
    template <typename Value>
    static bool match(Or<Patterns...> const& orPat, Value const& value)
    {
        return std::apply(
            [&value](Patterns const&... patterns)
            {
                return (PatternTraits<Patterns>::match(patterns, value) || ...);
            },
            orPat.patterns());
    }
};

template <typename Pred>
class When
{
public:
    explicit When(Pred const& pred)
        : mPred{pred}
        {}
    auto const& predicate() const
    {
        return mPred;
    }
private:
    Pred const& mPred;
};

template <typename Pred>
auto when(Pred const& pred)
{
    return When<Pred>{pred};
}

template <typename Pred>
class PatternTraits<When<Pred>>
{
public:
    template <typename Value>
    static bool match(When<Pred> const& whenPat, Value const& value)
    {
        return whenPat.predicate()(value);
    }
};

#endif // _PATTERNS_H_