In this section, we will introduce segment matching into our library.
This pattern is common in other programming languages or libraries.
In Racket ... and ___ are aliases for zero or more matches.
```Racket
> (match '(1 2 3)
    [(list 1 a ...) a])
'(2 3)
```
In Rust we can use the .. syntax to ignore the rest.
```Rust
match origin {
    Point { x, .. } => println!("x is {}", x),
}
```

Of course we are more close to Rust. It would be great if we can use the `...` directly.
But that is not allowed in C++. We choose `ooo` as the symbol here since if you squint, it can look like `...`, if you have a really poor sight, LOL.

Let's see how this pattern can be used:
```C++
match(std::make_tuple("123", 3, 3, 3, 3, 2))
(
    pattern(ds(std::string("123"), ooo(3), 2)) = []{return 0;}
);
```
We match a tuple of a string "123", followed by four integers of 3, and the last one is integer 2.
The pattern is direct, a `ds` pattern consisting of first the string "123", followed by an `ooo` pattern containing a subpattern `3`, the end being 2.
The interesting part is the `ooo(3)` part, it can match arbitrary number of 3s.

We do not forbid multiple `ooo` patterns in the same `ds` pattern now. Rust treats it as compile error: "`..` can only be used once per tuple or tuple struct pattern".
We also did not see multiple `...` allowed in Racket patterns.

We may forbid this as well some time later if we find that it may bring more problems than benefits.

Currently `ooo` pattern works as follows:
Inside a `ds` pattern, loop over all subpatterns:
1. If no `ooo` patterns have been detected so far, match them as normal;
2. Once a `ooo` pattern is found, forward the rest patterns including the current `ooo` pattern to another path.
3. Try non-greedy match for the current `ooo` pattern, i.e., matching 0 items first, then 1, then 2..., return true if match succeeds;
4. When another `ooo` pattern is detected during 3, execute the process recursively.
5. When some pattern does not match given value, fail that `ooo` match, increase the number of `ooo` matched items.
6. return false if the end if reached and no matched case has been found.

The computation cost can increase fast if there are multiple `ooo` patterns. O(n^2) for a single `ooo` pattern, O(n^3) for two `ooo` patterns, O(n^(m+1)) for m `ooo` patterns. (Assume O(n) complexity for 0 `ooo` pattern.)

