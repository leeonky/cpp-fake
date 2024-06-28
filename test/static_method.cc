#include "CppUTest/TestHarness.h"

#include <unistd.h>
#include <sys/mman.h>
#include <cstring>
#include "mocker.h"

class ClassWithStaticMethod {
    public:
    static bool bool_() {
        return false;
    }
    static bool bool_true() {
        return true;
    }
};

bool return_true() {
    return true;
}

TEST_GROUP(StaticMethod) {
};

TEST(StaticMethod, mock_bool_void_by_long_jmp_and_back_to_original_when_mocker_destroyed) {
    {
        Mocker mocker(&ClassWithStaticMethod::bool_, &return_true);

        CHECK_TRUE(ClassWithStaticMethod::bool_());
    }

    CHECK_FALSE(ClassWithStaticMethod::bool_());
}

TEST(StaticMethod, mock_bool_void_by_short_jmp_and_back_to_original_when_mocker_destroyed) {
    {
        Mocker mocker(&ClassWithStaticMethod::bool_, &ClassWithStaticMethod::bool_true);

        CHECK_TRUE(ClassWithStaticMethod::bool_());
    }

    CHECK_FALSE(ClassWithStaticMethod::bool_());
}
