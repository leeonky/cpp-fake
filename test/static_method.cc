#include "CppUTest/TestHarness.h"

#include <unistd.h>
#include <sys/mman.h>
#include <cstring>

class ClassWithStaticMethod {
    public:
    static bool bool_() {
        return false;
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
    void mock(bool(*original)(), bool(*newFun)()) {
        void* originalAddress = (void*)(original);
        void* newAddress = (void*)(newFun);

        long pageSize = sysconf(_SC_PAGESIZE);
        void* pageStart = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(originalAddress) & ~(pageSize - 1));

        if (mprotect(pageStart, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
            perror("mprotect");
            exit(-1);
        }

        intptr_t offset = reinterpret_cast<char*>(newAddress) - reinterpret_cast<char*>(originalAddress) - 5;

        this->originalAddress = originalAddress;
        this->command = *reinterpret_cast<char*>(originalAddress);
        this->offset = *reinterpret_cast<intptr_t*>(reinterpret_cast<char*>(originalAddress) + 1);

        *reinterpret_cast<char*>(originalAddress) = 0xE9;
        *reinterpret_cast<intptr_t*>(reinterpret_cast<char*>(originalAddress) + 1) = offset;
        return;
    }

    ~Mocker() {
        *reinterpret_cast<char*>(originalAddress) = this->command;
        *reinterpret_cast<intptr_t*>(reinterpret_cast<char*>(originalAddress) + 1) = this->offset;
    }

private:
//    std::map<void*, std::vector<char>> backup;
    void *originalAddress;
    char command;
    intptr_t offset;
};

TEST_GROUP(StaticMethod) {
};

TEST(StaticMethod, mock_bool_void_by_short_jmp_and_back_to_original_when_mocker_destroyed) {
    {
        Mocker mocker;
        mocker.mock(&ClassWithStaticMethod::bool_, &return_true);

        CHECK_TRUE(ClassWithStaticMethod::bool_());
    }

    CHECK_FALSE(ClassWithStaticMethod::bool_());
}
