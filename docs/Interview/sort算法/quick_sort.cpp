#include<iostream>

using namespace std;

// 递归
void quick_sort(vector<int> &arr, int l, int r)
{
    if (l < r)
    {
        //Swap(s[l], s[(l + r) / 2]); //将中间的这个数和第一个数交换 参见注1
        int i = l, j = r, x = arr[l];
        while (i < j)
        {
            while(i < j && arr[j] >= x) // 从右向左找第一个小于x的数
                j--;  
            if(i < j) 
                arr[i++] = arr[j];
            
            while(i < j && arr[i] < x) // 从左向右找第一个大于等于x的数
                i++;  
            if(i < j) 
                arr[j--] = arr[i];
        }
        arr[i] = x;
        quick_sort(arr, l, i - 1); // 递归调用 
        quick_sort(arr, i + 1, r);
    }
}

// 迭代
void quick_sort_iterative(vector<int> &arr)
{
    /*
        快速排序本质上是二叉树的前序遍历，可以使用栈来模拟
    */
    stack<pair<int, int>> stk;
    int end = arr.size() - 1;
    stk.emplace(0, end);
    while (!stk.empty())
    {
        pair<int, int> cur = stk.top();
        stk.pop();
        // 下面就是直接复制递归的实现了
        int start = cur.first;
        int end = cur.second;
        if (start >= end)
            continue;
        int base = arr[start];
        int left = start, right = end;
        while (left < right)
        {
            // while结束条件
            while (left < right && arr[right] >= base)
            {
                right--; // 从右往左找到第一个比base小的数
            }
            while (left < right && arr[left] <= base)
            {
                left++; // 从左往右找到第一个比base大的数
            }
            swap(arr[left], arr[right]); // 交换
        }
        swap(arr[start], arr[left]); // 最后left==right，该位置的数一定是小于等于base的数
		// 模拟递归
        stk.emplace(left+1,end);
        stk.emplace(start,left-1);
    }
}

int main()
{
    int nums[] {10, 4,5, 19, 10};
    quick_sort(nums, 0, 5);
    for(auto x : nums) {
        std::cout << x << std::endl;
    }
    return 0;
}
