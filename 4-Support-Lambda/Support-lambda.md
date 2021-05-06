Since all patterns are of their own types (Even not in this time. Not true for all integrals. We can try storing them in a std::initializer_list first.), there is no way to store them in a container, unless we introduce type erasure. That would have a bad impact on performance.

How do you loop over `variadic parameters` or a `std::tuple`? Template specialization, SFINAE or fold expression introduced in C++17.
