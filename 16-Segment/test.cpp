#include <type_traits>
#include <string>

int main()
{
    return 0;
}

template <typename Pattern>
class PatternTraits;

template <typename Value, typename Pattern>
auto matchPattern(Value const &value, Pattern const &pattern)
-> decltype(PatternTraits<Pattern>::matchPatternImpl(value, pattern))
{
    return PatternTraits<Pattern>::matchPatternImpl(value, pattern);
}

template <typename Pattern>
class PatternTraits
{
public:
    template <typename Value>
    static auto matchPatternImpl(Value const &value, Pattern const &pattern)
    -> decltype(pattern == value)
    {
        return pattern == value;
    }
};

template <typename Value, typename Pattern, typename = std::void_t<> >
struct MatchFuncDefined : std::false_type
{
};

template <typename Value, typename Pattern>
struct MatchFuncDefined<Value, Pattern, std::void_t<decltype(matchPattern(std::declval<Value>(), std::declval<Pattern>()))> >
    : std::true_type
{
};

template <typename Value, typename Pattern>
inline constexpr bool MatchFuncDefinedV = MatchFuncDefined<Value, Pattern>::value;

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

template <typename ValuesTuple, typename PatternsTuple, typename Enable = void>
class TupleMatchHelper
{
public:
static auto tupleMatchImpl(ValuesTuple const &values, std::tuple<>)
{
    return false;
}
};

template <std::size_t N, typename Tuple, std::size_t... I>
auto dropImpl(Tuple &&t, std::index_sequence<I...>)
{
    using std::get;
    // Fixme, use std::forward_as_tuple when possible.
    // return std::forward_as_tuple(get<I + N>(std::forward<Tuple>(t))...);
    return std::make_tuple(get<I + N>(std::forward<Tuple>(t))...);
}

template <typename Tuple, std::size_t... I>
auto takeImpl(Tuple &&t, std::index_sequence<I...>)
{
    using std::get;
    return std::forward_as_tuple(get<I>(std::forward<Tuple>(t))...);
    // return std::make_tuple(get<I>(std::forward<Tuple>(t))...);
}

template <std::size_t N, typename Tuple>
auto take(Tuple &&t)
// -> decltype(
//     takeImpl(
//         std::forward<Tuple>(t),
//         std::make_index_sequence<N>{}))
{
    return takeImpl(
        std::forward<Tuple>(t),
        std::make_index_sequence<N>{});
}

template <std::size_t N, typename Tuple>
auto drop(Tuple &&t)
// -> decltype(
//     dropImpl<N>(
//         std::forward<Tuple>(t),
//         std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple> > - N>{}))
{
    return dropImpl<N>(
        std::forward<Tuple>(t),
        std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple> > - N>{});
}


using std::get;
template <typename ValuesTuple, typename PatternsTuple>
class TupleMatchHelper<ValuesTuple, PatternsTuple, std::enable_if_t<std::tuple_size_v<ValuesTuple> >= 1> >
{
public:
static auto tupleMatchImpl(ValuesTuple const &values, PatternsTuple const &patterns)
-> decltype(::matchPattern(get<0>(values), get<0>(patterns)) && TupleMatchHelper<decltype(drop<1>(values)), decltype(drop<1>(patterns))>::tupleMatchImpl(drop<1>(values), drop<1>(patterns)))
{
    return ::matchPattern(get<0>(values), get<0>(patterns)) && TupleMatchHelper<decltype(drop<1>(values)), decltype(drop<1>(patterns))>::tupleMatchImpl(drop<1>(values), drop<1>(patterns));
}
};

template <typename Pattern>
class IsOoo;

template <typename Pattern>
class Ooo;

template <typename Pattern>
inline constexpr bool isOooV = IsOoo<std::decay_t<Pattern> >::value;

template <typename ValuesTuple, typename PatternHead, typename... PatternTail>
class TupleMatchHelper<ValuesTuple, std::tuple<PatternHead, PatternTail...>, std::enable_if_t<isOooV<PatternHead> >>
{
public:
static auto tupleMatchImpl(ValuesTuple const &values, std::tuple<PatternHead, PatternTail...> const &patterns)
-> decltype(tryOooMatch(values, patterns))
{
    return tryOooMatch(values, patterns);
}
};

template <typename ValuesTuple>
class TupleMatchHelper<std::tuple<>, ValuesTuple>
{
public:
static auto tupleMatchImpl(ValuesTuple const &values, std::tuple<>)
-> decltype(std::tuple_size_v<ValuesTuple> == 0)
{
    return std::tuple_size_v<ValuesTuple> == 0;
}
};


template <typename... Patterns>
class PatternTraits<Ds<Patterns...> >
{
public:
    // template <typename Tuple>
    // static auto matchPatternImpl(Tuple const &valueTuple, Ds<Patterns...> const &dsPat)
    template <typename... Values>
    static auto matchPatternImpl(std::tuple<Values...> const &valueTuple, Ds<Patterns...> const &dsPat)
        // -> decltype(tupleMatchImpl(valueTuple, dsPat.patterns()))
    {
        return tupleMatchImpl(valueTuple, dsPat.patterns());
    }

private:
};

