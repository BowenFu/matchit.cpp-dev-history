There is a common practice for catch-all case in pattern matching.
Using a placeholder `_` to match all missing cases.

Then we can write our test function as 
```C++
auto x = match(input)(
    pattern(1) = func1,
    pattern(2) = func2,
    pattern(_) = someFunc
);
```

What is `_`? Can it be some magic integral number, such as INT_MAX? Maybe not. If that happens, what if we want match `INT_MAX` as following?
```C++
auto x = match(input)(
    pattern(1) = func1,
    pattern(2) = func2,
    pattern(INT_MAX) = func3,
    pattern(_) = someFunc
);
```

That means `_` should be of a special unique type, not be confused with all other types.
Let's define a type for it.
```C++
class WildCard{};
constexpr WildCard _;
```

What does this mean? `MatchHelper::operator()`'s parameter pack is of multiple types. We can no longer store them in a container, unless we introduce type erasure. But that would have a bad impact on performance. We have to turn to `std::tuple`.

How do you loop over `variadic parameters` or a `std::tuple`? Template specialization, SFINAE or fold expression introduced in C++17.
Using fold expression we can reimplement `MatchHelper::operator()` as 
```C++
template <typename... PatternPair>
auto operator()(PatternPair const&... patterns)
{
    bool result{}; // Should be RetType result{};
    auto const func = [this, &result](auto const& pattern) -> bool
    {
        if (pattern.match(mValue))
        {
            result = pattern.execute(mValue);
            return true;
        }
        return false;
    };
    bool const matched = (func(patterns) || ...);
    assert(matched);
    return result;
}
```
We use `||` or sematics to find the first match, instead of the last match or best match.

Here we simply use `bool` as the result type as we only test that for now.
Let's compile our codes. Oops, there are some errors.
```C++
In file included from main.cpp:1:
./core.h:52:25: error: invalid operands to binary expression ('const WildCard' and 'const int')
        return mPattern == value;
               ~~~~~~~~ ^  ~~~~~
./core.h:20:25: note: in instantiation of function template specialization 'PatternPair<WildCard, bool (int)>::match<int>' requested here
            if (pattern.match(mValue))
```
The trival `mPattern == value;` no longer works for the newly defined `WildCard` type.
Let's refactor this logic out of PatternPair and we can define a `PatternTraits`.
Now the original `PatternPair::match` becomes
```C++
template <typename Value>
bool match(Value const& value) const
{
    return PatternTraits<Pattern>::match(mPattern, value);
}
```
We define a default `PatternTraits` for all ordinary patterns.
```C++
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
```
And specialize that for our newly defined `WildCard`.
```C++
template <>
class PatternTraits<WildCard>
{
public:
    using Pattern = WildCard;
    template <typename Value>
    static bool match(Pattern const&, Value const&)
    {
        return true;
    }
};
```

Now
```C++
auto x = match(input)(
    pattern(1) = func1,
    pattern(2) = func2,
    pattern(_) = func1
);
```
combined with 
```C++
int32_t main()
{
    testMatch(1, true);
    testMatch(2, false);
    testMatch(55, true); // _ matched.
    return 0;
}
```
prints out
```
Passed!
Passed!
Passed!
```

Congratulations! You have implement the WildCard `_` pattern, similar to `default clause` of `switch case`.

Wait, we hard-coded our return type as `bool` in our codes. That is not acceptable. We can have handlers that return values of any types.
We need to extract the return types from handlers, i.e., `Func` of `PatternPair`.
We can choose to decode the template types of `PatternPair` via a template class, or let `PatternPair` provide an API to query the type directly. I prefer the latter since that does not reply on the implementation of `PatternPair`.

For multiple `PatternPair`, we need to check all their return types are of the same.
Alternatively, we need to call `std::common_type_t` to get their common types.
This can be implemented as 
```C++
template <typename Value, typename... PatternPair>
class PatternPairsRetType
{
public:
    using RetType = std::common_type_t<typename PatternPair::template RetType<Value>...>;
};
```
And the original `int32_t result;` becomes
```C++
using RetType = typename PatternPairsRetType<Value, PatternPair...>::RetType;
RetType result{};
```
Let's change func2's return type from bool to int64_t as
```C++
int64_t func2(int32_t v)
{
    return 12;
}
```

We can assert that the result type of the matching call is `int64_t` in testMatch
```C++
static_assert(std::is_same_v<int64_t, decltype(x)>);
```
.

Congratulation! Now you implement your pattern matching library with full feature of `switch case`.

Since we already support different types of pattern pairs, we can have not only differnt pattern types, but also different handler types. We can use lambda expressions now, not just function pointers or `std::function`.

Let's change the `match` test to
```C++
auto x = match(input)(
    pattern(1) = func1,
    pattern(2) = func2,
    pattern(_) = [](int32_t){ return 111; }
);
```
Good job!

Note that at the end of `MatchHelper::operator()` we assert that the match succeeds. Since it is not easy for us to detect the exhaustiveness of our patterns at compile time, we report errors at runtime. If we always put `pattern(_)` at the end of every matches, that assertion would never fail.