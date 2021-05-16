#ifndef _PATTERNS_H_
#define _PATTERNS_H_

#if 1
#define REQUIRES(x)                      \
    if (!(x))                            \
    {                                    \
        throw std::runtime_error("##x"); \
    }
#else
#define REQUIRES(x)
#endif

#include <memory>
#include <iostream>

template <typename Pattern>
class PatternTraits;

template <typename Value, typename Pattern>
auto matchPattern(Value const &value, Pattern const &pattern) -> decltype(PatternTraits<Pattern>::matchPatternImpl(value, pattern))
{
    return PatternTraits<Pattern>::matchPatternImpl(value, pattern);
}

template <typename Pattern>
void resetId(Pattern const &pattern)
{
    PatternTraits<Pattern>::resetId(pattern);
}

template <typename Pattern, typename Func>
class PatternPair
{
public:
    using RetType = std::invoke_result_t<Func>;

    PatternPair(Pattern const &pattern, Func const &func)
        : mPattern{pattern}, mHandler{func}
    {
    }
    template <typename Value>
    bool matchValue(Value const &value) const
    {
        resetId(mPattern);
        return ::matchPattern(value, mPattern);
    }
    auto execute() const
    {
        return mHandler();
    }

private:
    Pattern const &mPattern;
    Func const &mHandler;
};

template <typename Pattern, typename Pred>
class PostCheck;

template <typename Pattern>
class PatternHelper
{
public:
    explicit PatternHelper(Pattern const &pattern)
        : mPattern{pattern}
    {
    }
    template <typename Func>
    auto operator=(Func const &func)
    {
        return PatternPair<Pattern, Func>{mPattern, func};
    }
    template <typename Pred>
    auto when(Pred const &pred)
    {
        return PatternHelper<PostCheck<Pattern, Pred> >(PostCheck(mPattern, pred));
    }

private:
    Pattern const mPattern;
};

template <typename Pattern>
PatternHelper<Pattern> pattern(Pattern const &p)
{
    return PatternHelper<Pattern>{p};
}

template <typename... Patterns>
class Ds;
template <typename... Patterns>
auto ds(Patterns const &...patterns) -> Ds<Patterns...>;

template <typename First, typename... Patterns>
auto pattern(First const &f, Patterns const &...ps)
{
    return PatternHelper<Ds<First, Patterns...> >{ds(f, ps...)};
}

class WildCard
{
};
constexpr WildCard _;

template <typename Pattern>
class PatternTraits
{
public:
    template <typename Value>
    static auto matchPatternImpl(Value const &value, Pattern const &pattern) -> decltype(pattern == value)
    {
        return pattern == value;
    }
    static void resetId(Pattern const &)
    {
    }
};

template <>
class PatternTraits<WildCard>
{
    using Pattern = WildCard;

public:
    template <typename Value>
    static bool matchPatternImpl(Value const &, Pattern const &)
    {
        return true;
    }
    static void resetId(Pattern const &)
    {
    }
};

template <typename... Patterns>
class Or
{
public:
    explicit Or(Patterns const &...patterns)
        : mPatterns{patterns...}
    {
    }
    auto const &patterns() const
    {
        return mPatterns;
    }

private:
    std::tuple<Patterns...> mPatterns;
};

template <typename... Patterns>
auto or_(Patterns const &...patterns) -> Or<Patterns...>
{
    return Or<Patterns...>{patterns...};
}

template <typename... Patterns>
class PatternTraits<Or<Patterns...> >
{
public:
    template <typename Value>
    static auto matchPatternImpl(Value const &value, Or<Patterns...> const &orPat) -> decltype((::matchPattern(value, std::declval<Patterns>()) || ...))
    {
        return std::apply(
            [&value](Patterns const &...patterns) {
                return (::matchPattern(value, patterns) || ...);
            },
            orPat.patterns());
    }
    static void resetId(Or<Patterns...> const &orPat)
    {
        return std::apply(
            [](Patterns const &...patterns) {
                return (::resetId(patterns), ...);
            },
            orPat.patterns());
    }
};

template <typename Pred>
class Meet
{
public:
    explicit Meet(Pred const &pred)
        : mPred{pred}
    {
    }
    auto const &predicate() const
    {
        return mPred;
    }

private:
    Pred const mPred;
};

template <typename Pred>
auto meet(Pred const &pred)
{
    return Meet<Pred>{pred};
}

