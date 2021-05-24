In last section, we can implement `destructure` pattern via composing existing patterns.
In this sectio
n, we will further exploring composing patterns to implement the `as` pattern.
This can be used to implement the visitor pattern in object-oriented programming.
Suppose you have a base class `Num` and two derived classes `One` and `Two`.
```C++
enum class Kind
{
    kONE,
    kTWO
};

class Num
{
public:
    virtual ~Num() = default;
    virtual Kind kind() const = 0;
};

class One : public Num
{
public:
    Kind kind() const override
    {
        return Kind::kONE;
    }
    int get() const
    {
        return 1;
    }
};

class Two : public Num
{
public:
    Kind kind() const override
    {
        return Kind::kTWO;
    }
    int get() const
    {
        return 2;
    }
};
```
Instead of downcasting via `dynamic_cast`, we choose to downcast according to the tag returned by `kind()`.
If `kind()` returns `KONE`, we downcast the object from `Num` to `One`. If `kind()` returns `kTWO`, we downcast to `Two`.
Checking `kind()` results, we need an `app` pattern. Downcast the object, we need another `app` pattern.
Then we can compose patterns like
```C++
template <Kind k>
auto const kind = app(&Num::kind, k);

template <typename T>
auto const cast = [](Num const& input){
    return static_cast<T const&>(input);
}; 

template <typename T, Kind k>
auto const as = [](Id<T> const& id)
{
    return and_(kind<k>, app(cast<T>, id));
};
```
And we may want to use the patterns like
```C++
Id<One> one;
auto const matchFunc = [](Num const& input)
{
    match(input)(
        pattern(as<One, Kind::kONE>(one)) = [&one]{ return one.value().get(); },
        pattern(kind<Kind::kTWO>) = []{ return 2; },
        pattern(_) = []{return 3;}
    );
};
matchFunc(One{});
```
But this does not even compile! We need to implement the `operator==` for the classes, otherwise identifier patterns will not work.
```C++
bool operator==(One const&, One const&)
{
    return true;
}

bool operator==(Two const&, Two const&)
{
    return true;
}
```
Still, this cannot compile. Why? We store the value to be matched in `MatchHelper`, of course this does not work for objects that cannot be copied. The abstract class in this case.
Can we change it from value to reference? Let's try it.
It does compile, but there are some random test failure. What's the problem?
In last section, in order to facilitate to match multiple values at the same time, we overload the `match` function to accept multiple values, then we wrap them in a tuple and construct a `MatchHelper` object.
We are passing the temporary tuple to `MatchHelper` constructor and return the `MatchHelper` object which references the temporary tuple.
That is the root cause.
So we need to store the tuple copy, not reference the temporary one.
Store it as a value or reference it, that is a question.
We are in a dilemma? 
Should we partially specialize the template for `std::tuple` and store all tuples as values?
Of course not. What if users creates their tuples containing some objects that cannot be copied.
Let's summarize what we find now. 
We can reference to what users pass to `match(...)` since these objects will at least survive until the end of the whole `match(xxx)(xxx)` expression, but not the temporary objects created inside `match`.
That is to say, we need to distinguish single argument `match()` and multiple argument `match()`. MatchHelper stores references for the first one and values for the second one.
We need the two versions of `match()` to pass this information to `MatchHelper`.
Then we can write the codes as
```C++
template <typename Value, bool byRef>
class ValueType
{
public:
    using ValueT = Value const;
};

template <typename Value>
class ValueType<Value, true>
{
public:
    using ValueT = Value const&;
};

template <typename Value, bool byRef>
class MatchHelper
{
...
private:
    typename ValueType<Value, byRef>::ValueT mValue;
};

template <typename Value>
auto match(Value const& value)
{
    return MatchHelper<Value, true>{value};
}

template <typename First, typename... Values>
auto match(First const& first, Values const&... values)
{
    auto const x = std::forward_as_tuple(first, values...);
    return MatchHelper<decltype(x), false>{x};
}
```
Finally the codes pass.
Hmmm, but there are still some failure.
They are caused by that we do not store patterns.
Let's store our patterns since they are all defined by ourselves and we can guarantee that they are copyable and cheap to copy.
One more issue, the identifier patterns.
If we always copy patterns, how can we pass the bound value from one copy to another?
`std::shared_ptr` is the rescue.
That is not even enough. 
Some values to be bound can be copied, while some cannot. Some are cheap to copy, while some are expensive.
So we can let users decide. We add a `own` template parameter. 
Now the end of the section.