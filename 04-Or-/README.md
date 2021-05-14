In last section, we added `WildCard` pattern to mimic the defaul clause in `switch case` block.

Is there anything we can borrow from `switch case`?

It is a common pattern in `switch` statements to have common logics for multiple cases.
In that case, we will have something like
```C++
switch(label)
{
  case L1:
  case L2:
    // do somthing;
    break;
  case L3:
    // do something else;
    break;
  default:
    break;
}
```

Can we implement this in our `pattern matching` library?
Multiple patterns, single handler.
The vertical bar (`|`) is adopted by Rust. Can we adopt this in our library as well?
Maybe possible, but we need to overload that operator. Assume you want to match an `integer`, do you want to change the semantics of `5 | 6`?
Maybe not.
Why this can be done in Rust? Because Rust pattern matching is implemented as a language feature, not a library. The same syntax can have different semantics inside and outside `pattern` context.
Since we are implementing this feature as a library, we have to make sure we do not change normal semantics.

Can common values can be plain patterns. We cannot use operator overloading here.
Let's define a wrapper over them.
What comes to your mind? `any(5, 6)` or `anyof(5, 6)`?
Let's adopt one with a wider semantics, `or(5, 6)`, since we can have other patterns inside it, not just literal/value patterns. This is inspired by the Racket powerful pattern matching syntax.

https://docs.racket-lang.org/reference/match.html
```Racket
> (match '(1 2)
   [(or (list a 1) (list a 2)) a])
1
```

Since `or` is a keywork of C++, we adopt the variant `or_`.

How do we implent that?
`or_` can accept an arbitrary number of sub patterns. That is to say, we need to use a `tuple` to store these sub patterns. Why not `array` or `vector`? Because these patterns can be of aribitrary types, not guaranteed to be of the same type.

```C++
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
    std::tuple<Patterns const&...> mPatterns;
};

template <typename... Patterns>
auto or_(Patterns const&... patterns) -> Or<Patterns...>
{
    return Or<Patterns...>{patterns...};
}
```

Nothing special, quite simple, right?.
The great commission is not finished.
What is the expected behavior of `or` pattern or `any` pattern?
Succeed when one of the patterns get matched! One survivor is enough.
Let's define its pattern traits.
```C++
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
                return (PatternTraits<Patterns>::match(patterns, value) || ...);
            },
            orPat.patterns());
    }
};
```
Cool! We use `fold expression` here. In order to use `fold expression`, we need to expand the variadic parameters, via the powerful `std::apply`.

Let's test it.
```C++
auto x = match(input)(
    pattern(or_(56, 59)) = func2,
    pattern(_) = [](int32_t){ return 111; }
);
 
testMatch(59, 12); // or_ matched.
```
Good job!
Now we are all done and it's the time to award yourself.