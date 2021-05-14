# First Try

In this section, we will focus on implementing fundamental pattern matching functionality. How do you define "fundamental pattern matching functionality"?

For me, covering the usages of `switch case`.

Let's begin.

C++ `switch case` handles integral types (including `char`) and enums.
Then for any integral types, we can have
```C++
auto result = match (1)
(
    pattern(1) = []{std::cout << "1 matched" << std::endl;},
    pattern(2) = []{std::cout << "2 matched" << std::endl;}
);
```

Let's implement our core.h first.
As mentioned in last section, we will have the two functions `match` and `pattern`.

```C++
template <typename Value>
MatchHelper<Value> match(Value const& value)
{
    return MatchHelper<Value>{value};
}

template <typename Pattern>
PatternHelper<Pattern> pattern(Pattern const& p)
{
    return PatternHelper<Pattern>{p};
}
```

As we expect, `MatchHelper` will have a `operator()` and `PatternHelper` will have a `operator=`.
Their implementations can be
```C++
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
        // match the value against the patterns and execution corresponding functions.
    }
private:
    Value const& mValue;
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
```
In `PatternHelper::operator=` we simply bind patterns and their tasks together via constructing a `PatternPair`.

A basic `PatternPair` would be
```C++
template <typename Pattern, typename Func>
class PatternPair
{
public:
    PatternPair(Pattern const& pattern, Func const& func)
        : mPattern{pattern}
        , mHandler{func}
    {
    }
private:
    Pattern const& mPattern;
    Func const& mHandler;
};
```

`MatchHelper::operator()` is not trival. What do you expect that function to do?
The main work should be done there -- match first pattern and execute its task.
(Not like best match like function overloading or template specialization in C++. First match is a common practice in pattern matching since users can define priority of patterns.)
Sounds like we need to call something like `std::find_if` or loop over all patterns.
Let's do it!
(Here we requires all PatternPairs are of the same type. Otherwise the compilation would fail.)
```C++
template <typename... PatternPair>
auto operator()(PatternPair const&... patterns)
{
    auto const pats = {patterns...};
    auto const iter = std::find_if(pats.begin(), pats.end(), [this](auto const& pat){return pat.match(mValue);});
    assert(iter != pats.end());
    return iter->execute(mValue);
}
```

We used two methods of `PatternPair` in this code snippet, `match` and `execute`. They are simple for integrals.
```C++
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
```

Now we have finished the implementation of a very basic pattern matching library.
Let's test it.

We put our library codes in [core.h](./core.h) and tests in [main.cpp](./main.cpp).

We define a test function as 
```C++
template <typename V, typename U>
void testMatch(V const &input, U const &expected)
{
    auto x = match(input)(
        pattern(1) = func1,
        pattern(2) = func2
    );
    if (x == expected)
    {
        printf("Passed!\n");
    }
    else
    {
        printf("Failed!\n");
    }
}
```
where `func1` and `func2` are defined as 
```C++
bool func1(int32_t v)
{
    return true;
}

bool func2(int32_t v)
{
    return false;
}
```

Inside `main` function, we test two cases as
```C++
int32_t main()
{
    testMatch(1, true);
    testMatch(2, false);
    return 0;
}
```

Build the test and run it via 
```
$ clang main.cpp --std=c++17
$ ./a.out
```
You can get the outputs as 
```
Passed!
Passed!
```

Now we are all done!
Now we have an open question: what happens if no patterns get matched. How can we implement a `default` case?
