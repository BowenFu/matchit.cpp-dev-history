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