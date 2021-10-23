//
// Created by reza011 on 7/1/2019.
//
#include <math.h>
#include "topsis.hpp"
#include <iostream>

using namespace std;

/**
 * \brief
 * @param parameters is base structure of inputs
 * @return
 */


float** Topsis::normalization (float** array, int rows, int col)
{

    float** norm = new float*[255];


    for (int i = 0; i < rows; i++)
    {
        norm[i] = new float[255];


        for (int j = 0; j < col; j++)
        {

            //برای هر درایه
            // for each norm[i,j]
            //که این ماتریس
            //norm
            // در نهایت ماتریس نرمالی هست که باید پر شود

            float normalNumber = 0;

            //                    if (j == 0)
            //                  {

            // جمع یک ستون ماتریس
            double jazrSigma = 0;
            // مجموع توان 2 درایه های هر ستون
            for (int c = 0; c < rows; c++)
            {
                jazrSigma += array[c][j] * array[c][j];

            }

            //                   }
            // که در نهایت میشه درایه متناظر در ماتریس اصلی تقسیم بر
            // رادیکالِ مجموع توان 2 درایه های ستون نظیر
            normalNumber = (array[i][j] / (float)sqrt(jazrSigma));

            // مقدار گرفتن درایه ماتریس نرمال
            norm[i][j] = normalNumber;
        }
    }
    return norm;
}

/**
 * \brief this is matrix for weight
 * @param weightArr
 * @param col
 * @return
 */
float** Topsis::weightMatrix(float weightArr[], int col)
{
    float** m=new float*[255];

    for (int i = 0; i < col; i++)
    {
        m[i] = new float[255];
        for (int j = 0; j < col; j++)
        {
            if (i == j) m[i][j] = weightArr[j];
            else m[i][j] = 0;
        }
    }

    return m;
}

/**
 * \brief For multiplex wight in rows
 * @param norm
 * @param row1
 * @param col1
 * @param weightMatx
 * @param row2
 * @param col2
 * @return
 */
float** Topsis::multiplexToWeight(float** norm, int row1, int col1, float** weightMatx, int row2, int col2)
{
    //row1 = row1 - 2;
    //row2 = row2 - 2;
    //col1 = col1 - 1;
    //col2 = col2 - 1;
    float** c=new float*[256];
    for (int i = 0; i < row1; i++)
    {
        c[i] = new float[255];
        for (int j = 0; j < col2; j++)
        {
            c[i][j] = 0;
            for (int k = 0; k <col1; k++)
            {
                c[i][j] += norm[i][k] * weightMatx[k][j];
            }
        }
    }

    return c;
}

/**
 * \brief
 * @param multiWeightMatrix
 * @param row
 * @param col
 * @param type
 * @return
 */
float* Topsis::idealPositive(float** multiWeightMatrix, int row, int col, char type[])
{
    // row = row;
    // col = col;
    float* ap=new float[256];
    for (int j = 0; j < col; j++)
    {

        if (type[j] == '+')
        {
            float max = multiWeightMatrix[0][j];

            for (int i = 0; i < row; i++)
            {
                max = fmax(multiWeightMatrix[i][j], max);
            }
            ap[j] = max;
        }
        else
        {
            float min = multiWeightMatrix[0][j];

            for (int i = 0; i < row; i++)
            {
                min = fmin(multiWeightMatrix[i][j], min);
            }
            ap[j] = min;

        }
    }


    return ap;
}

/**
 * \brief
 * @param multiWeightMatrix
 * @param row
 * @param col
 * @param type
 * @return
 */
float* Topsis::idealNegative(float**  multiWeightMatrix, int row, int col, char type[])
{
    //row = row - 2;
    //col = col - 1;
    float* am=new float[256];
    for (int j = 0; j < col; j++)
    {

        if(type[j] == '-')
        {
            float max = multiWeightMatrix[0][j];
            for (int i = 0; i < row; i++)
            {
                max = fmax(multiWeightMatrix[i][j], max);
            }
            am[j] = max;
        }
        else
        {
            float min = multiWeightMatrix[0][j];
            for (int i = 0; i < row; i++)
            {
                min = fmin(multiWeightMatrix[i][j], min);
            }
            am[j] = min;


        }
    }

    return am;
}