template <typename Pred>
class PatternTraits<Meet<Pred> >
{
public:
    template <typename Value>
    static auto matchPatternImpl(Value const &value, Meet<Pred> const &meetPat) -> decltype(meetPat.predicate()(value))
    {
        return meetPat.predicate()(value);
    }
    static void resetId(Meet<Pred> const &meetPat)
    {
    }
};

template <typename Unary, typename Pattern>
class App
{
public:
    App(Unary const &unary, Pattern const &pattern)
        : mUnary{unary}, mPattern{pattern}
    {
    }
    auto const &unary() const
    {
        return mUnary;
    }
    auto const &pattern() const
    {
        return mPattern;
    }

private:
    Unary const mUnary;
    Pattern const mPattern;
};

template <typename Unary, typename Pattern>
auto app(Unary const &unary, Pattern const &pattern)
{
    return App<Unary, Pattern>{unary, pattern};
}

template <typename Unary, typename Pattern>
class PatternTraits<App<Unary, Pattern> >
{
public:
    template <typename Value>
    static auto matchPatternImpl(Value const &value, App<Unary, Pattern> const &appPat) -> decltype(::matchPattern(std::invoke(appPat.unary(), value), appPat.pattern()))
    {
        return ::matchPattern(std::invoke(appPat.unary(), value), appPat.pattern());
    }
    static void resetId(App<Unary, Pattern> const &appPat)
    {
        return ::resetId(appPat.pattern());
    }
};

template <typename T>
auto operator<(WildCard const &, T const &t)
{
    return meet([t](auto &&p) { return p < t; });
}

template <typename T>
auto operator<=(WildCard const &, T const &t)
{
    return meet([t](auto &&p) { return p <= t; });
}

template <typename T>
auto operator>=(WildCard const &, T const &t)
{
    return meet([t](auto &&p) { return p >= t; });
}

template <typename T>
auto operator>(WildCard const &, T const &t)
{
    return meet([t](auto &&p) { return p > t; });
}

template <typename... Patterns>
class And
{
public:
    explicit And(Patterns const &...patterns)
        : mPatterns{patterns...}
    {
    }
    auto const &patterns() const
    {
        return mPatterns;
    }

private:
    std::tuple<Patterns...> mPatterns;
};

template <typename... Patterns>
auto and_(Patterns const &...patterns)
{
    return And<Patterns...>{patterns...};
}

template <typename... Patterns>
class PatternTraits<And<Patterns...> >
{
public:
    template <typename Value>
    static auto matchPatternImpl(Value const &value, And<Patterns...> const &andPat) -> decltype((::matchPattern(value, std::declval<Patterns>()) && ...))
    {
        return std::apply(
            [&value](Patterns const &...patterns) {
                return (::matchPattern(value, patterns) && ...);
            },
            andPat.patterns());
    }
    static void resetId(And<Patterns...> const &andPat)
    {
        return std::apply(
            [](Patterns const &...patterns) {
                return (::resetId(patterns), ...);
            },
            andPat.patterns());
    }
};

template <typename Pattern>
class Not
{
public:
    explicit Not(Pattern const &pattern)
        : mPattern{pattern}
    {
    }
    auto const &pattern() const
    {
        return mPattern;
    }

private:
    Pattern mPattern;
};

template <typename Pattern>
auto not_(Pattern const &pattern)
{
    return Not<Pattern>{pattern};
}

template <typename Pattern>
class PatternTraits<Not<Pattern> >
{
public:
    template <typename Value>
    static auto matchPatternImpl(Value const &value, Not<Pattern> const &notPat) -> decltype(!::matchPattern(value, notPat.pattern()))
    {
        return !::matchPattern(value, notPat.pattern());
    }
    static void resetId(Not<Pattern> const &notPat)
    {
        ::resetId(notPat.pattern());
    }
};

template <typename... T>
class Debug;

template <bool own>
class IdTrait;

template <>
class IdTrait<true>
{
public:
    template <typename Ptr, typename Value>
    static auto matchValueImpl(Ptr &ptr, Value const &value) -> decltype(ptr = std::make_unique<Value>(value), void())
    {
        ptr = std::make_unique<Value>(value);
    }
};

template <>
class IdTrait<false>
{
public:
    template <typename Ptr, typename Value>
    static auto matchValueImpl(Ptr &ptr, Value const &value) -> decltype(ptr.reset(&value), void())
    {
        ptr.reset(&value);
    }
};

