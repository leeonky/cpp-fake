#include "CppUTest/TestHarness.h"

#include <unistd.h>
#include <sys/mman.h>
#include <cstring>
#include "mocker.h"

class ClassWithInstanceMethod {
    public:
    bool bool_() {
        return false;
    }
};

bool return_true(ClassWithInstanceMethod *instance) {
    return true;
}

TEST_GROUP(InstanceMethod) {
};

TEST(InstanceMethod, non_virtual) {
    ClassWithInstanceMethod instance;
    {
        Mocker mocker(&ClassWithInstanceMethod::bool_, &return_true);

        CHECK_TRUE(instance.bool_());
    }

    CHECK_FALSE(instance.bool_());
}
