#include "CppUTest/TestHarness.h"

//#include <unistd.h>
//#include <sys/mman.h>
//#include <cstring>
#include "mocker.h"

class ClassWithInstanceMethod {
    public:
    bool bool_() {
        return false;
    }

    bool const_bool() const {
        return false;
    }
};

class SubClassWithInstanceMethod : public ClassWithInstanceMethod {
};


class SubDefineA : public ClassWithInstanceMethod {
    public:
    bool bool_() {
        return false;
    }
};

class SubDefineB {
    public:
    bool bool_() {
        return false;
    }
};

class SubDefineC : public SubDefineA, public SubDefineB {
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

TEST(InstanceMethod, mock_non_virtual) {
    ClassWithInstanceMethod instance;
    {
        Mocker mocker(&ClassWithInstanceMethod::bool_, &return_true);

        CHECK_TRUE(instance.bool_());
    }

    CHECK_FALSE(instance.bool_());
}

TEST(InstanceMethod, mock_non_virtual_const) {
    ClassWithInstanceMethod instance;
    {
        Mocker mocker(&ClassWithInstanceMethod::const_bool, &return_true);

        CHECK_TRUE(instance.const_bool());
    }

    CHECK_FALSE(instance.const_bool());
}

TEST(InstanceMethod, mock_non_virtual_in_sub) {
    SubClassWithInstanceMethod instance;
    {
        Mocker mocker(&ClassWithInstanceMethod::bool_, &return_true);

        CHECK_TRUE(instance.bool_());
    }

    CHECK_FALSE(instance.bool_());
}

TEST(InstanceMethod, mock_non_virtual_const_in_sub) {
    SubClassWithInstanceMethod instance;
    {
        Mocker mocker(&ClassWithInstanceMethod::const_bool, &return_true);

        CHECK_TRUE(instance.const_bool());
    }

    CHECK_FALSE(instance.const_bool());
}

TEST(InstanceMethod, mock_non_virtual_in_sub_get_address_by_sub_class) {
    SubClassWithInstanceMethod instance;
    {
        Mocker mocker(&SubClassWithInstanceMethod::bool_, &return_true);

        CHECK_TRUE(instance.bool_());
    }

    CHECK_FALSE(instance.bool_());
}

TEST(InstanceMethod, mock_non_virtual_const_in_sub_get_address_by_sub_class) {
    SubClassWithInstanceMethod instance;
    {
        Mocker mocker(&SubClassWithInstanceMethod::const_bool, &return_true);

        CHECK_TRUE(instance.const_bool());
    }

    CHECK_FALSE(instance.const_bool());
}

TEST(InstanceMethod, mock_non_virtual_redefined_in_sub) {
    SubDefineA instance;
    {
        Mocker mocker(&ClassWithInstanceMethod::bool_, &return_true);

        CHECK_FALSE(instance.bool_());

        CHECK_TRUE(((ClassWithInstanceMethod)instance).bool_());
    }

    CHECK_FALSE(((ClassWithInstanceMethod)instance).bool_());
    CHECK_FALSE(instance.bool_());

    {
        Mocker mocker(&SubDefineA::bool_, &return_true);

        CHECK_TRUE(instance.bool_());

        CHECK_FALSE(((ClassWithInstanceMethod)instance).bool_());
    }

    CHECK_FALSE(((ClassWithInstanceMethod)instance).bool_());
    CHECK_FALSE(instance.bool_());
}

TEST(InstanceMethod, mock_non_virtual_redefined_in_sub_with_multi_base) {
    SubDefineC instance;
    {
        Mocker mocker(&ClassWithInstanceMethod::bool_, &return_true);

        CHECK_FALSE(instance.bool_());
        CHECK_FALSE(((SubDefineA)instance).bool_());
        CHECK_FALSE(((SubDefineB)instance).bool_());
        CHECK_TRUE(((ClassWithInstanceMethod)instance).bool_());
    }

    CHECK_FALSE(instance.bool_());
    CHECK_FALSE(((SubDefineA)instance).bool_());
    CHECK_FALSE(((SubDefineB)instance).bool_());
    CHECK_FALSE(((ClassWithInstanceMethod)instance).bool_());

    {
        Mocker mocker(&SubDefineA::bool_, &return_true);

        CHECK_FALSE(instance.bool_());
        CHECK_TRUE(((SubDefineA)instance).bool_());
        CHECK_FALSE(((SubDefineB)instance).bool_());
        CHECK_FALSE(((ClassWithInstanceMethod)instance).bool_());
    }

    CHECK_FALSE(instance.bool_());
    CHECK_FALSE(((SubDefineA)instance).bool_());
    CHECK_FALSE(((SubDefineB)instance).bool_());
    CHECK_FALSE(((ClassWithInstanceMethod)instance).bool_());

    {
        Mocker mocker(&SubDefineB::bool_, &return_true);

        CHECK_FALSE(instance.bool_());
        CHECK_FALSE(((SubDefineA)instance).bool_());
        CHECK_TRUE(((SubDefineB)instance).bool_());
        CHECK_FALSE(((ClassWithInstanceMethod)instance).bool_());
    }

    CHECK_FALSE(instance.bool_());
    CHECK_FALSE(((SubDefineA)instance).bool_());
    CHECK_FALSE(((SubDefineB)instance).bool_());
    CHECK_FALSE(((ClassWithInstanceMethod)instance).bool_());

    {
        Mocker mocker(&SubDefineC::bool_, &return_true);

        CHECK_TRUE(instance.bool_());
        CHECK_FALSE(((SubDefineA)instance).bool_());
        CHECK_FALSE(((SubDefineB)instance).bool_());
        CHECK_FALSE(((ClassWithInstanceMethod)instance).bool_());
    }

    CHECK_FALSE(instance.bool_());
    CHECK_FALSE(((SubDefineA)instance).bool_());
    CHECK_FALSE(((SubDefineB)instance).bool_());
    CHECK_FALSE(((ClassWithInstanceMethod)instance).bool_());
}
