# c++中的volatile

### volatile官方解释

#### volatile对象

类型为 *volatile-限定的* 对象，或 volatile 对象的子对象，或 const volatile 对象的 mutable 子对象。通过 volatile 限定的类型的泛左值表达式的每次访问（读或写操作、成员函数调用等），都被当作对于[优化](https://zh.cppreference.com/w/cpp/language/as_if)而言是可见的副作用（即在单个执行线程内，volatile 访问不能被优化掉，或者与另一[按顺序早于](https://zh.cppreference.com/w/cpp/language/eval_order)或按顺序晚于该 volatile 访问的可见副作用进行重排序。这使得 volatile 对象适用于与[信号处理函数](https://zh.cppreference.com/w/cpp/utility/program/signal)之间的交流，但不适于与另一执行线程交流，参阅 [std::memory_order](https://zh.cppreference.com/w/cpp/atomic/memory_order)）。试图通过非 volatile [泛左值](https://zh.cppreference.com/w/cpp/language/value_category#.E6.B3.9B.E5.B7.A6.E5.80.BC)涉指 volatile 对象（例如，通过到非 volatile 类型的引用或指针）会导致未定义行为。

#### 要点

1. 在单个执行线程内，volatile 访问不能被优化掉
2. volatile 对象适用于与[信号处理函数](https://zh.cppreference.com/w/cpp/utility/program/signal)之间的交流
3. 不适于与另一执行线程交流
4. 线程间交流使用[std::memory_order](https://zh.cppreference.com/w/cpp/atomic/memory_order)

### 与Java的不同

Java中的volatile主要是为了不同线程间提供可见性，而c++ 中的volatile并没有这个作用。Java的volatile还会禁止指令重排，这个作用在c++  中是有的。下面的代码在c++中1和2没有不同，但在Java中会有不同结果。

```cpp
#include <iostream>
#include <thread>
#include <unistd.h>

class Test{
	public:
		volatile int n = 0; //1
    	//int n = 0; //2
};

int main(void)
{
        Test test;

        std::cout << "start a new thread" << std::endl;
        std::thread t([&](){
        	std::cout << "changing n" << std::endl;
        	sleep(1);
        	test.n += 99;
        });

        while(test.n == 0){}
        std::cout << "n = "<< test.n << std::endl;

        t.join();

        return 0;
}
```

### 使用例子

下面的例子在O2优化的时候不使用volatile，ctrl+c信号会被优化掉，也就是编译器看不到对gSignalStatus的修改，将while循环优化成了`while(true)`；这也就是官方文档说的“volatile 对象适用于与[信号处理函数](https://zh.cppreference.com/w/cpp/utility/program/signal)之间的交流”。

```cpp
#include <csignal>
#include <iostream>

namespace
{
        //volatile std::sig_atomic_t gSignalStatus = 0;
        std::sig_atomic_t gSignalStatus = 0;
}

void signal_handler(int signal)
{
        gSignalStatus = signal;
        std::cout << "Sending signal " << signal << '\n';
        std::cout << "SignalValue: " << gSignalStatus << '\n';
}

int main()
{
        // 安装信号处理函数
        std::signal(SIGINT, signal_handler);

        while(gSignalStatus == 0){

        }
}
```

### 例外

在微软平台，也就是windows上，volatile被增强了，变得和Java的volatile一致。

不过，可以使用C++11之中的原子变量避免编译器过度优化。