/**
 * \brief
 * @param multiWeightMatrix
 * @param row
 * @param col
 * @param idealPos
 * @return
 */
float* Topsis::distanceFromPositive(float** multiWeightMatrix, int row, int col, float idealPos[])
{
    //row = row - 2;
    //col = col - 1;
    float* diP=new float[256];


    for (int i = 0; i < row; i++)
    {
        diP[i] = 0;
        double temp = 0;
        for (int j = 0; j < col; j++)
        {
            temp = temp + (fabs(static_cast<double>((multiWeightMatrix[i][j] - idealPos[j])))* fabs(static_cast<double>((multiWeightMatrix[i][j] - idealPos[j]))));

        }
        diP[i] = (float)sqrt(temp);
    }


    return diP;
}

/**
 * \brief
 * @param multiWeightMatrix
 * @param row
 * @param col
 * @param idealNeg
 * @return
 */
float* Topsis::distanceFromNegative(float** multiWeightMatrix, int row, int col, float idealNeg[256])
{

    //row = row - 2;
    //col = col - 1;
    float* diP=new float[256];


    for (int i = 0; i < row; i++)
    {
        diP[i] = 0;
        double temp = 0;
        for (int j = 0; j < col; j++)
        {
            temp = temp + (fabs(static_cast<double>((multiWeightMatrix[i][j] - idealNeg[j])))* fabs(static_cast<double>((multiWeightMatrix[i][j] - idealNeg[j]))));

        }
        diP[i] = (float)sqrt(temp);
    }


    return diP;
}

/**
 * \brief
 * @param disNeg
 * @param disPos
 * @param count
 * @return
 */
float* Topsis::finalRank(float disNeg[], float disPos[], int count)
{

    float* c=new float[256];

    for (int i = 0; i < count; i++)
    {
        c[i] = disNeg[i] / (disNeg[i] + disPos[i]);
    }

    return c;
}

int Topsis::result(float** arr, int rows) {

    int col = 4;
    float weightArray[256] = { 0.25,0.25,0.25,0.25};
    char typeValue[256] = { '-', '+', '-', '+'};
    cout << "نرمال سازی:\n";
    float **norm= normalization(arr,rows,col);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < col; ++j) {
            cout << norm[i][j] << "  ";
        }
        cout << endl;
    }

    float **weightMatx = weightMatrix(weightArray, col);

    cout << "\n" << "وزن دهی به ماتریس" << "\n";
    float **multiWeightMatrix = multiplexToWeight(norm, rows, col, weightMatx, col, col);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < col; ++j) {
            cout << multiWeightMatrix[i][j] << "  ";
        }
        cout << endl;
    }

    cout << "\n" << "ایدآل مثبت" << "\n";
    float* aplus = idealPositive(multiWeightMatrix, rows, col, typeValue);
    for (int k = 0; k < col; ++k) {
        cout << aplus[k] << "  ";
    }
    cout << "\n" << "ایدآل منفی" << "\n";
    float* aminus = idealNegative(multiWeightMatrix, rows, col, typeValue);
    for (int k = 0; k < col; ++k) {
        cout << aminus[k] << "  ";
    }

    cout << "\n" << "فاصله تا ایدآل مثبت" << "\n";
    float* disPos = distanceFromPositive(multiWeightMatrix, rows, col, aplus);
    for (int k = 0; k < rows; ++k) {
        cout << disPos[k] << "  ";
    }

    cout << "\n" << "فاصله تا ایدآل منفی" << "\n";
    float* disNeg = distanceFromNegative(multiWeightMatrix, rows, col, aminus);
    for (int k = 0; k < rows; ++k) {
        cout << disNeg[k] << "  ";
    }

    cout << "\n" << "نتیجه نهایی" << "\n";
    float* result = finalRank(disNeg, disPos, rows);
    for (int i = 0; i <rows; i++){
        cout << result[i] << " ";
    }
    float bestValue=0;
    int bestRow;
    for (int l = 0; l < rows; ++l) {
        if(result[l] > bestValue){
            bestRow = l;
            bestValue = result[l];
        }
    }
    return bestRow;
}

