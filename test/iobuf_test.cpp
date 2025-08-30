#include "catch.hpp"

#include <stdio.h>
#include <string.h>

#include "IOBuf.h"


TEST_CASE("case1", "[raw interface]") {
    // set up
    IOBuf buf;
    int def_count = 1024;
    CHECK(0 == buf.readable_bytes());
    CHECK(def_count == buf.writable_bytes());
    REQUIRE(0 != buf.has_readed(1));

    int count = 7;
    REQUIRE(0 == buf.ensure_writable(count));
    CHECK(def_count == buf.writable_bytes());

    REQUIRE(0 == buf.has_written(count));
    CHECK(count == buf.readable_bytes());
    CHECK((def_count - count) == buf.writable_bytes());

    REQUIRE(0 != buf.has_readed(count * 2));
    REQUIRE(0 == buf.has_readed(3));
    CHECK((count - 3) == buf.readable_bytes());
    CHECK((def_count - count) == buf.writable_bytes());

    REQUIRE(0 != buf.has_readed(count));
    REQUIRE(0 == buf.has_readed(4));
    CHECK(0 == buf.readable_bytes());
    CHECK((def_count - count) == buf.writable_bytes());

    count = 2048;
    REQUIRE(0 == buf.ensure_writable(count));
    CHECK(count == buf.writable_bytes());

    REQUIRE(0 == buf.has_written(count));
    CHECK(count == buf.readable_bytes());
    CHECK(0 == buf.writable_bytes());

    REQUIRE(0 == buf.has_readed(1024));
    CHECK(1024 == buf.readable_bytes());
    CHECK(0 == buf.writable_bytes());

    REQUIRE(0 != buf.has_readed(count));

    // remain 1024 readable bytes
    count = 3072;
    REQUIRE(0 == buf.ensure_writable(count));
    REQUIRE(0 == buf.has_readed(buf.readable_bytes())); // buf.has_readall()
    CHECK(0 == buf.readable_bytes());
    CHECK(count == buf.writable_bytes());

    REQUIRE(0 == buf.has_written(1024));
    CHECK(1024 == buf.readable_bytes());
    CHECK(2048 == buf.writable_bytes());

    REQUIRE(0 == buf.has_written(1024));
    CHECK(2048 == buf.readable_bytes());
    CHECK(1024 == buf.writable_bytes());

    REQUIRE(0 == buf.has_written(1024));
    CHECK(count == buf.readable_bytes());
    CHECK(0 == buf.writable_bytes());

    REQUIRE(0 == buf.has_readed(count));

    count = 4096;
    REQUIRE(0 == buf.ensure_writable(count));
    CHECK(count == buf.writable_bytes());

    REQUIRE(0 == buf.has_written(1024));
    CHECK(1024 == buf.readable_bytes());
    CHECK(3072 == buf.writable_bytes());

    REQUIRE(0 == buf.has_written(1024));
    CHECK(2048 == buf.readable_bytes());
    CHECK(2048 == buf.writable_bytes());

    REQUIRE(0 == buf.has_written(1024));
    CHECK(3072 == buf.readable_bytes());
    CHECK(1024 == buf.writable_bytes());

    REQUIRE(0 == buf.has_written(1024));
    CHECK(count == buf.readable_bytes());
    CHECK(0 == buf.writable_bytes());

    REQUIRE(0 == buf.has_readed(count));

    count = 5120;
    REQUIRE(0 != buf.ensure_writable(count));

    CHECK(0 == buf.writable_bytes());
    REQUIRE(0 == buf.ensure_writable(4096));
    CHECK(4096 == buf.writable_bytes());

    REQUIRE(0 == buf.has_written(4096));
    REQUIRE(0 == buf.has_readed(4096));
    CHECK(0 == buf.writable_bytes());
    CHECK(0 == buf.readable_bytes());

    // different sections

    // tear down

}

TEST_CASE("case2", "[r/w interface]") {
    // set up
    IOBuf buf;
    int def_count = 1024;
    CHECK(0 == buf.readable_bytes());
    CHECK(def_count == buf.writable_bytes());
    REQUIRE(0 != buf.has_readed(1));

    auto uptr = std::make_unique<char[]>(1024);

    REQUIRE(0 == buf.write("mediacore", 9));
    CHECK(1015 == buf.writable_bytes());
    CHECK(9 == buf.readable_bytes());

    REQUIRE(0 == buf.read(uptr.get(), 5));
    CHECK(0 == strncmp(uptr.get(), "media", 5));
    CHECK(1015 == buf.writable_bytes());
    CHECK(4 == buf.readable_bytes());

    REQUIRE(0 == buf.read(uptr.get() + 5, 4));
    CHECK(0 == strncmp(uptr.get() + 5, "core", 4));
    CHECK(1015 == buf.writable_bytes());
    CHECK(0 == buf.readable_bytes());

    REQUIRE(0 != buf.read(uptr.get(), 9));

    REQUIRE(0 == buf.write(uptr.get(), 9));
    CHECK(1006 == buf.writable_bytes()); // write 9 twice, remain 1006
    CHECK(9 == buf.readable_bytes());

    // REQUIRE(0 == buf.ensure_writable(def_count)); // extend to 2k
    REQUIRE(0 == buf.write(uptr.get(), def_count)); // extend to 2k
    CHECK(1015 == buf.writable_bytes());
    CHECK(1033 == buf.readable_bytes());

    // different sections

    // tear down

}