template <typename Type, bool own = true>
class Id
{
    class NoDelete
    {
    public:
        void operator()(Type const *) {}
    };
    using PtrT = std::conditional_t<own, std::unique_ptr<Type const>, std::unique_ptr<Type const, NoDelete> >;
    mutable std::shared_ptr<PtrT> mValue = std::make_shared<PtrT>();

public:
    template <typename Value>
    auto matchValue(Value const &value) const -> decltype(**mValue == value, IdTrait<own>::matchValueImpl(*mValue, value), bool{})
    {
        if (*mValue)
        {
            return **mValue == value;
        }
        IdTrait<own>::matchValueImpl(*mValue, value);
        return true;
    }
    void reset() const
    {
        (*mValue).reset();
    }
    Type const &value() const
    {
        return **mValue;
    }
    Type const &operator*() const
    {
        return value();
    }

private:
    template <typename P, typename Value>
    auto matchValueImpl(P const &p, Value const &value) const;
};

template <typename Type>
using RefId = Id<Type, false>;

template <typename Type, bool own>
class PatternTraits<Id<Type, own> >
{
public:
    template <typename Value>
    static auto matchPatternImpl(Value const &value, Id<Type, own> const &idPat) -> decltype(idPat.matchValue(value))
    {
        return idPat.matchValue(value);
    }
    static void resetId(Id<Type, own> const &idPat)
    {
        idPat.reset();
    }
};

template <typename... Patterns>
class Ds
{
public:
    explicit Ds(Patterns const &...patterns)
        : mPatterns{patterns...}
    {
    }
    auto const &patterns() const
    {
        return mPatterns;
    }

private:
    std::tuple<Patterns...> mPatterns;
};

template <typename... Patterns>
auto ds(Patterns const &...patterns) -> Ds<Patterns...>
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

template <typename Pattern>
class Ooo;

template <typename Pattern>
class IsOoo : public std::false_type
{
};

template <typename Pattern>
class IsOoo<Ooo<Pattern> > : public std::true_type
{
};

template <typename Pattern>
inline constexpr bool isOooV = IsOoo<std::decay_t<Pattern> >::value;

static_assert(isOooV<Ooo<int> > == true);
static_assert(isOooV<Ooo<int &> > == true);
static_assert(isOooV<Ooo<int const &> > == true);
static_assert(isOooV<Ooo<int &&> > == true);
static_assert(isOooV<int> == false);
static_assert(isOooV<const Ooo<WildCard> &> == true);

template <std::size_t N, typename Tuple, std::size_t... I>
auto dropImpl(Tuple &&t, std::index_sequence<I...>)
{
    using std::get;
    return std::forward_as_tuple(get<I + N>(std::forward<Tuple>(t))...);
}

template <std::size_t N, typename Tuple>
auto drop(Tuple &&t)
{
    return dropImpl<N>(
        std::forward<Tuple>(t),
        std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple> > - N>{});
}

template <typename Value, typename Pattern, typename = std::void_t<> >
struct MatchFuncDefined : std::false_type
{
};

template <typename Value, typename Pattern>
struct MatchFuncDefined<Value, Pattern, std::void_t<decltype(matchPattern(std::declval<Value>(), std::declval<Pattern>()))> >
    : std::true_type
{
};

// template<typename... Args>
template <typename Value, typename Pattern>
inline constexpr bool MatchFuncDefinedV = MatchFuncDefined<Value, Pattern>::value;

static_assert(!MatchFuncDefinedV<char, std::string>);
static_assert(MatchFuncDefinedV<std::string, std::string>);
static_assert(!MatchFuncDefinedV<std::size_t, std::string>);

template <typename... Values, typename... Patterns>
static bool tryOooMatch(std::tuple<Values...> const &values, std::tuple<Patterns...> const &patterns)
{
    // std::cout << "tryOooMatch " << sizeof...(Values) << ", " << sizeof...(Patterns) << std::endl;
    if constexpr (sizeof...(Patterns) == 0)
    {
        return sizeof...(Values) == 0;
    }
    else if constexpr (isOooV<std::tuple_element_t<0, std::tuple<Patterns...> > >)
    {
        auto index = std::make_index_sequence<sizeof...(Values) + 1>{};
        return tryOooMatchImpl(values, patterns, index);
    }
    else if constexpr (sizeof...(Values) >= 1)
    {
        if constexpr (MatchFuncDefinedV<std::tuple_element_t<0, std::tuple<Values...> >, std::tuple_element_t<0, std::tuple<Patterns...> > >)
        {
            return ::matchPattern(std::get<0>(values), std::get<0>(patterns)) && tryOooMatch(drop<1>(values), drop<1>(patterns));
        }
    }
    return false;
}

