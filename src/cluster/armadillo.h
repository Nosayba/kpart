#pragma once

//#define ARMA_DONT_USE_CXX11
//#include <armadillo>
#define NOMINMAX
#include </usr/include/armadillo2/armadillo>

// Local specializations
typedef arma::Col<uint64_t> u64vec;
typedef arma::Col<uint32_t> u32vec;
typedef arma::vec dblvec;
typedef arma::Mat<uint64_t> u64mat;
typedef arma::Mat<uint32_t> u32mat;
typedef arma::Mat<double> dblmat;
typedef arma::Cube<uint64_t> u64cube;
typedef arma::Cube<uint32_t> u32cube;
typedef arma::Cube<double> dblcube;
