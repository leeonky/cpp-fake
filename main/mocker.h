#ifndef MOCKER_H
#define MOCKER_H

#include <stdexcept>
#include <stdint.h>
#include <cstdio>

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
    virtual ~Jumper() {}
};

class ShortJumper : public Jumper{
public:
    ShortJumper(void *original, void *newAddress): originalAddress(original){
        intptr_t offset = reinterpret_cast<char*>(newAddress) - reinterpret_cast<char*>(originalAddress) - 2;
        this->command = *reinterpret_cast<char*>(originalAddress);
        this->offset_8 = *(reinterpret_cast<char*>(originalAddress) + 1);

        PageWriteable write(originalAddress);
        *reinterpret_cast<char*>(originalAddress) = 0xEB;
        *(reinterpret_cast<char*>(originalAddress) + 1) = (unsigned char)offset;
    }
    ~ShortJumper() {
        PageWriteable write(originalAddress);
        *reinterpret_cast<char*>(originalAddress) = this->command;
        *(reinterpret_cast<char*>(originalAddress) + 1) = this->offset_8;
    }

private:
    void *originalAddress;
    char command;
    char offset_8;
};

enum JumpPolicy {
    Short=1,
    Long=2,
    Long64=3,
    Auto
};

class LongJumper : public Jumper{
public:
    LongJumper(void *original, void *newAddress): originalAddress(original){
        intptr_t offset = reinterpret_cast<char*>(newAddress) - reinterpret_cast<char*>(originalAddress) - 5;
        this->command = *reinterpret_cast<char*>(originalAddress);
        this->offset = *reinterpret_cast<intptr_t*>(reinterpret_cast<char*>(originalAddress) + 1);
        PageWriteable write(originalAddress);
        *reinterpret_cast<char*>(originalAddress) = 0xE9;
        *reinterpret_cast<intptr_t*>(reinterpret_cast<char*>(originalAddress) + 1) = offset;
    }

    ~LongJumper() {
        PageWriteable write(originalAddress);
        *reinterpret_cast<char*>(originalAddress) = this->command;
        *reinterpret_cast<intptr_t*>(reinterpret_cast<char*>(originalAddress) + 1) = this->offset;
    }

    void *originalAddress;
    char command;
    intptr_t offset;
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
