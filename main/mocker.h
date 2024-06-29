#ifndef MOCKER_H
#define MOCKER_H

#include <stdexcept>
#include <stdint.h>
#include <cstdio>
#include <cstring>

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

    long long offset(void *newAddress) {
        return reinterpret_cast<long long>(newAddress) - reinterpret_cast<long long>(originalAddress) - size;
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
        intptr_t offset = reinterpret_cast<char*>(newAddress) - reinterpret_cast<char*>(originalAddress) - 2;
        if (offset >= -128 && offset <= 127) {
            return Short;
        } else {
            return Long;
        }
        return Long64;
    }

    Jumper *jumper;
};

#endif
