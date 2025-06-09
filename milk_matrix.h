
#include <vector>
#include "milk_server.h"

#pragma once

class Matrix {
public:
    Matrix(int rows_, int cols_) : rows(rows_), cols(cols_) {
        matrix.resize(rows * cols);
    }

    int get(int i, int j) {
        return matrix[i * cols + j];
    }

    void set(int i, int j, int value) {
        matrix[i * cols + j] = value;
    }

    void multiply(Matrix &A, Matrix &B, Matrix &C, ThreadPool &t);
private:
    std::vector<int> matrix;
    int rows;
    int cols;

    void multiply_part(Matrix &A, Matrix &B, Matrix &C, size_t start, size_t end);
};

void Matrix::multiply(Matrix &A, Matrix &B, Matrix &C, ThreadPool &t) {
    int part_size = A.rows / 8;

    for (int thread = 0; thread < 8; ++thread) {
        int start = thread * part_size;
        int end = (thread == 7) ? A.rows : start + part_size;
        t.enqueue([this, &A, &B, &C, start, end]() {
            multiply_part(A, B, C, start, end);
        });
    }
}

void Matrix::multiply_part(Matrix &A, Matrix &B, Matrix &C, size_t start, size_t end) {
    int n = B.rows;
    int p = B.cols;
    for (size_t i = start; i < end; ++i)
        for (size_t j = 0; j < p; ++j)
            for (size_t k = 0; k < n; ++k)
                C.set(i, j, (A.get(i, k) * B.get(k, j)));
}