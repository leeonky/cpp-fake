#include "CppUTest/TestHarness.h"

//#include <unistd.h>
//#include <sys/mman.h>
#include <cstring>
#include "mocker.h"

bool bool_() {
    return false;
}

namespace nm {
    bool bool_() {
        return false;
    }
}

bool free_return_true() {
    return true;
}

TEST_GROUP(FreeMethod) {
};

TEST(FreeMethod, mock_cxx_global_method) {
    {
        Mocker mocker(&bool_, &free_return_true);

        CHECK_TRUE(bool_());
    }

    CHECK_FALSE(bool_());
}

TEST(FreeMethod, mock_cxx_namespace_method) {
    {
        Mocker mocker(&nm::bool_, &free_return_true);

        CHECK_TRUE(nm::bool_());
    }

    CHECK_FALSE(nm::bool_());
}

char *my_strerror(int errnum) {
    static char buffer[128];
    sprintf(buffer, "error-is-%d", errnum);
    return buffer;
}

TEST(FreeMethod, mock_system_call) {
    {
        Mocker mocker(&strerror, &my_strerror);

        STRCMP_EQUAL("error-is-100", strerror(100));
    }

    STRCMP_EQUAL("Network is down", strerror(100));
}
