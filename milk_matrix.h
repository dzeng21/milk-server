#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include "milk_server.h"

#pragma once

class Matrix {
public:
    Matrix() : rows(0), cols(0) {
        matrix.reserve(64);
    }

    void verify(const std::string &s);
    void construct(const std::string &s);
    void multiply(Matrix &A, Matrix &B, Matrix &C, ThreadPool &t);
    void deconstruct(std::string &output);
    friend std::ostream& operator<<(std::ostream& os, const Matrix& m);
private:
    std::vector<double> matrix;
    size_t rows;
    size_t cols;
    std::string ctor;

    double get(int i, int j) const { return matrix[i * cols + j]; }
    void set(int i, int j, int value) { matrix[i * cols + j] = value; }
    void add(int i, int j, int value) { matrix[i * cols + j] += value; }
    void multiply_part(Matrix &A, Matrix &B, Matrix &C, size_t start, size_t end);
};

void Matrix::construct(const std::string &s) {
    Matrix* temp = nullptr;
    
    if (this->rows || this->cols) {
        temp = new Matrix();
        *temp = *this;
        *this = Matrix();
    }
    else {
        temp = new Matrix();
    }

    std::invalid_argument arg_error = std::invalid_argument("matrix format [1 2 3; 4 5 6; 7 8 9]...");

    if (s.empty() || s.front() != '[' || s.back() != ']') {
        *this = *temp;
        delete temp;
        throw arg_error;
    }

    std::string input = s.substr(1, s.size() - 2);

    std::stringstream iss(input);

    std::string row;

    //split matrix into rows by ;
    while (std::getline(iss, row, ';')) {
        std::vector<double> row_data;
        std::stringstream rowstream(row);
        std::string val;
        
        //split matrix rows into values
        while (rowstream >> val) {
            try {
                row_data.push_back(std::stod(val));
            }
            catch (const std::invalid_argument &e) {
                *this = *temp;
                delete temp;
                throw arg_error;
            }
        }

        if (row_data.empty()) {
            continue;
        }

        //check of input rows are same size
        if (this->cols == 0) {
            this->cols = row_data.size();
        }
        else {
            if (row_data.size() != this->cols) {
                *this = *temp;
                delete temp;
                throw arg_error;
            }
        }

        this->matrix.insert(matrix.end(), row_data.begin(), row_data.end());

        this->rows++;
    }

    delete temp;
}

void Matrix::multiply(Matrix &A, Matrix &B, Matrix &C, ThreadPool &t) {
    if (&A == &C || &B == &C) {
        throw std::invalid_argument("C cannot be A or B");
    }

    if (!A.rows || !A.cols) {
        throw std::invalid_argument("A is uninitialized");
    }

    if (!B.rows || !B.cols) {
        throw std::invalid_argument("B is uninitialized");
    }

    if (A.cols != B.rows) {
        throw std::invalid_argument("matrix dimension mismatch");
    }

    C = Matrix();
    C.rows = A.rows;
    C.cols = B.cols;
    C.matrix.assign(C.rows * C.cols, 0.0);

    size_t part_size = A.rows / t.thread_count();

    for (size_t thread = 0; thread < t.thread_count(); ++thread) {
        size_t start = thread * part_size;
        size_t end = (thread == 7) ? A.rows : start + part_size;
        t.enqueue([this, &A, &B, &C, start, end]() {
            multiply_part(A, B, C, start, end);
        });
    }
}

void Matrix::multiply_part(Matrix &A, Matrix &B, Matrix &C, size_t start, size_t end) {
    size_t n = B.rows;
    size_t p = B.cols;
    for (size_t i = start; i < end; ++i)
        for (size_t j = 0; j < p; ++j)
            for (size_t k = 0; k < n; ++k)
                C.add(i, j, (A.get(i, k) * B.get(k, j)));
}

void Matrix::deconstruct(std::string &output) {
    std::stringstream oss;
    oss << "[";
    for (size_t i = 0; i < this->rows; i++) {
        for (size_t j = 0; j < this->cols; j++) {
            oss << " " << this->get(i, j);
        }
        oss << ";";
    }
    oss << "]";
    output = oss.str();
}

std::ostream& operator<<(std::ostream& os, const Matrix& m) {
    os << "\n";
    for (size_t i = 0; i < m.rows; i++) {
        os << "        ";
        for (size_t j = 0; j < m.cols; j++) {
            os << std::setw(20) << std::fixed << std::setprecision(4);
            os << m.get(i, j);
        }
        os << "\n";
    }
    os << "\n";
    return os;
}