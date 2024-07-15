# cpp-fake
cpp-fake 是一个用于 C++ 单元测试的打桩框架，其目的是在不同的cpp版本和OS上，在不修改被测代码的前提下，替换其依赖的外部方法。cpp-fake 只做一件事情，就是替换依赖方法，不提供依赖方法行为控制和验证。所以，使用时需要结合其他库（如 [cppumock](https://cpputest.github.io/mocking_manual.html) ）一起使用。

cpp-fake 借鉴了 [CppFreeMock](https://github.com/gzc9047/CppFreeMock) 的实现思路，但用更加“原始”的 C++ 语言特性实现，以适应更多的cpp版本。

# 使用说明

使用 cpp-fake 很简单，只需要把`main`目录下的`mocker.h`这个头文件拷贝到你的项目中，然后在测试代码中包含这个头文件即可。然后参考下面的示例，在单元测试中替换依赖的方法即可。更多的示例可以参考`test`目录下的测试用例。

## 替换静态方法

```c++
class ClassWithStaticMethod {
public:
    static bool bool_() {
        return false;
    }
};

bool return_true() {
    return true;
}

TEST(StaticMethod, example) {
    {
        Mocker mocker(&ClassWithStaticMethod::bool_, &return_true, Long);

        CHECK_TRUE(ClassWithStaticMethod::bool_());
    }

    CHECK_FALSE(ClassWithStaticMethod::bool_());
}
```

## 替换自由方法

```c++
bool bool_() {
    return false;
}

bool free_return_true() {
    return true;
}

TEST(FreeMethod, example) {
    {
        Mocker mocker(&bool_, &free_return_true);

        CHECK_TRUE(bool_());
    }

    CHECK_FALSE(bool_());
}
```

## 替换库方法（类似自由方法）

```c++
char *my_strerror(int errnum) {
    static char buffer[128];
    sprintf(buffer, "error-is-%d", errnum);
    return buffer;
}

TEST(StdMethod, example) {
    {
        Mocker mocker(&strerror, &my_strerror);

        STRCMP_EQUAL("error-is-100", strerror(100));
    }

    STRCMP_EQUAL("Network is down", strerror(100));
}
```

## 替换类成员方法

```c++
class ClassWithInstanceMethod {
public:
    bool bool_() {
        return false;
    }
};

bool return_true(ClassWithInstanceMethod *instance) {
    return true;
}

TEST(InstanceMethod, example) {
    ClassWithInstanceMethod instance;
    {
        Mocker mocker(&ClassWithInstanceMethod::bool_, &return_true);

        CHECK_TRUE(instance.bool_());
    }

    CHECK_FALSE(instance.bool_());
}
```

## 替换类重载方法

```c++
class ClassWithInstanceMethod {
public:
    bool overloaded_method() {
        return false;
    }

    bool overloaded_method(uint32_t value) {
        return false;
    }
};

bool overloaded_return_true(ClassWithInstanceMethod *instance, uint32_t value) {
    return true;
}

TEST(InstanceMethod, example) {
    ClassWithInstanceMethod instance;
    {
        bool (ClassWithInstanceMethod::*methodToBeStub)(uint32_t) = &ClassWithInstanceMethod::overloaded_method;
        Mocker mocker(methodToBeStub, &overloaded_return_true);

        CHECK_TRUE(instance.overloaded_method(42));
        CHECK_FALSE(instance.overloaded_method());
    }
    
    CHECK_FALSE(instance.overloaded_method(42));
    CHECK_FALSE(instance.overloaded_method());
}
```

# 支持的环境（x86架构）
* 各种Linux发行版中的c++
* 各种Mac版本中的c++
* 各种Windows版本中的VC++（待验证）

# 使用限制
* 不支持 Arm 架构
* 不支持打桩虚函数(virtual)
* 不支持打桩私有函数(private)
* 不支持打桩构造函数和析构函数