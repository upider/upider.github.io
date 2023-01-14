#include <iostream>
#include <vector>

using namespace std;

void merge_sort(vector<int> &nums, int l, int r)
{
    if (l >= r)
        return;

    int mid = l + r >> 1;
    merge_sort(nums, l, mid);
    merge_sort(nums, mid + 1, r);

    int k = 0, i = l, j = mid + 1;
    vector<int> tmp = vector<int>(r - l + 1);
    while (i <= mid && j <= r)
    {
        if (nums[i] <= nums[j])
        {
            tmp[k++] = nums[i++];
        }
        else
        {
            tmp[k++] = nums[j++];
        }
    }

    while (i <= mid)
    {
        tmp[k++] = nums[i++];
    }
    while (j <= r)
    {
        tmp[k++] = nums[j++];
    }

    for (i = l, j = 0; i <= r; i++, j++)
    {
        nums[i] = tmp[j];
    }
}

int main()
{
    vector<int> nums = vector<int>{99, 21, 33, 4, 10, 4, 34, 9};
    merge_sort(nums, 0, 8);
    for (auto x : nums)
    {
        std::cout << x << std::endl;
    }
    return 0;
}