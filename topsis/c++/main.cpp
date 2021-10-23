// ConsoleApplication1.cpp : Defines the entry point for the console application.
//
#include "topsis.hpp"
#include <iostream>

using namespace std;


int main()
{
    int rows = 4;
    float** arr = new float*[255];
    arr[0] = new float[4];
    arr[0][0] = {120};
    arr[0][1] = {76};
    arr[0][2] = {98};
    arr[0][3] = {87.5};
    arr[1] = new float[4];
    arr[1][0] = {43};
    arr[1][1] = {987};
    arr[1][2] = {965};
    arr[1][3] = {12.8};
    arr[2] = new float[4];
    arr[2][0] = {237};
    arr[2][1] = {908};
    arr[2][2] = {43};
    arr[2][3] = {54};
    arr[3] = new float[4];
    arr[3][0] = {78.9};
    arr[3][1] = {324};
    arr[3][2] = {12};
    arr[3][3] = {19};

    int bestRow = Topsis::result(arr,rows);
    cout << "\n" << "best nexthops id is:" << bestRow;
    return 0;
}

