//
// Created by reza011 on 7/1/2019.
//
#include <math.h>
#include "topsis.hpp"
#include <iostream>

using namespace std;

/**
 * \brief it is normalization of input array
 * @param parameters is base structure of inputs
 * @return normalization array[][]
 */

float** Topsis::normalization (float** array, int rows, int col)
{
    float** norm = new float*[255];
    for (int i = 0; i < rows; i++){

        norm[i] = new float[255];
        for (int j = 0; j < col; j++){

            float normalNumber = 0;
            double jazrSigma = 0;
            for (int c = 0; c < rows; c++){

                jazrSigma += array[c][j] * array[c][j];
            }
            normalNumber = (array[i][j] / (float)sqrt(jazrSigma));
            norm[i][j] = normalNumber;
        }
    }
    return norm;
}

/**
 * \brief this is matrix for weight
 * @param weightArr
 * @param col
 * @return weightMatrix[]
 */
float** Topsis::weightMatrix(float weightArr[], int col){
    float** m=new float*[255];

    for (int i = 0; i < col; i++){

        m[i] = new float[255];
        for (int j = 0; j < col; j++){

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
 * @return multiplex array
 */
float** Topsis::multiplexToWeight(float** norm, int row1, int col1, float** weightMatx, int row2, int col2){

    float** c=new float*[256];
    for (int i = 0; i < row1; i++){

        c[i] = new float[255];
        for (int j = 0; j < col2; j++){

            c[i][j] = 0;
            for (int k = 0; k <col1; k++){

                c[i][j] += norm[i][k] * weightMatx[k][j];
            }
        }
    }

    return c;
}

/**
 * \brief for positive ideal
 * @param multiWeightMatrix
 * @param row
 * @param col
 * @param type
 * @return
 */
float* Topsis::idealPositive(float** multiWeightMatrix, int row, int col, char type[]){

    float* ap=new float[256];
    for (int j = 0; j < col; j++){

        if (type[j] == '+'){
            float max = multiWeightMatrix[0][j];
            for (int i = 0; i < row; i++){

                max = fmax(multiWeightMatrix[i][j], max);
            }
            ap[j] = max;
        }
        else{
            float min = multiWeightMatrix[0][j];

            for (int i = 0; i < row; i++){

                min = fmin(multiWeightMatrix[i][j], min);
            }
            ap[j] = min;
        }
    }
    return ap;
}

/**
 * \brief for positive ideal
 * @param multiWeightMatrix
 * @param row
 * @param col
 * @param type
 * @return
 */
float* Topsis::idealNegative(float**  multiWeightMatrix, int row, int col, char type[]){

    float* am=new float[256];
    for (int j = 0; j < col; j++){

        if(type[j] == '-'){
            float max = multiWeightMatrix[0][j];
            for (int i = 0; i < row; i++){

                max = fmax(multiWeightMatrix[i][j], max);
            }
            am[j] = max;
        }
        else{
            float min = multiWeightMatrix[0][j];
            for (int i = 0; i < row; i++){

                min = fmin(multiWeightMatrix[i][j], min);
            }
            am[j] = min;
        }
    }
    return am;
}

/**
 * \brief distance from positive ideal
 * @param multiWeightMatrix
 * @param row
 * @param col
 * @param idealPos
 * @return dip
 */
float* Topsis::distanceFromPositive(float** multiWeightMatrix, int row, int col, float idealPos[]){

    float* posDis=new float[256];
    for (int i = 0; i < row; i++){

        posDis[i] = 0;
        double temp = 0;
        for (int j = 0; j < col; j++){

            temp = temp + (fabs(static_cast<double>((multiWeightMatrix[i][j] - idealPos[j])))* fabs(static_cast<double>((multiWeightMatrix[i][j] - idealPos[j]))));
        }
        posDis[i] = (float)sqrt(temp);
    }
    return posDis;
}

/**
 * \brief distance from Negative ideal
 * @param multiWeightMatrix
 * @param row
 * @param col
 * @param idealNeg
 * @return
 */
float* Topsis::distanceFromNegative(float** multiWeightMatrix, int row, int col, float idealNeg[256]){

    float* negDis=new float[256];
    for (int i = 0; i < row; i++){

        negDis[i] = 0;
        double temp = 0;
        for (int j = 0; j < col; j++){

            temp = temp + (fabs(static_cast<double>((multiWeightMatrix[i][j] - idealNeg[j])))* fabs(static_cast<double>((multiWeightMatrix[i][j] - idealNeg[j]))));
        }
        negDis[i] = (float)sqrt(temp);
    }
    return negDis;
}

/**
 * \brief final ranking of Negative distances and positive distances
 * @param disNeg
 * @param disPos
 * @param count
 * @return array of ranking values
 */
float* Topsis::finalRank(float disNeg[], float disPos[], int count){

    float* c=new float[256];

    for (int i = 0; i < count; i++){

    	if(disNeg[i]==0){
    		c[i] = 0;
    	}else{
        c[i] = disNeg[i] / (disNeg[i] + disPos[i]);
    	}
    }
    return c;
}


