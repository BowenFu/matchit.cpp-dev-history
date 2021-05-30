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

template <typename Value, typename Pattern>
bool matchPattern(Value const& value, Pattern const& pattern)
{
    return PatternTraits<Pattern>::matchPatternImpl(value, pattern);
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
    bool matchValue(Value const& value) const
    {
        resetId(mPattern);
        return ::matchPattern(value, mPattern);
    }
    auto execute() const
    {
        return mHandler();
    }
private:
    Pattern const& mPattern;
    Func const& mHandler;
};

template <typename Pattern, typename Pred>
class PostCheck;

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
    template <typename Pred>
    auto when(Pred const& pred)
    {
        return PatternHelper<PostCheck<Pattern, Pred>>(PostCheck(mPattern, pred));
    }
private:
    Pattern const mPattern;
};

template <typename Pattern>
PatternHelper<Pattern> pattern(Pattern const& p)
{
    return PatternHelper<Pattern>{p};
}

template <typename... Patterns>
class Ds;
template <typename... Patterns>
auto ds(Patterns const&... patterns) -> Ds<Patterns...>;

template <typename First, typename... Patterns>
auto pattern(First const& f, Patterns const&... ps)
{
    return PatternHelper<Ds<First, Patterns...>>{ds(f, ps...)};
}

template <typename Pattern>
class PatternTraits
{
public:
    template <typename Value>
    static bool matchPatternImpl(Value const& value, Pattern const& pattern)
    {
        return pattern == value;
    }
    static void resetId(Pattern const&)
    {
    }
};

class WildCard{};
constexpr WildCard _;

template <>
class PatternTraits<WildCard>
{
    using Pattern = WildCard;
public:
    template <typename Value>
    static bool matchPatternImpl(Value const&, Pattern const&)
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
    static bool matchPatternImpl(Value const& value, Or<Patterns...> const& orPat)
    {
        return std::apply(
            [&value](Patterns const&... patterns)
            {
                return (::matchPattern(value, patterns) || ...);
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
class Meet
{
public:
    explicit Meet(Pred const& pred)
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
auto meet(Pred const& pred)
{
    return Meet<Pred>{pred};
}

template <typename Pred>
class PatternTraits<Meet<Pred>>
{
public:
    template <typename Value>
    static bool matchPatternImpl(Value const& value, Meet<Pred> const& meetPat)
    {
        return meetPat.predicate()(value);
    }
    static void resetId(Meet<Pred> const& meetPat)
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
    static bool matchPatternImpl(Value const& value, App<Unary, Pattern> const& appPat)
    {
        return ::matchPattern(std::invoke(appPat.unary(), value), appPat.pattern());
    }
    static void resetId(App<Unary, Pattern> const& appPat)
    {
        return ::resetId(appPat.pattern());
    }
};

template <typename T>
auto operator<(WildCard const&, T const& t)
{
    return meet([t](auto&& p){ return p < t;});
}

template <typename T>
auto operator<=(WildCard const&, T const& t)
{
    return meet([t](auto&& p){return p <= t;});
}

template <typename T>
auto operator>=(WildCard const&, T const& t)
{
    return meet([t](auto&& p){return p >= t;});
}

template <typename T>
auto operator>(WildCard const&, T const& t)
{
    return meet([t](auto&& p){return p > t;});
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
    static bool matchPatternImpl(Value const& value, And<Patterns...> const& andPat)
    {
        return std::apply(
            [&value](Patterns const&... patterns)
            {
                return (::matchPattern(value, patterns) && ...);
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

template <typename Pattern>
class Not
{
public:
    explicit Not(Pattern const& pattern)
        : mPattern{pattern}
        {}
    auto const& pattern() const
    {
        return mPattern;
    }
private:
    Pattern mPattern;
};

template <typename Pattern>
auto not_(Pattern const& pattern)
{
    return Not<Pattern>{pattern};
}

template <typename Pattern>
class PatternTraits<Not<Pattern>>
{
public:
    template <typename Value>
    static bool matchPatternImpl(Value const& value, Not<Pattern> const& notPat)
    {
        return !::matchPattern(value, notPat.pattern());
    }
    static void resetId(Not<Pattern> const& notPat)
    {
        ::resetId(notPat.pattern());
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
    bool matchValue(Value const& value) const
    {
        if (*mValue)
        {
            return **mValue == value;
        }
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
    Type const& operator*() const
    {
        return value();
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
    static bool matchPatternImpl(Value const& value, Id<Type> const& idPat)
    {
        return idPat.matchValue(value);
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

namespace impl
{
    // std::apply implementation from cppreference, except that std::get -> get to allow for ADL
    namespace detail
    {
        template <class F, class Tuple, std::size_t... I>
        constexpr decltype(auto) apply_impl(F &&f, Tuple &&t, std::index_sequence<I...>)
        {
            // This implementation is valid since C++20 (via P1065R2)
            // In C++17, a constexpr counterpart of std::invoke is actually needed here
            using std::get;
            return std::invoke(std::forward<F>(f), get<I>(std::forward<Tuple>(t))...);
        }
    } // namespace detail

    template <class F, class Tuple>
    constexpr decltype(auto) apply(F &&f, Tuple &&t)
    {
        return detail::apply_impl(
            std::forward<F>(f), std::forward<Tuple>(t),
            std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple> > >{});
    }
}

template <typename... Patterns>
class PatternTraits<Ds<Patterns...>>
{
public:
    template <typename Tuple, typename... Values>
    static bool matchPatternImpl(Tuple const& valueTuple, Ds<Patterns...> const& dsPat)
    {
        return std::apply(
            [&valueTuple](Patterns const&... patterns)
            {
                return impl::apply(
                    [&patterns...](auto const&... values)
                    {
                        return (::matchPattern(values, patterns) && ...);
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

template <typename Pattern, typename Pred>
class PostCheck
{
public:
    explicit PostCheck(Pattern const &pattern, Pred const &pred)
        : mPattern{pattern}
        , mPred{pred}
    {}
    bool check() const
    {
        return mPred();
    }
    auto const& pattern() const
    {
        return mPattern;
    }
private:
    Pattern const mPattern;
    Pred const mPred;
};

template <typename Pattern, typename Pred>
class PatternTraits<PostCheck<Pattern, Pred>>
{
public:
    template <typename Value>
    static bool matchPatternImpl(Value const& value, PostCheck<Pattern, Pred> const& postCheck)
    {
        return ::matchPattern(value, postCheck.pattern()) && postCheck.check();
    }
    static void resetId(PostCheck<Pattern, Pred> const& postCheck)
    {
        ::resetId(postCheck.pattern());
    }
};

#endif // _PATTERNS_H_
