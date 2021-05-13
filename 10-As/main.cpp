#include "core.h"
#include "patterns.h"

template <typename V, typename U, typename Func>
void testMatch(V const &input, U const &expected, Func matchFunc)
{
    auto const x = matchFunc(input);
    if (x == expected)
    {
        printf("Passed!\n");
    }
    else
    {
        printf("Failed!\n");
        if constexpr(std::is_same_v<U, int>)
        {
            std::cout << x << " != " << expected << std::endl;
        }
    }
}

bool func1(int32_t v)
{
    return true;
}

int64_t func2(int32_t v)
{
    return 12;
}

int32_t test1()
{
    auto const matchFunc = [](int32_t input)
    {
        Id<int> ii;
        // ii.match(5);
        return match(input)(
            pattern(1) = func1,
            pattern(2) = func2,
            pattern(or_(56, 59)) = func2,
            // pattern(when([](auto&& x){return x < 0; })) = [](int32_t){ return -1; },
            pattern(_ < 0) = [](int32_t){ return -1; },
            pattern(_ < 10) = [](int32_t){ return -10; },
            pattern(and_(_ < 17, _ > 15)) = [](int32_t){ return 16; },
            pattern(app([](int32_t x){return x*x; }, when([](auto&& x){return x > 1000; }))) = [](int32_t){ return 1000; },
            pattern(app([](int32_t x){return x*x; }, ii)) = [&ii](int32_t){ return ii.value() + 0; },
            pattern(ii) = [&ii](int32_t){ return ii.value() + 1; },
            pattern(_) = [](int32_t){ return 111; }
        );
    };
    testMatch(1, true, matchFunc);
    testMatch(2, 12, matchFunc);
    testMatch(11, 121, matchFunc); // Id matched.
    testMatch(59, 12, matchFunc); // or_ matched.
    testMatch(-5, -1, matchFunc); // when matched.
    testMatch(10, 100, matchFunc); // app matched.
    testMatch(100, 1000, matchFunc); // app > when matched.
    testMatch(5, -10, matchFunc); // _ < 10 matched.
    testMatch(16, 16, matchFunc); // and_ matched.
    return 0;
}

int32_t test2()
{
    auto const matchFunc = [](std::tuple<char, int, int> const& input)
    {
        Id<int> i;
        Id<int> j;
        auto [op, lhs, rhs] = input;
        return match(op, lhs, rhs)(
        // return match(input)(
            pattern(ds('/', 1, 1)) = [](auto&&){ return 1; },
            pattern(ds('/', 0, _)) = [](auto&&){ return 0; },
            // pattern(ds('/', _)) = [](auto&&){ return 0; },
            pattern(ds('*', i, j)) = [&i, &j](auto&&){ return i.value() * j.value(); },
            pattern(ds('+', i, j)) = [&i, &j](auto&&){ return i.value() + j.value(); },
            pattern(_) = [&i, &j](auto&&){ return -1; }
        );
    };
    testMatch(std::make_tuple('/', 1, 1), 1, matchFunc);
    testMatch(std::make_tuple('+', 2, 1), 3, matchFunc);
    testMatch(std::make_tuple('/', 0, 1), 0, matchFunc);
    testMatch(std::make_tuple('*', 2, 1), 2, matchFunc);
    testMatch(std::make_tuple('/', 2, 1), -1, matchFunc);
    return 0;
}

struct A
{
    int a;
    int b;
};
bool operator == (A const lhs, A const rhs)
{
    return lhs.a == rhs.a && lhs.b == rhs.b;
}
int32_t test3()
{
    auto const matchFunc = [](A const& input)
    {
        Id<int> i;
        Id<int> j;
        Id<A> a;
        // compose patterns for destructuring struct A.
        // auto const dsA = [&i]()
        // // auto const dsA = [](Id<int>& x)
        // {
        //     return and_(app(&A::a, i), app(&A::b, 1));
        // };
        // Debug<decltype(dsA(i))> xx;
        // auto x = and_(app(&A::b, 1), a); // and(app, )出错
        auto x = [&]
        {
            // return and_(app(&A::b, 1));
            return and_(app(&A::b, 1), app(&A::a, i));
        };
        return match(input)(
            pattern(and_(app(&A::a, i), app(&A::b, 1))) = [&i](auto&&){ return i.value(); },
            // pattern(dsA()) = [&i](auto&&){ return i.value(); },
            pattern(_) = [](auto&&){ return -1; }
        );
    };
    testMatch(A{3, 1}, 3, matchFunc);
    auto a = A{2, 1};
    testMatch(a, 2, matchFunc);
    return 0;
}

enum class Kind
{
    kZERO,
    kONE,
    kTWO
};

class K
{
public:
    virtual ~K() = default;
    virtual Kind kind() const
    {
        return Kind::kZERO;
    }
};

class One : public K
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

bool operator==(One const&, One const&)
{
    return true;
}

int32_t test4()
{
    auto const matchFunc = [](K const& input)
    {
        Id<int> i;
        Id<int> j;
        Id<One> one;
        auto const castWhen = when([&one](K const& k)
        {
            std::cout << static_cast<int>(k.kind()) << std::endl;
            if (k.kind() == Kind::kONE)
            {
                return match(one, static_cast<One const&>(k));
            }
            return false;
        });
        return match(input)(
            pattern(castWhen) = [&one](auto&&){ return 1; },
            pattern(_) = [](auto&&){return 2;}
        );
    };
    testMatch(One{}, 1, matchFunc);
    return 0;
}

int main()
{
    test1();
    test2();
    test3();
    test4();
    return 0;
}