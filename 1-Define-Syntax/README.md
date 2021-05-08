# Define pattern matching syntax.

What do you expect your pattern matching library look like?

Let's get some hints from other languages.

We heard of rust lots of times recently. Let's take a look at Rust pattern matching syntaxes.

```rust
let num = Some(4);
match num {
  Some(x) if x < 5 => println!("less than five: {}", x), Some(x) => println!("{}", x),
  None => (),
}
```

Cool! The syntax is 

```rust
match VALUE {
  PATTERN1 => EXPRESSION1,
  PATTERN2 => EXPRESSION2,
  PATTERN3 => EXPRESSION3,
}
```

What about Haskell, a very famous functional programming? After all, pattern matching is more of a functional programming stype programming.

```haskell
myHead (x:xs) = x
myHead [] = error "No head for empty list"
```

Hmmm, more like function overloading in C++. Not something we can borrow to C++.

What about Racket!

```Racket
> (match '(1 2)
    [(list x) (+ x 1)]
    [(list x y) (+ x y)])
3
```

Based on Rust and Racket syntaxes, we decide to adopt `match` as the keyword. (Of course we cannot define keywords, we will adopt that as our function/class/object name instead.)

We also heard that `pattern matching` should contains all functionality of `switch case`. Let's revist what `switch case` syntax looks like in C++.

```C++
switch (state) {
  case normal_score_state:
    func1(state);
    break;
  case forty_scoring_state:
    func2(state);
    break;
};
```

So a naive idea is to have somthing like 

```C++
match (state) {
  case normal_score_state:
    func1(state);
    break;
  case forty_scoring_state:
    func2(state);
    break;
};
```

But `case` can only be used for `switch`. Also, `break;` cannot be used.

What about

```C++
match (state) {
  normal_score_state : func1(state);
  forty_scoring_state : func2(state);
};
```

Quite similar to `Rust` style.

But still it cannot work. There is no `:` operator in C++ (neither is =>). And all statements inside a block will be executed sequentially. 

Then we can have

```C++
match (state) (
  normal_score_state = func1,
  forty_scoring_state = func2
);
```

How can this be syntaxly valid?

The first `()` construct an object, the second `()` calling the operator `()` of that object.

Since `match` should accept parameters of all kinds of types, `match` need to be a template class. But we may not want to provide the template type every time. Let's make `match` a template function instead, which can deduce the template type automatically. 

We will have something like 

```C++
template <typename T>
auto match(T const& t)
{
  return MatchHelper<T>{t};
}
```

What do the two lines inside the second `()` do?

```C++
normal_score_state = func1,
forty_scoring_state = func2
```

We want to match `state` against the two value patterns, `normal_score_state` and `forty_scoring_state`. When ``normal_score_state` is matched, func1 will be called. When`forty_scoring_state`. is matched, func2 will be called. We need to ensure that there are no cases escaping. 

That said, we need to hold the `pattern` info and the `action`. The assignment operator `=` will simply wrap the `pattern` - `action` pairs and pass them to `MatchHelper<T>::operator()`. We may not want to provide a super generic `operator = ` for all pattern types including any structures or literal values. Let's wrap the `pattern`s .

```C++
match (state) (
  pattern(normal_score_state) = func1,
  pattern(forty_scoring_state) = func2
);
```

`pattern` is a template function similar to `match`.

```C++
template <typename T>
auto pattern(T const& t)
{
  return PatternHelper<T>{t};
}
```



