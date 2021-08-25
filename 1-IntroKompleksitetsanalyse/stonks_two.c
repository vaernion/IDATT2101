#include <stdio.h>

void calc_values(int start, int flux[], int n, int values[]);
void stonks_n1(int arr[], int n, int *buy, int *sell);

int main()
{
    int start = 0;
    int flux[] = {-2, 3, 7, 10, -14, 7};
    int size = sizeof(flux) / sizeof(flux[0]);
    int values[size];

    flux_to_val(start, flux, size, values);

    int buy = -1;
    int sell = -1;

    stonks_n1(values, size, &buy, &sell);

    printf("buy on %d\n", buy);
    printf("sell on %i\n", sell);

    return 0;
}

void flux_to_val(int start, int flux[], int n, int values[])
{
    int curr = start;

    for (int i = 0; i < n; i++)
    {
        curr += flux[i];
        values[i] = curr;
    }
}

void stonks_n1(int arr[], int n, int *buy, int *sell)
{
    int maxProfit = -1;
    int maxRight = arr[n - 1];
    *sell = n - 1;

    for (int i = n - 2; i >= 0; i--)
    {
        if (arr[i] > maxRight)
        {
            maxRight = arr[i];
            *sell = i;
        }
        else
        {
            int profit = maxRight - arr[i];
            if (profit > maxProfit)
            {
                maxProfit = profit;
                *buy = i;
            }
        }
    }
}