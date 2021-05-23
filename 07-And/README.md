We already have `or` pattern? What about `and` pattern? Someone may ask.
Let's add it to our pattern matching library.
What is the use case of `and` pattern?
For `or`, we have `or(1,2,3,4)`, can we have `and(1,2,3,4)`?
No, that does not make any sense.
Recall that when implementing `when` pattern we mentioned the case in which we would like to match n in `[1,10]`.
We have 
```C++
auto within(int min, int max)
{
    return [min, max](auto n){return n>=min && n<= max;};
}
match(n)
(
    pattern(when(within(1, 10)) = []{return 'A';},
    pattern(_)                  = []{return 'B';}
);
```
and
```C++
match(n)
(
    pattern(_ < 1)   = []{return 'B';},
    pattern(_ <= 10) = []{return 'A';},
    pattern(_)       = []{return 'B';}
);
```
With `and` pattern, we can then have
```C++
match(n)
(
    pattern(and_(_ >= 1, _ <= 10)) = []{return 'A';},
    pattern(_)                     = []{return 'B';}
);
```
This is a good case of `and` pattern. Note that we cannot use `and` directly since that is a C++ keyword. We use `and_` instead.
The implementation is also simple

```C++
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
};
```
One open question, is it possible for us to write the patterns more naturally like
```C++
match(n)
(
    pattern(_ >= 1 && _ <= 10) = []{return 'A';},
    pattern(_)                 = []{return 'B';}
);
```
?
Hint, you may need to overload the operator `&&` for some type. What is the type? 