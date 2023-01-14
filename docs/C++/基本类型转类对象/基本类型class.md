# 基本类型class

```c++
#include<iostream>

using namespace std;

enum UID : uint64_t{};

int main()
{
    UID uid(100);
    
    std::cout << uid << std::endl;
    return 0;
}
```