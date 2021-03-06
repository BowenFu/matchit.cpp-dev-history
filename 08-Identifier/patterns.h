#ifndef _PATTERNS_H_
#define _PATTERNS_H_

template <typename Pattern>
class PatternTraits;

template <typename Pattern, typename Value>
bool match(Pattern const& pattern, Value const& value)
{
    return PatternTraits<Pattern>::match(pattern, value);
}

template <typename Pattern>
void resetId(Pattern const& pattern)
{
    PatternTraits<Pattern>::resetId(pattern);
}

template <typename Pattern, typename Func>
class PatternPair
{
public:
    using RetType = std::invoke_result_t<Func>;

    PatternPair(Pattern const& pattern, Func const& func)
        : mPattern{pattern}
        , mHandler{func}
    {
    }
    template <typename Value>
    bool match(Value const& value) const
    {
        resetId(mPattern);
        return ::match(mPattern, value);
    }
    auto execute() const
    {
        return mHandler();
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
    static void resetId(Pattern const&)
    {
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
    static void resetId(Pattern const&)
    {}
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
                return (::match(patterns, value) || ...);
            },
            orPat.patterns());
    }
    static void resetId(Or<Patterns...> const& orPat)
    {
        return std::apply(
            [](Patterns const&... patterns)
            {
                return (::resetId(patterns), ...);
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
    Pred const mPred;
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
    static void resetId(When<Pred> const& whenPat)
    {
    }
};

template <typename Unary, typename Pattern>
class App
{
public:
    App(Unary const& unary, Pattern const& pattern)
        : mUnary{unary}
        , mPattern{pattern}
        {}
    auto const& unary() const
    {
        return mUnary;
    }
    auto const& pattern() const
    {
        return mPattern;
    }
private:
    Unary const& mUnary;
    Pattern const& mPattern;
};

template <typename Unary, typename Pattern>
auto app(Unary const& unary, Pattern const& pattern)
{
    return App<Unary, Pattern>{unary, pattern};
}

template <typename Unary, typename Pattern>
class PatternTraits<App<Unary, Pattern>>
{
public:
    template <typename Value>
    static bool match(App<Unary, Pattern> const& appPat, Value const& value)
    {
        return ::match(appPat.pattern(), appPat.unary()(value));
    }
    static void resetId(App<Unary, Pattern> const& appPat)
    {
        return ::resetId(appPat.pattern());
    }
};

template <typename T>
auto operator<(WildCard const&, T const& t)
{
    return when([t](auto&& p){ return p < t;});
}

template <typename T>
auto operator<=(WildCard const&, T const& t)
{
    return when([t](auto&& p){return p <= t;});
}

template <typename T>
auto operator>=(WildCard const&, T const& t)
{
    return when([t](auto&& p){return p >= t;});
}

template <typename T>
auto operator>(WildCard const&, T const& t)
{
    return when([t](auto&& p){return p > t;});
}

template <typename... Patterns>
class And
{
public:
    explicit And(Patterns const&... patterns)
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
auto and_(Patterns const&... patterns)
{
    return And<Patterns...>{patterns...};
}

template <typename... Patterns>
class PatternTraits<And<Patterns...>>
{
public:
    template <typename Value>
    static bool match(And<Patterns...> const& andPat, Value const& value)
    {
        return std::apply(
            [&value](Patterns const&... patterns)
            {
                return (::match(patterns, value) && ...);
            },
            andPat.patterns());
    }
    static void resetId(And<Patterns...> const& andPat)
    {
        return std::apply(
            [](Patterns const&... patterns)
            {
                return (::resetId(patterns), ...);
            },
            andPat.patterns());
    }
};

template <typename Type>
class Id
{
public:
    Id() = default;
    template <typename Value>
    bool match(Value const& value) const
    {
        if (mHasValue)
        {
            return mValue == value;
        }
        mHasValue = true;
        mValue = value;
        return true;
    }
    void reset() const
    {
        mHasValue = false;
    }
    Type& value() const
    {
        assert(mHasValue);
        return mValue;
    }
    Type& operator* () const
    {
        return value();
    }
private:
    mutable Type mValue;
    mutable bool mHasValue{false};
};

template <typename Type>
class PatternTraits<Id<Type>>
{
public:
    template <typename Value>
    static bool match(Id<Type> const& idPat, Value const& value)
    {
        return idPat.match(value);
    }
    static void resetId(Id<Type> const& idPat)
    {
        idPat.reset();
    }
};

#endif // _PATTERNS_H_
