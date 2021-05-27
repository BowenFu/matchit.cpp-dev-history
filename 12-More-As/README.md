We further rename some functions in this section.
We switch the order of the parameters from pattern, value to value, pattern to better match the function name `matchPattern`.
And we rename the one in the PatternTraits to `matchPatternImpl` to further distinguish them from the global one.

We will show how powerful composing patterns would be in this section.

`Optional` pattern is very common. We have `Some` in Rust. 
We can mimic that with our library, as simple as
```C++
template <typename T>
auto const cast = [](auto && input){
    return static_cast<T>(input);
}; 

auto const some = [](auto const& id)
{
    auto deref = [](auto&& x) { return *x; };
    return and_(app(cast<bool>, true), app(deref, id));
};
auto const none = app(cast<bool>, false);
```

Firstly, it casts the value to a boolean value and check if it is true.
If that is the case, we can further dereference the value.
We can use this for raw pointer, `std::unique_ptr`, `std::shared_ptr`, `std::optional` and all other types that implement the conversion operator to bool and the dereference (`*`) operator.
We can use the patterns as
```C++
auto const canDeref = [](auto const& i)
{
    Id<int32_t> x;
    return match(i)(
        pattern(some(x)) = []{return true;},
        pattern(none) = []{return false;}
    );
};

EXPECT_TRUE(canDeref(std::make_unique<int32_t>(2)));
EXPECT_FALSE(canDeref(std::optional<int32_t>{}));
```

Let's see how can our library works with vist `std::variant`.
First we define `getAs`
```C++
template <typename T>
auto const getAs = [](auto&& id)
{
    auto getIf = [](auto&& p){return std::get_if<T>(std::addressof(p)); };
    return app(getIf, some(id));
};
```
`std::get_if` will return nullptr if the call fails. That is how this pattern works and we can match the result against the `some` pattern.
Then we can write our `visit` as 
```C++
auto const visit = [](auto const& i)
{
    return match(i)(
        pattern(getAs<Square>(_)) = []{return std::string("Square");},
        pattern(getAs<Circle>(_)) = []{return std::string("Circle");}
    );
};
```
And test it with
```C++
std::variant<Square, Circle> sc;
sc = Square{};
EXPECT_EQ(visit(sc), "Square");
```

This process also applies to `std::any`.
We have
```C++
template <typename T>
auto const anyAs = [](auto&& id)
{
    auto anyCast = [](auto&& p){return std::any_cast<T>(std::addressof(p)); };
    return app(anyCast, some(id));
};
auto const visitAny = [](auto const& i)
{
    return match(i)(
        pattern(anyAs<Square>(_)) = []{return std::string("Square");},
        pattern(anyAs<Circle>(_)) = []{return std::string("Circle");}
    );
};

std::any sc;
sc = Square{};
EXPECT_EQ(visitAny(sc), "Square");
```

That is all of this section.