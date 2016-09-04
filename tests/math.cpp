/*
MIT License

Copyright (c) 2016 Patrick Lafferty

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "catch.hpp"

#include "../Math.h"

using namespace Math;

TEST_CASE("normalize", "[math]") {
    SECTION("zero vector normalized should still be zero") {
        Vector3 zero{};
        zero.normalize();

        REQUIRE(zero.x == 0.f);
        REQUIRE(zero.y == 0.f);
        REQUIRE(zero.z == 0.f);
    }

    SECTION("non-zero vector normalized should have length 1") {
        Vector3 v{3.f, 5.f, 7.f};
        v.normalize();

        REQUIRE(lengthSquared(v) == 1.f);
    }
}

TEST_CASE("vector -=", "[math]") {
    SECTION("v -= zeroVec should be v") {
        Vector3 v{3.f, 5.f, 7.f};
        auto copy = v;
        v -= Vector3{};

        REQUIRE(v.x == copy.x);
        REQUIRE(v.y == copy.y);
        REQUIRE(v.z == copy.z);
    }

    SECTION("v -= t should change v if t nonzero") {
        Vector3 v{3.5f, 12.99f, 6.91f};
        v -= Vector3{2.f, 1.9f, 30.f};

        REQUIRE(v.x == 3.5f - 2.f);
        REQUIRE(v.y == 12.99f - 1.9f);
        REQUIRE(v.z == 6.91f - 30.f);
    }
}

TEST_CASE("vector operator+", "[math]") {
    SECTION("v + zeroVec == v") {
        Vector3 v{1.f, 2.f, 3.f};
        auto result = v + Vector3{};

        REQUIRE(result.x == v.x);
        REQUIRE(result.y == v.y);
        REQUIRE(result.z == v.z);
    }

    SECTION("v + t != v") {
        Vector3 v{1.f, 2.f, 3.f};
        auto result = v + Vector3{2.f, 3.f, 4.f};

        REQUIRE(result.x == 3.f);
        REQUIRE(result.y == 5.f);
        REQUIRE(result.z == 7.f);
    }
} 

TEST_CASE("vector operator-", "[math]") {
    SECTION("v - zeroVec == v") {
        Vector3 v{1.f, 2.f, 3.f};
        auto result = v - Vector3{};

        REQUIRE(result.x == v.x);
        REQUIRE(result.y == v.y);
        REQUIRE(result.z == v.z);
    }

    SECTION("v - t != v") {
        Vector3 v{1.f, 2.f, 3.f};
        auto result = v - Vector3{2.f, 3.f, 4.f};

        REQUIRE(result.x == -1.f);
        REQUIRE(result.y == -1.f);
        REQUIRE(result.z == -1.f);
    }
} 

TEST_CASE("vector scaling", "[math]") {
    SECTION("v * s = {xs, ys, zs}") {
        Vector3 v{1.f, 2.f, 3.f};
        auto result = v * 13.37f;

        REQUIRE(result.x == 1.f * 13.37f);
        REQUIRE(result.y == 2.f * 13.37f);
        REQUIRE(result.z == 3.f * 13.37f);
    }
} 

TEST_CASE("lengthSquared", "[math]") {
    Vector3 v{1.f, 2.f, 3.f};
    REQUIRE(lengthSquared(v) == 14.f);
}

TEST_CASE("distanceSquared", "[math]") {
    SECTION("distanceSquared(v, zeroVec) == lengthSquared(v)") {
        Vector3 v{1.f, 2.f, 3.f};
        REQUIRE(distanceSquared(v, {}) == lengthSquared(v));
    }

    SECTION("non-zero distance") {
        Vector3 v{1.f, 2.f, 3.f};
        Vector3 t{0.f, 2.f, 4.f};
        REQUIRE(distanceSquared(v, t) == 2.f);
    }
}

TEST_CASE("dot", "[math]") {
    SECTION("dot(v, v) == lengthSquared(v)") {
        Vector3 v{3.f, 5.f, 7.f};
        REQUIRE(dot(v, v) == lengthSquared(v));
    }

    SECTION("non-zero dot") {
        Vector3 v{3.f, 4.f, 5.f};
        Vector3 t{6.f, 7.f, 8.f};
        
        REQUIRE(dot(v, t) == (3.f * 6.f + 4.f * 7.f + 5.f * 8.f));
    }
}