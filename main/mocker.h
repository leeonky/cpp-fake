#ifndef MOCKER_H
#define MOCKER_H

#include <stdexcept>
#include <stdint.h>
#include <cstdio>
#include <cstring>
#include <sys/mman.h>

class PageWriteable {
public:
    PageWriteable(void *target) {
        long pageSize = sysconf(_SC_PAGESIZE);
        pageStart = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(target) & ~(pageSize - 1));

        if (mprotect(pageStart, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
            perror("mprotect");
            throw std::runtime_error("Change memory writeable failed!");
        }
    }

    ~PageWriteable() {
        long pageSize = sysconf(_SC_PAGESIZE);
        if (mprotect(pageStart, pageSize, PROT_READ | PROT_EXEC) != 0) {
            perror("mprotect");
            exit(-1);
        }
    }

private:
    void *pageStart;
};

class Jumper {
public:
    Jumper(void *original, int s): originalAddress(original), size(s) {
        index = 0;
        memcpy(backupCode, originalAddress, size);
    }
    virtual ~Jumper() {
        PageWriteable write(originalAddress);
        memcpy(originalAddress, backupCode, size);
    }

protected:
    void *originalAddress;
    char backupCode[32];
    int size, index;

    uintptr_t offset(void *newAddress) {
        return reinterpret_cast<uintptr_t>(newAddress) - reinterpret_cast<uintptr_t>(originalAddress) - size;
    }

    void patch(char c) {
        reinterpret_cast<char*>(originalAddress)[index++] = c;
    }

    void patch4(int32_t i) {
        *reinterpret_cast<int32_t*>(reinterpret_cast<char*>(originalAddress)+index) = i;
        index+=4;
    }
};

class ShortJumper : public Jumper{
public:
    ShortJumper(void *original, void *newAddress): Jumper(original, 2) {
        PageWriteable write(original);
        patch(0xEB);
        patch(offset(newAddress));
    }
};

class LongJumper : public Jumper{
public:
    LongJumper(void *original, void *newAddress): Jumper(original, 5){
        PageWriteable write(original);
        patch(0xE9);
        patch4(offset(newAddress));
    }
};

class Long64Jumper : public Jumper{
public:
    Long64Jumper(void *original, void *newAddress): Jumper(original, 14){
        PageWriteable write(original);
#ifdef __x86_64__
        patch(0xFF);
        patch(0x25);
        patch4(0);
        patch4((int64_t)newAddress);
        patch4(((int64_t)newAddress)>>32);
#endif
    }
};

enum JumpPolicy {
    Short=1,
    Long=2,
    Long64=3,
    Auto
};

class Mocker {
public:
    template<typename F1, typename F2>
    Mocker(F1 original, F2 newFun, JumpPolicy policy = Auto) {
        setJump(*(void**)&original, (void*)newFun, policy);
    }

    ~Mocker() {
        delete jumper;
    }

private:
    void setJump(void *originalAddress, void *newAddress, JumpPolicy support) {
        switch(guessPolicy(originalAddress, newAddress, support)){
            case Short:
                    jumper = new ShortJumper(originalAddress, newAddress);
                break;
            case Long:
                    jumper = new LongJumper(originalAddress, newAddress);
                break;
            case Long64:
                    jumper = new Long64Jumper(originalAddress, newAddress);
                break;
            default:
                throw std::runtime_error("Cannot hook method!");
                break;
        }
    }

    JumpPolicy guessPolicy(void *originalAddress, void *newAddress, JumpPolicy support) {
        JumpPolicy policy = guessPolicyByOffset(originalAddress, newAddress);
        if(policy > support)
            throw std::runtime_error("Cannot use given jump policy!");
        if(support != Auto)
            return support;
        return policy;
    }

    JumpPolicy guessPolicyByOffset(void *originalAddress, void *newAddress) {
        long long offset = reinterpret_cast<long>(newAddress) - reinterpret_cast<long>(originalAddress) - 2;
        if (offset >= -128 && offset <= 127)
            return Short;
        offset = reinterpret_cast<long>(newAddress) - reinterpret_cast<long>(originalAddress) - 5;
        if (offset >= -2147483648LL && offset <= 2147483647LL)
            return Long;
        return Long64;
    }

    Jumper *jumper;
};

#endif
