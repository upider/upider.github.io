# c++ 类型转换

## static_cast

格式：`static_cast<type>(expression)`

任何编写程序时能够明确的类型转换都可以使用static_cast（static_cast不能转换掉底层const，volatile和__unaligned属性）。由于不提供运行时的检查，所以叫static_cast，因此，需要在编写程序时确认转换的安全性。

主要在以下几种场合中使用：

    用于类层次结构中，父类和子类之间指针和引用的转换；进行上行转换，把子类对象的指针/引用转换为父类指针/引用，这种转换是安全的；进行下行转换，把父类对象的指针/引用转换成子类指针/引用，这种转换是不安全的，需要编写程序时来确认；用于基本数据类型之间的转换，例如把int转char，int转enum等，需要编写程序时来确认安全性；把void指针转换成目标类型的指针（这是极其不安全的）；

示例：
```c++
int i, j;
double slope = static_cast<double>(j) / i;
void *p = &d;
double *dp = static_cast<double*>(p);
```

## dynamic_cast

格式：`dynamic_cast<type>(expression)`

相比static_cast，dynamic_cast会在运行时检查类型转换是否合法，具有一定的安全性.
dynamic_cast转换仅适用于指针或引用。

## const_cast

格式：`const_cast<type>(expression)`

const_cast用于移除类型的const、volatile和__unaligned属性。

## reinterpret_cast

格式：`reinterpret_cast<type>(expression)`

非常激进的指针类型转换，在编译期完成，可以转换任何类型的指针，所以极不安全