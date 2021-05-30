Since we already have `or` pattern and `and` pattern, it is natural to have `not` pattern for logical completement.
Inside STL, we have `any_of`, `all_of`, and `none_of`. Their counterparts in our library are `or`, `and`, and `not` patterns, respectively. (But not exactly.)
For the STL `any_of`, `all_of`, and `none_of`, we are looping over some container and check if any or all or none of the elements in the container meet the predicate.
In our library, `or`, `and`, `not`, we are checking against sub patterns/predicates to see if they can be matched.
Actually they are more similar to C++ keyword `or/||`, `and/&&`, and `not/!`.
```C++
match(i)(
    pattern(and_(_ > 15, _ < 16))               = []{return 1;}, // i > 15 && i < 16
    pattern(or_(1, 2))                          = []{return 2;}, // i == 1 || i == 2
    pattern(not_(2))                            = []{return 2;}, // i == 1 || i == 2
    pattern(_)                                  = []{return -1;}
);
```
Let's have some complex `not_` patterns.
```C++
match(i)(
    pattern(not_(and_(_ > 15, _ < 16)))         = []{return 1;}, // !(i > 15 && i < 16)
    // pattern((or_(not_(_ > 15), not_(_ < 16))))  = []{return 1;}, // (!(i > 15) || !(i < 16))
    // pattern((or_(_ <= 15), _ >= 16))            = []{return 1;}, // (i <= 15) || (i >= 16)
    pattern(not_(or_(1, 2)))                    = []{return 2;}, // !(i == 1 || i == 2)
    // pattern(and_(not_(1), not_(2)))             = []{return 2;}, // (i != 1) && (i != 2)
    pattern(_)                                  = []{return -1;}
);
```
The implementation of `not_` is quite intuitive. 
```C++
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
```

One thing to note is about the `identifier` pattern inside `not` pattern.
Actually this issue also exists in `or` pattern.
For example
```C++
Id<int> a;
match(i)
(
    pattern(or_(5, a)) = []{return 1;}
);
```
is this a valid case?
Sure.
What about this one?
```C++
Id<int> a;
match(i)
(
    pattern(or_(5, a)) = [&a]{return *a;}
);
```
Maybe not. When i is 5, a will not be bound and *a is not a valid call and will trigger runtime error.
```C++
Id<int> a;
match(i)
(
    pattern(or_(a, 5)) = [&a]{return *a;}
);
```
This is a valid case though since we guarantee the match order of sub patterns of the `or` pattern.
But writing patterns like this is misleading, and should be simplified to
```C++
Id<int> a;
match(i)
(
    pattern(a) = [&a]{return *a;}
);
```
Should we forbid `identifier` pattern inside `or` pattern?
Maybe not. We do see some practical use cases.
Say
```C++
Id<int> a;
match(i, j)
(
    pattern(a, or_(a, 5)) = [&a]{return *a;}
);
```
This pattern means that j should be 5 or equal to i;
But writing this as
```C++
Id<int> a;
match(j, i)
(
    pattern(or_(a, 5), a) = [&a]{return *a;}
);
```
is totally wrong since when j is 5 and i is not 5, the pattern will not be matched as expected.
Writing it as
```C++
Id<int> a;
match(j, i)
(
    pattern(or_(5, a), a) = [&a]{return *a;}
);
```
is right.
A lesson learnt from this example is to always put `identifer` pattern or patterns containing `identifier` sub patterns as the last sub pattern of `or` pattern.

Now let's talk about the issues of `identifier` pattern inside `not` pattern.
Say
```C++
Id<int> a;
match(i, j, k)
(
    pattern(not_(a), not_(a), a) = [&a]{return *a;} // i != k && j != k
);
```
this pattern intends to be matched when `i != k && j != k`. Will it work?
This pattern will match `i`, `j`, `k` against `not_(a)`, `not_(a)`, `a` consequently.
But `a` for an unbound identifier will match any values, i.e. always return true, meaning that `not_(a)` will always return false.
How to fix this?
Users have to ensure that only bound identifiers appear inside `not` patterns.
Changing the matching order to
```C++
Id<int> a;
match(k, i, j)
(
    pattern(a, not_(a), not_(a)) = [&a]{return *a;} // i != k && j != k
);
```
will fix the issue.

Should we throw errors when we detect unbound identifiers inside `or` and `not` patterns?
That is a good question. We will revisit this later and leave this as a future improvement.