#include "CppUTest/TestHarness.h"

#include <unistd.h>
#include <sys/mman.h>
#include <cstring>

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

#include <map>
#include <vector>
#include <algorithm>

class Mocker {

public:
    Mocker(bool(*original)(), bool(*newFun)()) {
        void* originalAddress = (void*)(original);
        void* newAddress = (void*)(newFun);

        long pageSize = sysconf(_SC_PAGESIZE);
        pageStart = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(originalAddress) & ~(pageSize - 1));

        if (mprotect(pageStart, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
            perror("mprotect");
            exit(-1);
        }

        intptr_t offset = reinterpret_cast<char*>(newAddress) - reinterpret_cast<char*>(originalAddress) - 2;

        if (offset < -128 || offset > 127) {
            offset = reinterpret_cast<char*>(newAddress) - reinterpret_cast<char*>(originalAddress) - 5;
            this->style = 2;
            this->originalAddress = originalAddress;
            this->command = *reinterpret_cast<char*>(originalAddress);
            this->offset = *reinterpret_cast<intptr_t*>(reinterpret_cast<char*>(originalAddress) + 1);

            *reinterpret_cast<char*>(originalAddress) = 0xE9;
            *reinterpret_cast<intptr_t*>(reinterpret_cast<char*>(originalAddress) + 1) = offset;
        } else {
            this->style = 1;

            this->originalAddress = originalAddress;
            this->command = *reinterpret_cast<char*>(originalAddress);
            this->offset_8 = *(reinterpret_cast<char*>(originalAddress) + 1);

            *reinterpret_cast<char*>(originalAddress) = 0xEB;
            *(reinterpret_cast<char*>(originalAddress) + 1) = (unsigned char)offset;
        }

        if (mprotect(pageStart, pageSize, PROT_READ | PROT_EXEC) != 0) {
            perror("mprotect");
            exit(-1);
        }
    }

    ~Mocker() {
        long pageSize = sysconf(_SC_PAGESIZE);
        if (mprotect(pageStart, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
            perror("mprotect");
            exit(-1);
        }
        switch(this->style) {
            case 1:
                *reinterpret_cast<char*>(originalAddress) = this->command;
                *(reinterpret_cast<char*>(originalAddress) + 1) = this->offset_8;
                break;
            default:
                *reinterpret_cast<char*>(originalAddress) = this->command;
                *reinterpret_cast<intptr_t*>(reinterpret_cast<char*>(originalAddress) + 1) = this->offset;
                break;
        }
        if (mprotect(pageStart, pageSize, PROT_READ | PROT_EXEC) != 0) {
            perror("mprotect");
            exit(-1);
        }
    }

private:
    int style;
    void *originalAddress;
    char command;
    intptr_t offset;
    char offset_8;
    void *pageStart;
};

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
