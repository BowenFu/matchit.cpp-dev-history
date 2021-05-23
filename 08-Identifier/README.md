Now comes the really useful but a little chanllenging part, the `identifier` pattern.
We have mentioned the `identifier` pattern in previous sections. Now we are ready to introduce it to our pattern matching library.
Pattern matching would not be so powerful without `identifier` pattern. `idenfitifer` pattern is to pattern matching what hands are to human.
Suppose we are implementing the `ReLU` operator using our pattern matching library,
```C++
match(n)
(
    pattern(_ >= 0) = [n]{return n;},
    pattern(_)      = []{return 0;}
);
```
What if you are matching the result of an expensive computation?
```C++
match(expensiveComputation(n))
(
    pattern(_ >= 0) = [?]{return ?;},
    pattern(_)      = []{return 0;}
);
```
Or the transformer for `app` pattern is expensive and we want to return the result of the transformer?
```C++
match(n)
(
    pattern(app(expensiveTransformer, _ >= 0)) = [?]{return ?;},
    pattern(_)                                 = []{return -1;}
);
```
We need the `identifier` pattern.
How do we design our `identifier` pattern syntax?
Let's check `identifier` patterns from other languages.
A Rust sample:
```Rust
match meadow.count_rabbits() {
    0 => {} // nothing to say
    1 => println!("A rabbit is nosing around in the clover."),
    n => println!("There are {} rabbits hopping about in the meadow", n)
}
```
Non literals will be considered as identifiers automatically and no additional declarations or definitions are needed.
It is not possible for us to implement this unless we change the `C++` standard / compiler implementation or use some Macro hacks.
Let's see how other C++ pattern matching librarys implement this.
Example from [mpark's patterns library](https://github.com/mpark/patterns)
```C++
void is_same(int lhs, int rhs) {
  using namespace mpark::patterns;
  IDENTIFIERS(x, y);
  match(lhs, rhs)(
      pattern(x, x) = [](auto) { std::cout << "same\n"; },
      //      ^  ^ binding identifier (repeated)
      pattern(x, y) = [](auto, auto) { std::cout << "diff\n"; }
      //      ^  ^ binding identifier
  );
}

is_same(101, 101);  // prints: "same"
is_same(101, 202);  // prints: "diff"
```
The identifiers are more like placeholders, and correspond to the argument list of the handlers.

Example from [solodon4's Mach7 library](https://github.com/solodon4/Mach7)
```C++
void print(const boost::variant<double,float,int>& v)
{
    var<double> d; var<float> f; var<int> n;

    Match(v)
    {
      Case(C<double>(d)) cout << "double " << d << endl; break;
      Case(C<float> (f)) cout << "float  " << f << endl; break;
      Case(C<int>   (n)) cout << "int    " << n << endl; break;
    }
    EndMatch
}
```
Pre-declare / define the identifiers before the pattern matching parts.

We decide to adopt the second solution, i.e. pre-declaring / defining identifiers, since that is more expressive and less likely to confuse users.

With our library, the original Rust sample will be
```C++
Id<int> n;
match (meadow.count_rabbits()) (
    pattern(0) = []{return ""; }, // nothing to say
    pattern(1) = []{return "A rabbit is nosing around in the clover."; },
    pattern(n) = [&n]{return "There are {} rabbits hopping about in the meadow" + std::to_string(*n); }
)
```
Why `std::to_string(*n)` instead of `std::to_string(n)`? We avoid the implicit casting from `Id<T>` to `T` since that can be a source of bugs.
Just like using smart pointers, we force users to explicitly derefernce the identifiers.

We can then implement the `identifier` pattern as
```C++
template <typename Type>
class Id
{
public:
    Id() = default;
    template <typename Value>
    bool match(Value const& value) const
    {
        if (mHasValue)
        {
            return mValue == value;
        }
        mHasValue = true;
        mValue = value;
        return true;
    }
    Type& value() const
    {
        assert(mHasValue);
        return mValue;
    }
    Type& operator* () const
    {
        return value();
    }
private:
    mutable Type mValue;
    mutable bool mHasValue{false};
};

template <typename Type>
class PatternTraits<Id<Type>>
{
public:
    template <typename Value>
    static bool match(Id<Type> const& idPat, Value const& value)
    {
        return idPat.match(value);
    }
};

```
The first time the identifier is matched, the value is to bound to it. For the second time the bound value will be used to compare to the new value.
(We can implement the `Id` using `std::optional`. )
Why mark the members `mValue` and `mHasValue` as mutable? Why not pass them as non-const references?
That is a good question. We choose this way because we do not want to change all arguments and members to non-const references. And const reference looks more correct for patterns. Contributors may be confused when they look into the codes and find that all these members are `non-const` references.

Since we already decided to use pre-defined identifiers instead of argument passing, we can change our handlers from accpeting the value to accepting zero arguments, i.e. from 
```C++
result = pattern.execute(mValue);
```
to
```C++
result = pattern.execute();
```
in MatchHelper::operator(). And corresponding changes in core.h and patterns.h.
Now we can use the `identifier` patterns as in the above count_rabbits sample.

Let's further handle large n specially from last sample,
```C++
Id<int> n;
auto s = match (10000) (
    pattern(0) = []{return ""; }, // nothing to say
    pattern(1) = []{return "A rabbit is nosing around in the clover."; },
    pattern(and_(n, _ <= 1000)) = [&n]{return "There are" + std::to_string(*n) + "rabbits hopping about in the meadow."; },
    pattern(app([](int i){return i/1000;}, n)) = [&n]{return "There are " + std::to_string(*n) + " kilo rabbits hopping about in the meadow."; }
);
std::cout << s;
```
Running it,
```
Assertion failed: (matched), function operator(), file ./core.h, line 37.
[1]    3870 abort      ./a.out
```
How can this happen? We expect it to print out
```
There are 10 kilo rabbits hopping about in the meadow.
```
.

Well, this is the problem due to the shared identifier across multiple pattern cases.
A tiny snippet to reproduce the problem is
```C++
Id<int> n;
assert(n.match(5));
assert(n.match(6));
```
.

If you change the second line to `assert(n.match(5));` the codes can pass though.
So the problem is identifiers are binding values only for the first time they gets matched.
All later calls to `match` is simply comparing the bound values to new values.

One solution is to ask users never to share identifiers across multiple patterns. But that sounds like a strange requirement and makes our pattern matching library harder to use.
Another solution is to reset the identifiers for each pattern case.
Then the decision is on how to implement this `reset` functionality.
Can we have a context to record which identifiers are used in the current pattern case and reset all of them after or before matching the pattern?
Or should we maintain a global mapping for all matched identifiers (record in the `match` function)?
These solutions are not ideal. We decide to add a new `resetId` method to handle this. And call it before calling match for each pattern case. (Call resetId before or after match does not matter, it is just a preference issue.)
This requires us to implement the `resetId` method for all patterns, since we can have nested patterns.
Inside `PatternPair` now we have
```C++
template <typename Value>
bool match(Value const& value) const
{
    resetId(mPattern);
    return ::match(mPattern, value);
}
```
And add a new method `reset` to Id class
```C++
template <typename Type>
class Id
{
public:
    void reset() const
    {
        mHasValue = false;
    }
};
```
.
And the PatternTraits
```C++
template <typename Type>
class PatternTraits<Id<Type>>
{
public:
    ...
    static void resetId(Id<Type> const& idPat)
    {
        idPat.reset();
    }
};

```
Implementations of `resetId` for other PatternTraits are trival and calling sub patterns' resetId recursively is enough.

Now
```C++
Id<int> n;
auto s = match (10000) (
    pattern(0) = []{return ""; }, // nothing to say
    pattern(1) = []{return "A rabbit is nosing around in the clover."; },
    pattern(and_(n, _ <= 1000)) = [&n]{return "There are" + std::to_string(*n) + "rabbits hopping about in the meadow."; },
    pattern(app([](int i){return i/1000;}, n)) = [&n]{return "There are " + std::to_string(*n) + " kilo rabbits hopping about in the meadow."; }
);
std::cout << s;
```
gives
```
There are 10 kilo rabbits hopping about in the meadow.
```
.
Now the end of the long journey for `identifier` pattern, congratulations.
