We implemented `or` pattern in last section. That makes it possible for us to mimic the multiple cases sharing the same handler in switch statements.
But sometimes we may want to dispatch handlers based on ranges. Suppose we return 'A' for [1,10], 'B' otherwise. How would you code?
Something like the following snippet?
```C++
switch (n)
{
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        return 'A';
    default:
        return 'B';
}
```
Maybe not. You may code it as
```C++
if (n >= 1 && n <= 10)
{
    return 'A';
}
else
{
    return 'B';
}
```
or simply
```C++
return (n>=1 && n<=10) ? 'A' : 'B';
```
Using our lib, we have to write codes like
```C++
match(n)
(
    pattern(or_(1,2,3,4,5,6,7,8,9,10)) = []{return 'A';},
    pattern(_)                         = []{return 'B';}
);
```
To avoid this verboseness, we decide to add a `when` pattern. Why not call it `if` pattern. That is because there is often a `else` case for `if` statement, that is not what we plan to implement. In Racket, you always have `then` and `else` cases for a `if`. `when` is different.
We will use it as
```C++
match(n)
(
    pattern(when([](auto n){return n>=1 && n<= 10;})) = []{return 'A';},
    pattern(_)                                      = []{return 'B';}
);
```

Hmmm, even more verbose. But it would be simpler if you extract the function and reuse it.
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
We see that `when` pattern will accept a predicate.
Then we can simply implement it as 
```C++
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
```
Quite straightforward.

Sometimes we may not want to define a verbose lambda or a separate function. Can we make the usage of `when` pattern easier?
There comes the utility functions.
Since WildCard `_` is so special and easy to read and write, we can use it to help users express their conditionals.
We overload four operators for this purpose.
```C++
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
```

Now users can write the sample as
```C++
match(n)
(
    pattern(_ < 1)   = []{return 'B';},
    pattern(_ <= 10) = []{return 'A';},
    pattern(_)       = []{return 'B';}
);
```
Much better now, right?
Well done!