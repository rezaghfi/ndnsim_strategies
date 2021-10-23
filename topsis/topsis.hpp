//
// Created by reza011 on 7/1/2019.
//

#ifndef TOPSIS_TOPSIS_H
#define TOPSIS_TOPSIS_H


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


};


#endif //TOPSIS_TOPSIS_H
