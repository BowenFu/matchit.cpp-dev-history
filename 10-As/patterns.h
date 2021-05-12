#ifndef _PATTERNS_H_
#define _PATTERNS_H_

#if 1
#define REQUIRES(x) if (!(x)){throw std::runtime_error("##x");}
#else
#define REQUIRES(x)
#endif

#include <memory>
#include <iostream>

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
        resetId(mPattern);
        return ::match(mPattern, value);
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
    std::tuple<Patterns...> mPatterns;
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
    Unary const mUnary;
    Pattern const mPattern;
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
        return ::match(appPat.pattern(), std::invoke(appPat.unary(), value));
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
    std::tuple<Patterns...> mPatterns;
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

template <typename... T>
class Debug;

template <typename Type>
class Id
{
public:
    Id(bool own = false)
    : mOwn{true}
    {}
    template <typename Value>
    bool match(Value const& value) const
    {
        if (*mValue)
        {
            return **mValue == value;
        }
        // Debug<decltype(mValue)> x;
        if (mOwn)
        {
            *mValue = std::make_shared<Value>(value);
        }
        else
        {
            (*mValue).reset(&value, [](auto&&){});
        }
        return true;
    }
    void reset() const
    {
        (*mValue).reset();
    }
    Type const& value() const
    {
        return **mValue;
    }
private:
    bool const mOwn;
    mutable std::shared_ptr<std::shared_ptr<Type const>> mValue = std::make_shared<std::shared_ptr<Type const>>();
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

template <typename... Patterns>
class Ds
{
public:
    explicit Ds(Patterns const&... patterns)
        : mPatterns{patterns...}
        {}
    auto const& patterns() const
    {
        return mPatterns;
    }
private:
    std::tuple<Patterns...> mPatterns;
};

template <typename... Patterns>
auto ds(Patterns const&... patterns) -> Ds<Patterns...>
{
    return Ds<Patterns...>{patterns...};
}

template <typename... Patterns>
class PatternTraits<Ds<Patterns...>>
{
public:
    template <typename... Values>
    static bool match(Ds<Patterns...> const& dsPat, std::tuple<Values...> const& valueTuple)
    {
        return std::apply(
            [&valueTuple](Patterns const&... patterns)
            {
                return std::apply(
                    [&patterns...](Values const&... values)
                    {
                        if constexpr(sizeof...(patterns) != sizeof...(values))
                        {
                            return false;
                        }
                        else
                        {
                            return (::match(patterns, values) && ...);
                        }
                    },
                    valueTuple);
            },
            dsPat.patterns());
    }
    static void resetId(Ds<Patterns...> const& dsPat)
    {
        return std::apply(
            [](Patterns const&... patterns)
            {
                return (::resetId(patterns), ...);
            },
            dsPat.patterns());
    }
};

#endif // _PATTERNS_H_