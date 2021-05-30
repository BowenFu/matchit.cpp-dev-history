In this section, we will introduce a `guard` pattern.
`guard` clause to pattern matching is what `require` clause to template programming inside C++20.
We will use `when` as its name since we prefer to leave `if` for something like tenary opeartors.
The ideal syntax would be like
```C++
Id<int32_t> i, j;
match(x)(
    pattern(i, j).when(i + j == 10}) = [] { return 3; },
    pattern(_) = [] { return 1; });
```
But we due to C++ syntax limitation we have to compromise to
```C++
Id<int32_t> i, j;
match(a, b)(
    pattern(i, j).when([&] { return *i + *j == 10; }) = [] { return 3; },
    pattern(_) = [] { return 1; });
```
.
Still it is possible to achieve something like `when(i + j == 10})`, the trick is to overload operators for `Id`, `operator+` will return an expression. `i + j == 10` returns a zero-argument function object at last.
Will leave this to future improvement.

Recall that we already have a `when` pattern. Can't that be enough?
What is the use case of the `when` pattern?
```C++
Id<int32_t> i;
match(n)
(
    pattern(and_(when(within(1, 10)), i)) = []{return 'A';},
    pattern(_)                            = []{return 'B';}
);
```
It can cast restrictions on a single value to be matched. In order to cast restrictions on more than one values, we have to write a complex predicate and destructure some value ourselves and then check on the resulted values. This will make our library hard to use and it is against our design.
That is the reason why we make the `guard` pattern as our first class pattern.
Can the new `guard` pattern replace the original `when` pattern?
That is possible. 
```C++
Id<int32_t> i;
match(n)
(
    pattern(i).when([&]{return *i >= 10 && *i <= 10;}) = []{return 'A';},
    pattern(_)                                         = []{return 'B';}
);
```
Not very clean compared to
```C++
match(n)
(
    pattern(and_(_ >= 1, _ <= 10)) = []{return 'A';},
    pattern(_)                     = []{return 'B';}
);
```
So to faciliate the use of our library, we prefer to keep both of them.
Though it is allowed to have the same name for the both patterns (one as a free function, one as a member function), we prefer to name them differently so that users will not be confused.
Then new `guard` pattern will win the `when` name to be consistent with other programming languages (Racket) or pattern matching libraries ([mpark's patterns library](https://github.com/mpark/patterns)). Note that `if` keyword is used for guard pattern in Rust (similar meaning to `when` in English).
The original `when` pattern contains a predicate, we can call it `predicate` pattern.
To make the codes sound more natural, we decide to call it `meet` pattern.
```C++
Id<int32_t> i;
match(n)
(
    pattern(and_(meet(within(1, 10)), i)) = []{return 'A';},
    pattern(_)                            = []{return 'B';}
);
```
.
We have come to the end of this section. Will explore more of identifier patterns in next section.