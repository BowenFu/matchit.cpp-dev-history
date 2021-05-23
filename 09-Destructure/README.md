What is `destructure` pattern?
The Rust version can be
```Rust
match balloon.location {
    Point{x: 0, y: height}
        => println!("straight up {} meters", height),
    Point{x: x, y: y}
        =>println!("at ({}m, {}m)", x, y)
}
```
(We choose not to introduce Racket samples in this section since Racket is of dynamic type and its samples for destructure can be misleading here.)
Can we implement similar functionality using existent patterns?
Sure, our `app` pattern is powerful enough.
Suppose we have a Location class and two functions to get its members.
```C++
struct Location
{
    int x;
    int y;
};
auto x = [](Location& const l){return l.x;};
auto y = [](Location& const l){return l.y;};
```
Then we can match a Location object like
```C++
Id<int> xi;
Id<int> yi;
match(l)
(
    pattern(and_(app(x, 0), app(y, yi)))
        = [&yi]{return "straight up " + std::to_string(*yi) + " meters";},
    pattern(and_(app(x, xi), app(y, yi)))
        = [&xi, &yi]{return "at (" + std::to_string(*xi) + "m, " + std::to_string(*yi) + "m)";}
);
```
Can we bind the two identifiers in a single function?
You may want to write something like this.
```C++
auto ds = [](Id<int>& i, Id<int>& j)
{
    return [](Location& const l)
    {
        return i.match(l.x) && j.match(l.y);
    };
};
match(l)
(
    pattern(and_(app(x, 0), app(y, yi)))
        = [&yi]{return "straight up " + std::to_string(*yi) + " meters";},
    pattern(when(ds(xi, yi)))
        = [&xi, &yi]{return "at (" + std::to_string(*xi) + "m, " + std::to_string(*yi) + "m)";}
);
```
Quite cool, but this is not the correct way since we can no longer reset these identifiers used to generate a predicate.
And users are not expected to call patterns' methods directly. They should know nothing about them.
We need to make these methods only visible to some friend classes for finer access control later.

Then what is the recommended way? Compose patterns!

```C++
Id<int> xi;
Id<int> yi;
auto ds = [](auto&& xPat, auto&& yPat)
{
    return and_(app(x, xPat), app(y, yPat));
};
match(l)
(
    pattern(ds(0, yi))
        = [&yi]{return "straight up " + std::to_string(*yi) + " meters";},
    pattern(ds(xi, yi))
        = [&xi, &yi]{return "at (" + std::to_string(*xi) + "m, " + std::to_string(*yi) + "m)";}
);
```
Good job! The new version of `ds` can accept any form of x patterns and y patterns.
If we use `std::invoke` instead of calling `f()` for `app` patterns we can save the two extracter functions and write the `ds` as
```C++
auto ds = [](auto&& xPat, auto&& yPat)
{
    return and_(app(Location::x, xPat), app(Location::y, yPat));
};
```
How about destructuring a tuple?
```C++
template <typename Tuple, typename PatternTuple, std::size_t... I>
auto dsImpl(PatternTuple &&patterns, std::index_sequence<I...>)
{
    using std::get;
    return and_(app(get<I, PatternTuple>, get<I>(patterns)), ...);
}

template <typename Tuple, typename... Patterns>
auto ds(Patterns... &&patterns)
{
    return dsImpl<Tuple>(
        std::forward_as_tuple(patterns),
        std::make_index_sequence<sizeof...(patterns)>{});
}
```
(Note that the app patterns are as many as provided, we do not check the length against the size of the tuple to be destructured. That is possible via adding a `static_assert` to the ds function.)
Now lets test a tuple destructuring as 
```C++
Id<int> xi;
Id<int> yi;
std::cout <<
match(std::make_tuple(1,2))
(
    pattern(ds(xi, yi))
        = [&xi, &yi]{return "at (" + std::to_string(*xi) + "m, " + std::to_string(*yi) + "m)";}
);
```
We get the output `at (1m, 2m)`.

Seems that we can implement the `destructure` pattern via composing `and` and `app` patterns.
But we still decide to implement this pattern as a base pattern in favor of more advanced features in later sections.
The implemetation is trival as
```C++
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
    std::tuple<Patterns const&...> mPatterns;
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
                        return (::match(patterns, values) && ...);
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
```

It is a common use case to match multiple values at the same time.
Users need to write codes as
```C++
match(std::make_tuple(x, y, z))
(
    ...
);
```
Let's make it shorter as
```C++
match(x, y, z)
(
    ...
);
```
This requires us to define a variadic version of `match` function.
This is it.
```C++
template <typename First, typename... Values>
auto match(First const& first, Values const&... values)
{
    std::tuple<First, Values...> const x = std::make_tuple(first, values...);
    return MatchHelper<decltype(x)>{x};
}
```
Now comes the end of this section.