template <typename ValuesTuple, typename PatternsTuple>
bool tryOooMatch(ValuesTuple const &values, PatternsTuple const &patterns)
{
    // std::cout << "tryOooMatch " << sizeof...(Values) << ", " << sizeof...(Patterns) << std::endl;
    if constexpr (std::tuple_size_v<PatternsTuple> == 0)
    {
        return std::tuple_size_v<ValuesTuple> == 0;
    }
    else if constexpr (isOooV<std::tuple_element_t<0, PatternsTuple> >)
    {
        auto index = std::make_index_sequence<std::tuple_size_v<ValuesTuple> + 1>{};
        return tryOooMatchImpl(values, patterns, index);
    }
    else if constexpr (std::tuple_size_v<ValuesTuple> >= 1)
    {
        if constexpr (MatchFuncDefinedV<std::tuple_element_t<0, ValuesTuple>, std::tuple_element_t<0, PatternsTuple> >)
        {
            using std::get;
            return ::matchPattern(get<0>(values), get<0>(patterns)) && tryOooMatch(drop<1>(values), drop<1>(patterns));
        }
    }
    return false;
}

class OooMatchBreak : public std::exception
{
};

template < std::size_t I, typename ValuesTuple, typename PatternsTuple>
bool tryOooMatchImplHelper(ValuesTuple const &values, PatternsTuple const &patterns)
{
    using std::get;
    // FIXME: cache matchPattern for ooo
    if constexpr (I == 0)
    {
        return (tryOooMatch(values, drop<1>(patterns)));
    }
    else if constexpr (I > 0)
    {
        if constexpr (MatchFuncDefinedV<decltype(take<I>(values)), std::tuple_element_t<0, PatternsTuple> >)
        {
            // std::cout << "values num: " << sizeof...(Values) << "\t index: " << I << "\tpatterns num: " << sizeof...(Patterns) << std::endl;
            // return ((::matchPattern(take<I>(values), get<0>(patterns)) && tryOooMatch(drop<I>(values), drop<1>(patterns))));
            if (!PatternTraits<std::tuple_element_t<0, PatternsTuple> >::matchPatternImplSingle(get<I - 1>(values), get<0>(patterns)))
            {
                throw OooMatchBreak();
            }

            if (!tryOooMatch(drop<I>(values), drop<1>(patterns)))
            {
                return false;
            }
            return true;
        }
    }
    throw OooMatchBreak();
}

template <typename ValuesTuple, typename PatternsTuple, std::size_t... I>
bool tryOooMatchImpl(ValuesTuple const &values, PatternsTuple const &patterns, std::index_sequence<I...>)
{
    try
    {
        return ((tryOooMatchImplHelper<I>(values, patterns)) || ...);
    }
    catch (const OooMatchBreak &)
    {
        return false;
    }
}

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
class IsOoo : public std::false_type
{
};

template <typename Pattern>
class IsOoo<Ooo<Pattern> > : public std::true_type
{
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
    template <typename... Values>
    static auto matchPatternImpl(std::tuple<Values...> const &valueTuple, Ooo<Pattern> const &oooPat)
    -> decltype((::matchPattern(std::declval<Values>(), oooPat.pattern()) && ...))
    {
        return std::apply(
            [&oooPat](Values const &...values) {
                auto result = (::matchPattern(values, oooPat.pattern()) && ...);
                // std::cout << "match " << ", ";
                // ((std::cout << ... << values) << "\t");
                // if constexpr(std::is_same_v<decltype(oooPat.pattern()), int>)
                // {
                //     std::cout << ", pat" << oooPat.pattern() << std::endl;
                // }
                // std::cout << "match ooo\t" << result << std::endl;
                return result;
                // return (::matchPattern(values, oooPat.pattern()) && ...);
            },
            valueTuple);
    }
    template <typename Value>
    static auto matchPatternImplSingle(Value const &value, Ooo<Pattern> const &oooPat)
    -> decltype(::matchPattern(value, oooPat.pattern()))
    {
        return ::matchPattern(value, oooPat.pattern());
    }
};

static_assert(!MatchFuncDefinedV<char, std::string>);

static_assert(!MatchFuncDefinedV<std::tuple<int>, int>);
static_assert(!MatchFuncDefinedV<std::tuple<int>, Ds<std::string, int>>);
static_assert(!MatchFuncDefinedV<std::tuple<int>, Ds<std::string, Ds<std::string, std::string>, int>>);
static_assert(!MatchFuncDefinedV<int, Ds<std::string, Ds<std::string, std::string>, int>>);
// static_assert(!MatchFuncDefinedV<int, Ds<std::string, Ds<std::string, Id<std::string, true> >, int>>);
static_assert(!MatchFuncDefinedV<std::tuple<std::string>, Ds<char> >);
static_assert(MatchFuncDefinedV<std::tuple<char, std::tuple<char, char>, int>, Ds<char, Ds<char, char>, int>>);