template <typename... Values, typename... Patterns, std::size_t... I>
static bool tryOooMatchImpl(std::tuple<Values...> const &values, std::tuple<Patterns...> const &patterns, std::index_sequence<I...>)
{
    using std::get;
    return (tryOooMatch(drop<I>(values), drop<1>(patterns)) || ...);
}

template <typename... Values, typename... Patterns>
static bool tupleMatchImpl(std::tuple<Values...> const &values, std::tuple<Patterns...> const &patterns)
{
    // std::cout << "tupleMatchImpl " << sizeof...(Values) << ", " << sizeof...(Patterns) << std::endl;
    constexpr bool KALLOW_TYPE_MISMATCH_FOR_NON_OOO = false;
    if constexpr (KALLOW_TYPE_MISMATCH_FOR_NON_OOO)
    {
        return tryOooMatch(values, patterns);
    }
    else if constexpr (true)
    {
        if constexpr (sizeof...(Patterns) == 0)
        {
            return sizeof...(Values) == 0;
        }
        else if constexpr (isOooV<std::tuple_element_t<0, std::tuple<Patterns...> > >)
        {
            return tryOooMatch(values, patterns);
        }
        else if constexpr (sizeof...(Values) >= 1)
        {
            return ::matchPattern(std::get<0>(values), std::get<0>(patterns)) && tupleMatchImpl(drop<1>(values), drop<1>(patterns));
        }
    }
    return false;
}

template <typename... Patterns>
class PatternTraits<Ds<Patterns...> >
{
public:
    template <typename Tuple>
    static bool matchPatternImpl(Tuple const &valueTuple, Ds<Patterns...> const &dsPat)
    {
        return std::apply(
            [&valueTuple](Patterns const &...patterns) {
                return impl::apply(
                    [&patterns...](auto const &...values) {
                        return tupleMatchImpl(std::forward_as_tuple(values...), std::make_tuple(patterns...));
                    },
                    valueTuple);
            },
            dsPat.patterns());
    }
    static void resetId(Ds<Patterns...> const &dsPat)
    {
        return std::apply(
            [](Patterns const &...patterns) {
                return (::resetId(patterns), ...);
            },
            dsPat.patterns());
    }

private:
};

template <typename Pattern>
class Ooo
{
public:
    explicit Ooo(Pattern const &pattern)
        : mPattern{pattern}
    {
    }
    auto const &pattern() const
    {
        return mPattern;
    }

private:
    Pattern mPattern;
};

template <typename Pattern>
auto ooo(Pattern const &pattern)
{
    return Ooo<Pattern>{pattern};
}

template <typename Pattern>
class PatternTraits<Ooo<Pattern> >
{
public:
    template <typename Value>
    static auto matchPatternImpl(Value const &value, Ooo<Pattern> const &oooPat) -> decltype(::matchPattern(value, oooPat.pattern()))
    {
        return ::matchPattern(value, oooPat.pattern());
    }
    static void resetId(Ooo<Pattern> const &oooPat)
    {
        ::resetId(oooPat.pattern());
    }
};

template <typename Pattern, typename Pred>
class PostCheck
{
public:
    explicit PostCheck(Pattern const &pattern, Pred const &pred)
        : mPattern{pattern}, mPred{pred}
    {
    }
    bool check() const
    {
        return mPred();
    }
    auto const &pattern() const
    {
        return mPattern;
    }

private:
    Pattern const mPattern;
    Pred const mPred;
};

template <typename Pattern, typename Pred>
class PatternTraits<PostCheck<Pattern, Pred> >
{
public:
    template <typename Value>
    static auto matchPatternImpl(Value const &value, PostCheck<Pattern, Pred> const &postCheck) -> decltype(::matchPattern(value, postCheck.pattern()) && postCheck.check())
    {
        return ::matchPattern(value, postCheck.pattern()) && postCheck.check();
    }
    static void resetId(PostCheck<Pattern, Pred> const &postCheck)
    {
        ::resetId(postCheck.pattern());
    }
};

#endif // _PATTERNS_H_