//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/XRPLF/rippled/
    Copyright (c) 2023 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <ripple/beast/unit_test.h>
#include <ripple/json/MultivarJson.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

namespace ripple {
namespace test {

namespace {

// This needs to be in a namespace because of deduction guide
template <typename... Ts>
struct Overload : Ts...
{
    using Ts::operator()...;
};
template <typename... Ts>
Overload(Ts...) -> Overload<Ts...>;

}  // namespace

struct MultivarJson_test : beast::unit_test::suite
{
    static auto
    makeJson(const char* key, int val)
    {
        Json::Value obj1(Json::objectValue);
        obj1[key] = val;
        return obj1;
    }

    template <std::size_t Size>
    struct Helper final
    {
        constexpr static std::size_t size = Size;
        constexpr static auto
        index(unsigned int v) noexcept -> std::size_t
        {
            return v - 1;
        }
        constexpr static auto
        valid(unsigned int v) noexcept -> bool
        {
            return v > 0 && v <= size;
        }
    };

    void
    run() override
    {
        Json::Value const obj1 = makeJson("value", 1);
        Json::Value const obj2 = makeJson("value", 2);
        Json::Value const obj3 = makeJson("value", 3);
        Json::Value const jsonNull{};

        MultivarJson<Helper<3>> subject{};
        static_assert(sizeof(subject) == sizeof(subject.val));
        static_assert(subject.size == subject.val.size());
        static_assert(
            std::is_same_v<decltype(subject.val), std::array<Json::Value, 3>>);

        BEAST_EXPECT(subject.val.size() == 3);
        BEAST_EXPECT(
            (subject.val ==
             std::array<Json::Value, 3>{jsonNull, jsonNull, jsonNull}));

        subject.val[0] = obj1;
        subject.val[1] = obj2;

        {
            testcase("default copy construction / assignment");

            MultivarJson<Helper<3>> x{subject};

            BEAST_EXPECT(x.val.size() == subject.val.size());
            BEAST_EXPECT(x.val[0] == subject.val[0]);
            BEAST_EXPECT(x.val[1] == subject.val[1]);
            BEAST_EXPECT(x.val[2] == subject.val[2]);
            BEAST_EXPECT(x.val == subject.val);
            BEAST_EXPECT(&x.val[0] != &subject.val[0]);
            BEAST_EXPECT(&x.val[1] != &subject.val[1]);
            BEAST_EXPECT(&x.val[2] != &subject.val[2]);

            MultivarJson<Helper<3>> y;
            BEAST_EXPECT((y.val == std::array<Json::Value, 3>{}));
            y = subject;
            BEAST_EXPECT(y.val == subject.val);
            BEAST_EXPECT(&y.val[0] != &subject.val[0]);
            BEAST_EXPECT(&y.val[1] != &subject.val[1]);
            BEAST_EXPECT(&y.val[2] != &subject.val[2]);

            y = std::move(x);
            BEAST_EXPECT(y.val == subject.val);
            BEAST_EXPECT(&y.val[0] != &subject.val[0]);
            BEAST_EXPECT(&y.val[1] != &subject.val[1]);
            BEAST_EXPECT(&y.val[2] != &subject.val[2]);
        }

        {
            testcase("set");

            auto x = MultivarJson<Helper<2>>{Json::objectValue};
            x.set("name1", 42);
            BEAST_EXPECT(x.val[0].isMember("name1"));
            BEAST_EXPECT(x.val[1].isMember("name1"));
            BEAST_EXPECT(x.val[0]["name1"].isInt());
            BEAST_EXPECT(x.val[1]["name1"].isInt());
            BEAST_EXPECT(x.val[0]["name1"].asInt() == 42);
            BEAST_EXPECT(x.val[1]["name1"].asInt() == 42);

            x.set("name2", "bar");
            BEAST_EXPECT(x.val[0].isMember("name2"));
            BEAST_EXPECT(x.val[1].isMember("name2"));
            BEAST_EXPECT(x.val[0]["name2"].isString());
            BEAST_EXPECT(x.val[1]["name2"].isString());
            BEAST_EXPECT(x.val[0]["name2"].asString() == "bar");
            BEAST_EXPECT(x.val[1]["name2"].asString() == "bar");

            // Tests of requires clause - these are expected to match
            static_assert([](auto&& v) {
                return requires
                {
                    v.set("name", Json::nullValue);
                };
            }(x));
            static_assert([](auto&& v) {
                return requires
                {
                    v.set("name", "value");
                };
            }(x));
            static_assert([](auto&& v) {
                return requires
                {
                    v.set("name", true);
                };
            }(x));
            static_assert([](auto&& v) {
                return requires
                {
                    v.set("name", 42);
                };
            }(x));

            // Tests of requires clause - these are expected NOT to match
            struct foo_t final
            {
            };
            static_assert([](auto&& v) {
                return !requires
                {
                    v.set("name", foo_t{});
                };
            }(x));
            static_assert([](auto&& v) {
                return !requires
                {
                    v.set("name", std::nullopt);
                };
            }(x));
        }

        {
            testcase("isMember");

            // Well defined behaviour even if we have different types of members
            BEAST_EXPECT(subject.isMember("foo") == decltype(subject)::none);

            {
                // All variants have element "One", none have element "Two"
                MultivarJson<Helper<2>> s1{};
                s1.val[0] = makeJson("One", 12);
                s1.val[1] = makeJson("One", 42);
                BEAST_EXPECT(s1.isMember("One") == decltype(s1)::all);
                BEAST_EXPECT(s1.isMember("Two") == decltype(s1)::none);
            }

            {
                // Some variants have element "One" and some have "Two"
                MultivarJson<Helper<2>> s2{};
                s2.val[0] = makeJson("One", 12);
                s2.val[1] = makeJson("Two", 42);
                BEAST_EXPECT(s2.isMember("One") == decltype(s2)::some);
                BEAST_EXPECT(s2.isMember("Two") == decltype(s2)::some);
            }

            {
                // Not all variants have element "One", because last one is null
                MultivarJson<Helper<3>> s3{};
                s3.val[0] = makeJson("One", 12);
                s3.val[1] = makeJson("One", 42);
                BEAST_EXPECT(s3.isMember("One") == decltype(s3)::some);
                BEAST_EXPECT(s3.isMember("Two") == decltype(s3)::none);
            }
        }

        {
            testcase("visitor");

            MultivarJson<Helper<3>> s1{};
            s1.val[0] = makeJson("value", 2);
            s1.val[1] = makeJson("value", 3);
            s1.val[2] = makeJson("value", 5);

            // Test different overloads
            static_assert([](auto&& v) {
                return requires
                {
                    v.visitor(
                        v,
                        std::integral_constant<unsigned, 1>{},
                        [](Json::Value&, std::integral_constant<unsigned, 1>) {
                        });
                };
            }(s1));
            BEAST_EXPECT(
                s1.visitor(
                    s1,
                    std::integral_constant<unsigned, 1>{},
                    Overload{
                        [](Json::Value& v,
                           std::integral_constant<unsigned, 1>) {
                            return v["value"].asInt();
                        },
                        [](Json::Value const&, auto...) { return 0; },
                        [](auto...) { return 0; }}) == 2);

            static_assert([](auto&& v) {
                return requires
                {
                    v.visitor(
                        v,
                        std::integral_constant<unsigned, 1>{},
                        [](Json::Value const&,
                           std::integral_constant<unsigned, 1>) {});
                };
            }(std::as_const(s1)));
            BEAST_EXPECT(
                s1.visitor(
                    std::as_const(s1),
                    std::integral_constant<unsigned, 2>{},
                    Overload{
                        [](Json::Value const& v,
                           std::integral_constant<unsigned, 2>) {
                            return v["value"].asInt();
                        },
                        [](Json::Value&, auto...) { return 0; },
                        [](auto...) { return 0; }}) == 3);

            static_assert([](auto&& v) {
                return requires
                {
                    v.visitor(v, 1, [](Json::Value&, unsigned) {});
                };
            }(s1));
            BEAST_EXPECT(
                s1.visitor(
                    s1,  //
                    3,
                    Overload{
                        [](Json::Value& v, unsigned) {
                            return v["value"].asInt();
                        },
                        [](Json::Value const&, auto...) { return 0; },
                        [](auto...) { return 0; }}) == 5);

            static_assert([](auto&& v) {
                return requires
                {
                    v.visitor(v, 1, [](Json::Value const&, unsigned) {});
                };
            }(std::as_const(s1)));
            BEAST_EXPECT(
                s1.visitor(
                    std::as_const(s1),  //
                    2,
                    Overload{
                        [](Json::Value const& v, unsigned) {
                            return v["value"].asInt();
                        },
                        [](Json::Value&, auto...) { return 0; },
                        [](auto...) { return 0; }}) == 3);

            // Test type conversions
            BEAST_EXPECT(
                s1.visitor(
                    s1,
                    std::integral_constant<unsigned, 1>{},  // to unsigned
                    [](Json::Value& v, unsigned) {
                        return v["value"].asInt();
                    }) == 2);
            BEAST_EXPECT(
                s1.visitor(
                    std::as_const(s1),
                    std::integral_constant<unsigned, 2>{},  // to unsigned
                    [](Json::Value const& v, unsigned) {
                        return v["value"].asInt();
                    }) == 3);
            BEAST_EXPECT(
                s1.visitor(
                    s1,  // to const
                    std::integral_constant<unsigned, 3>{},
                    [](Json::Value const& v, auto) {
                        return v["value"].asInt();
                    }) == 5);
            BEAST_EXPECT(
                s1.visitor(
                    s1,
                    3,  // to long
                    [](Json::Value& v, long) { return v["value"].asInt(); }) ==
                5);
            BEAST_EXPECT(
                s1.visitor(
                    std::as_const(s1),
                    1,  // to long
                    [](Json::Value const& v, long) {
                        return v["value"].asInt();
                    }) == 2);
            BEAST_EXPECT(
                s1.visitor(
                    s1,  // to const
                    2,
                    [](Json::Value const& v, auto) {
                        return v["value"].asInt();
                    }) == 3);

            // Test passing of additional arguments
            BEAST_EXPECT(
                s1.visitor(
                    s1,
                    std::integral_constant<unsigned, 2>{},
                    [](Json::Value& v, auto ver, auto a1, auto a2) {
                        return ver * a1 * a2 * v["value"].asInt();
                    },
                    5,
                    7) == 2 * 5 * 7 * 3);
            BEAST_EXPECT(
                s1.visitor(
                    s1,
                    std::integral_constant<unsigned, 2>{},
                    [](Json::Value& v, auto ver, auto... args) {
                        return ver * (1 * ... * args) * v["value"].asInt();
                    },
                    5,
                    7) == 2 * 5 * 7 * 3);

            // Several overloads we want to fail
            static_assert([](auto&& v) {
                return !requires
                {
                    v.visitor(
                        v,
                        1,                           //
                        [](Json::Value&, auto) {});  // missing const
                };
            }(std::as_const(s1)));

            static_assert([](auto&& v) {
                return !requires
                {
                    v.visitor(
                        std::move(v),  // cannot bind rvalue
                        1,
                        [](Json::Value&, auto) {});
                };
            }(s1));

            static_assert([](auto&& v) {
                return !requires
                {
                    v.visitor(
                        v,
                        1,         //
                        []() {});  // missing parameter
                };
            }(s1));

            static_assert([](auto&& v) {
                return !requires
                {
                    v.visitor(
                        v,
                        1,             //
                        [](auto) {});  // missing parameter
                };
            }(s1));

            static_assert([](auto&& v) {
                return !requires
                {
                    v.visitor(
                        v,
                        1,                     //
                        [](Json::Value&) {});  // missing parameter
                };
            }(s1));

            static_assert([](auto&& v) {
                return !requires
                {
                    v.visitor(
                        v,
                        1,                               //
                        [](Json::Value&, int, int) {});  // too many parameters
                };
            }(s1));

            // Want these to be unambiguous
            static_assert([](auto&& v) {
                return requires
                {
                    v.visitor(v, 1, [](auto...) {});
                };
            }(s1));

            static_assert([](auto&& v) {
                return requires
                {
                    v.visitor(v, 1, [](auto, auto...) {});
                };
            }(s1));

            static_assert([](auto&& v) {
                return requires
                {
                    v.visitor(v, 1, [](auto, auto, auto...) {});
                };
            }(s1));

            static_assert([](auto&& v) {
                return requires
                {
                    v.visitor(
                        v, 1, [](auto, auto, auto...) {}, "");
                };
            }(s1));

            static_assert([](auto&& v) {
                return requires
                {
                    v.visitor(
                        v, 1, [](auto, auto, auto, auto...) {}, "");
                };
            }(s1));
        }

        {
            testcase("visit");

            MultivarJson<Helper<3>> s1{};
            s1.val[0] = makeJson("value", 2);
            s1.val[1] = makeJson("value", 3);
            s1.val[2] = makeJson("value", 5);

            // Test different overloads
            static_assert([](auto&& v) {
                return requires
                {
                    v.visit(
                        std::integral_constant<unsigned, 1>{},
                        [](Json::Value&, std::integral_constant<unsigned, 1>) {
                        });
                };
            }(s1));
            BEAST_EXPECT(
                s1.visit(
                    std::integral_constant<unsigned, 1>{},
                    Overload{
                        [](Json::Value& v,
                           std::integral_constant<unsigned, 1>) {
                            return v["value"].asInt();
                        },
                        [](Json::Value const&, auto...) { return 0; },
                        [](auto...) { return 0; }}) == 2);
            static_assert([](auto&& v) {
                return requires
                {
                    v.visit()(
                        std::integral_constant<unsigned, 1>{},
                        [](Json::Value&, std::integral_constant<unsigned, 1>) {
                        });
                };
            }(s1));
            BEAST_EXPECT(
                s1.visit()(
                    std::integral_constant<unsigned, 1>{},
                    Overload{
                        [](Json::Value& v,
                           std::integral_constant<unsigned, 1>) {
                            return v["value"].asInt();
                        },
                        [](Json::Value const&, auto...) { return 0; },
                        [](auto...) { return 0; }}) == 2);

            static_assert([](auto&& v) {
                return requires
                {
                    v.visit(
                        std::integral_constant<unsigned, 1>{},
                        [](Json::Value const&,
                           std::integral_constant<unsigned, 1>) {});
                };
            }(std::as_const(s1)));
            BEAST_EXPECT(
                std::as_const(s1).visit(
                    std::integral_constant<unsigned, 2>{},
                    Overload{
                        [](Json::Value const& v,
                           std::integral_constant<unsigned, 2>) {
                            return v["value"].asInt();
                        },
                        [](Json::Value&, auto...) { return 0; },
                        [](auto...) { return 0; }}) == 3);
            static_assert([](auto&& v) {
                return requires
                {
                    v.visit()(
                        std::integral_constant<unsigned, 1>{},
                        [](Json::Value const&,
                           std::integral_constant<unsigned, 1>) {});
                };
            }(std::as_const(s1)));
            BEAST_EXPECT(
                std::as_const(s1).visit()(
                    std::integral_constant<unsigned, 2>{},
                    Overload{
                        [](Json::Value const& v,
                           std::integral_constant<unsigned, 2>) {
                            return v["value"].asInt();
                        },
                        [](Json::Value&, auto...) { return 0; },
                        [](auto...) { return 0; }}) == 3);

            static_assert([](auto&& v) {
                return requires
                {
                    v.visit(1, [](Json::Value&, unsigned) {});
                };
            }(s1));
            BEAST_EXPECT(
                s1.visit(
                    3,
                    Overload{
                        [](Json::Value& v, unsigned) {
                            return v["value"].asInt();
                        },
                        [](Json::Value const&, auto...) { return 0; },
                        [](auto...) { return 0; }}) == 5);
            static_assert([](auto&& v) {
                return requires
                {
                    v.visit()(1, [](Json::Value&, unsigned) {});
                };
            }(s1));
            BEAST_EXPECT(
                s1.visit()(
                    3,
                    Overload{
                        [](Json::Value& v, unsigned) {
                            return v["value"].asInt();
                        },
                        [](Json::Value const&, auto...) { return 0; },
                        [](auto...) { return 0; }}) == 5);

            static_assert([](auto&& v) {
                return requires
                {
                    v.visit(1, [](Json::Value const&, unsigned) {});
                };
            }(std::as_const(s1)));
            BEAST_EXPECT(
                std::as_const(s1).visit(
                    2,
                    Overload{
                        [](Json::Value const& v, unsigned) {
                            return v["value"].asInt();
                        },
                        [](Json::Value&, auto...) { return 0; },
                        [](auto...) { return 0; }}) == 3);
            static_assert([](auto&& v) {
                return requires
                {
                    v.visit()(1, [](Json::Value const&, unsigned) {});
                };
            }(std::as_const(s1)));
            BEAST_EXPECT(
                std::as_const(s1).visit()(
                    2,
                    Overload{
                        [](Json::Value const& v, unsigned) {
                            return v["value"].asInt();
                        },
                        [](Json::Value&, auto...) { return 0; },
                        [](auto...) { return 0; }}) == 3);

            // Cannot bind rvalues, const or not
            static_assert([](auto&& v) {
                return !requires
                {
                    std::forward<decltype(v)>(v).visit(
                        1, [](Json::Value&&, auto) {});
                };
            }(std::move(s1)));
            static_assert([](auto&& v) {
                return !requires
                {
                    std::forward<decltype(v)>(v).visit()(
                        1, [](Json::Value&&, auto) {});
                };
            }(std::move(s1)));
            static_assert([](auto&& v) {
                return !requires
                {
                    std::forward<decltype(v)>(v).visit(
                        1, [](Json::Value const&&, auto) {});
                };
            }(std::move(std::as_const(s1))));
            static_assert([](auto&& v) {
                return !requires
                {
                    std::forward<decltype(v)>(v).visit()(
                        1, [](Json::Value const&&, auto) {});
                };
            }(std::move(std::as_const(s1))));

            // Missing const
            static_assert([](auto&& v) {
                return !requires
                {
                    std::forward<decltype(v)>(v).visit(
                        1, [](Json::Value&, auto) {});
                };
            }(std::as_const(s1)));
            static_assert([](auto&& v) {
                return !requires
                {
                    std::forward<decltype(v)>(v).visit()(
                        1, [](Json::Value&, auto) {});
                };
            }(std::as_const(s1)));

            // Missing parameter
            static_assert([](auto&& v) {
                return !requires
                {
                    std::forward<decltype(v)>(v).visit(1, []() {});
                };
            }(s1));
            static_assert([](auto&& v) {
                return !requires
                {
                    std::forward<decltype(v)>(v).visit()(1, []() {});
                };
            }(s1));
            static_assert([](auto&& v) {
                return !requires
                {
                    std::forward<decltype(v)>(v).visit(1, [](auto) {});
                };
            }(s1));
            static_assert([](auto&& v) {
                return !requires
                {
                    std::forward<decltype(v)>(v).visit()(1, [](auto) {});
                };
            }(s1));
            static_assert([](auto&& v) {
                return !requires
                {
                    std::forward<decltype(v)>(v).visit(1, [](Json::Value&) {});
                };
            }(s1));
            static_assert([](auto&& v) {
                return !requires
                {
                    std::forward<decltype(v)>(v).visit()(
                        1, [](Json::Value&) {});
                };
            }(s1));

            // Sanity checks
            static_assert([](auto&& v) {
                return requires
                {
                    std::forward<decltype(v)>(v).visit(1, [](auto...) {});
                };
            }(std::as_const(s1)));
            static_assert([](auto&& v) {
                return requires
                {
                    std::forward<decltype(v)>(v).visit()(1, [](auto...) {});
                };
            }(std::as_const(s1)));
            static_assert([](auto&& v) {
                return requires
                {
                    std::forward<decltype(v)>(v).visit(
                        1, [](Json::Value&, auto) {});
                };
            }(s1));
            static_assert([](auto&& v) {
                return requires
                {
                    std::forward<decltype(v)>(v).visit()(
                        1, [](Json::Value&, auto) {});
                };
            }(s1));
        }
    }
};

BEAST_DEFINE_TESTSUITE(MultivarJson, ripple_basics, ripple);

}  // namespace test
}  // namespace ripple
