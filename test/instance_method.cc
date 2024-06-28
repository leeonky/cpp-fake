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
    bool bool_true() {
        return true;
    }
};

bool return_true(ClassWithInstanceMethod *instance) {
printf("%p\n", instance);
    return true;
}

TEST_GROUP(InstanceMethod) {
};



TEST(InstanceMethod, xxx) {
    ClassWithInstanceMethod instance;
    printf("%p\n", &instance);
    {
        Mocker mocker(&ClassWithInstanceMethod::bool_, &return_true);

        CHECK_TRUE(instance.bool_());
    }

    CHECK_FALSE(instance.bool_());
}
