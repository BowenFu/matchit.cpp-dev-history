We have added lots of patterns and it is the time to clean up our codes.
We find that there are multiple functions called the same name, i.e., `match`.
Let's rename and distinguish them.
We rename the one in the PatternPair to `matchValue` since it accepts a value.
We rename these accepting two arguments (patterns and values) to `matchPattern`.

That are the main changes in this section.

Recall that we let `match` accept variadic parameters in last section, making it possible to match multiple values at the same time.
Similarly in this section we let `pattern` accept variadic parameters as well.
Making `match` accept variadic parameters saves an extra `std::make_tuple`, while makeing `pattern` accept variadic parameters saves an extra `ds` and also make it consistent with `match`.
Before users can write codes as
```C++
match(std::make_tuple(1, 2))
(
    pattern(ds(1,2)) = []{return 1;}
);
```
Now they can writes codes as
```C++
match(1, 2)
(
    pattern(1, 2) = []{return 1;}
);
```
The codes are easier to read.

To make `pattern` accept variadic parameters, we need to create a temporary Ds object with the parameters and pass it to `PatternHelper`.
Similar story again, we need to store the temporary object as value .
So we need to change the `mPattern` in `PatternHelper` from reference to value.
That is all of this section.