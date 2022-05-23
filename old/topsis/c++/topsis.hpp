//
// Created by reza011 on 7/1/2019.
//

#ifndef TOPSIS_TOPSIS_H
#define TOPSIS_TOPSIS_H

/**
 *
 */
struct Parameter{
public:
    float array [256][256]= { {1, 2, 3, 4}, { 2,3,4,5 }, { 3,4,5,6 }};
    int col = 4;
    int rows = 3;
    float weightArray[256] = { 1,2,3,4};
    char typeValue[256] = { '+', '+', '+', '+'};
};

/**
 *
 */
class Topsis {

public:

    static float **normalization(float** arr, int rows, int col);

    static float **weightMatrix(float *weightArr, int col);

    static float **multiplexToWeight(float **norm, int row1, int col1, float **weightMatx, int row2, int col2);

    static float *idealPositive(float **multiWeightMatrix, int row, int col, char *type);

    static float *idealNegative(float **multiWeightMatrix, int row, int col, char *type);

    static float *distanceFromPositive(float **multiWeightMatrix, int row, int col, float *idealPos);

    static float *distanceFromNegative(float **multiWeightMatrix, int row, int col, float *idealNeg);

    static float *finalRank(float *disNeg, float *disPos, int count);

    static int result(float** array, int rows);

};


#endif //TOPSIS_TOPSIS_H
