In last section, we defined our `when` pattern, checking if the result of the predicate is true.
This is like the `?` pattern in Racket.
>(? expr pat ...) — applies expr to the value to be matched, and checks whether the result is a true value; the additional pats must also match; i.e., ? combines a predicate application and an and pattern. However, ?, unlike and, guarantees that expr is matched before any of the pats.
An example is 
```Racket
> (match "3.14"
   [(app string->number (? number? pi))
    `(I got ,pi)])
'(I got 3.14)
```
Eliminating the `app` part, the example becomes 
```Racket
> (match 3.14
   [(? number? pi)
    `(I got ,pi)])
'(I got 3.14)
```
We have not implemented `identifier` pattern in our lib yet, so a more reduced example would be
```Racket
> (match 3.14
   [(? number?)
    `(I got a number)])
'(I got a number)
```
Can we mimic the `app` pattern using the `?` alone?
Sure, we can choose to change the value to be matched, or modify the predicate.
```Racket
> (match (string->number "3.14")
   [(? number?)
    `(I got a number)])
'(I got a number)
```
```Racket
> (match "3.14"
   [(? (lambda (v) (number? (string->number v))))
    `(I got a number)])
'(I got a number)
```
The examples are not very useful, but they help demonstrate the relationship between `app` pattern and `?` pattern.
We can not always changes the value to be matched since in general we will match the value to lots of patterns and other patterns do not need that transformation. Then the only way is to construct a new complex predicate.
We can see that the `?` pattern alone version makes the pattern harder to read.

Let's see what things would be like using our lib.
Suppose we have an `app` pattern that accepts a transformer function and a sub pattern.
We want to check if the square of a number is larger than 100. Then we can write our codes as
```C++
int square(int t)
{
    return t*t;
}

match(n)
(
    pattern(app(square, _ > 100)) = []{ return true; },
    pattern(_)                    = []{ return false;}
);
```
The `when` predicate alone version would be
```C++
match(n)
(
    pattern(when([](int n){return square(n) > 100;})) = []{ return true; },
    pattern(_)                                        = []{ return false;}
);
```
Suppose we want to check if the square of a number is exactly 100, then we have

```C++
int square(int t)
{
    return t*t;
}

match(n)
(
    pattern(app(square, 100)) = []{ return true; },
    pattern(_)                = []{ return false;}
);
```
The `when` predicate alone version would be
```C++
match(n)
(
    pattern(when([](int n){return square(n) == 100;})) = []{ return true; },
    pattern(_)                                         = []{ return false;}
);
```
The most powerful part of `app` pattern is that it can accept any sub pattern. While for `when` precidate alone version, you have to change the predicate to refelct that.

Now let's implement our `app` pattern. That is as simple as
```C++
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
};
```
That's all for today's `app` pattern.