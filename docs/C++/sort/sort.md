# sort

## sort函数
- sort()函数是C++标准库中的排序函数, 头文件为algorithm, 时间复杂度为n*log2(n)
- 函数原型
```
void sort(RandomIt first, RandomIt last, Compare comp);
```
- 默认比较器std::less, 即元素升序排列