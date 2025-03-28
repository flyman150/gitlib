#include <iostream>
#include <vector>

void quickSort(std::vector<int>& arr, int left, int right) {
    if (left >= right) return;

    int pivot = arr[left];
    int i = left, j = right;

    while (i < j) {
        while (i < j && arr[j] >= pivot) j--;
        if (i < j) arr[i++] = arr[j];

        while (i < j && arr[i] <= pivot) i++;
        if (i < j) arr[j--] = arr[i];
    }

    arr[i] = pivot;

    quickSort(arr, left, i - 1);
    quickSort(arr, i + 1, right);
}

int main() {
    std::vector<int> arr = {3, 6, 8, 10, 1, 2, 1};
    quickSort(arr, 0, arr.size() - 1);

    for (int num : arr) {
        std::cout << num << " ";
    }
    std::cout << std::endl;

    return 0;
}

