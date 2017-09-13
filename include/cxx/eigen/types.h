#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <random>
#include <memory>
#include <string>
#include <sstream>
#include <cxx/xstring.h>
#include <cxx/prims.h>

#include <Eigen/Sparse>
#include <Eigen/Dense>
#include <Eigen/SparseCholesky>
#include <Eigen/SparseQR>
#include <Eigen/StdVector>
#include <Eigen/QR>
#include <Eigen/LU>

namespace cxx {

typedef Eigen::Vector2d FramePoint;
typedef Eigen::Vector3d WorldPoint;
typedef Eigen::Vector3d Vector3;
typedef Eigen::Matrix3d Matrix3;
typedef Eigen::Matrix2d Matrix2;
typedef Eigen::Vector2d Vector2;
typedef Eigen::Vector2i iVector2;

typedef Eigen::MatrixXd Matrix;
typedef Eigen::SparseMatrix<double> SparseMatrix;
typedef Eigen::VectorXd Vector;
typedef std::vector<FramePoint,Eigen::aligned_allocator<FramePoint>> frame_point_vec;
typedef std::vector<WorldPoint,Eigen::aligned_allocator<WorldPoint>> world_point_vec;
typedef std::vector<Matrix, Eigen::aligned_allocator<Matrix>> matrix_vec;
typedef std::vector<Matrix3, Eigen::aligned_allocator<Matrix3>> matrix3_vec;
typedef std::vector<Vector3, Eigen::aligned_allocator<Vector3>> vector3_vec;
typedef std::vector<Vector2, Eigen::aligned_allocator<Vector2>> vector2_vec;
typedef std::vector<int> int_vec;
typedef std::set<int> int_set;
typedef std::set<xstring> str_set;
typedef std::vector<int_vec> int_vec_vec;
typedef std::vector<bool> bool_vec;
typedef std::vector<double> dvec;
typedef std::vector<float> fvec;
//typedef std::vector<float, aligned_allocator<float, 32>> aligned_fvec;
typedef std::vector<xstring> str_vec;
typedef std::vector<uint8_t> status_vec;

class SparseMatrixBuilder
{
  typedef Eigen::Triplet<double> triplet;
  typedef std::vector<triplet> triplets_seq;
  triplets_seq m_Data;
  unsigned m_Width, m_Height;
public:
  typedef SparseMatrixBuilder self;
  typedef triplets_seq::const_iterator const_iterator;
  const_iterator begin() const { return m_Data.begin(); }
  const_iterator end()   const { return m_Data.end(); }

  SparseMatrixBuilder(unsigned initial_capacity=0)
    : m_Width(0), m_Height(0)
  {
    if (initial_capacity > 0)
      m_Data.reserve(initial_capacity);
  }

  void set_minimum_size(unsigned width, unsigned height)
  {
    m_Width = Max(m_Width, width);
    m_Height = Max(m_Height, height);
  }
  
  unsigned get_width() const { return m_Width; }
  unsigned get_height() const { return m_Height; }

  self& operator()(unsigned row, unsigned col, double v)
  {
    add(row, col, v);
    return *this;
  }

  self& operator() (unsigned row, unsigned col, const Matrix& values)
  {
    for (unsigned i = 0; i < unsigned(values.rows()); ++i)
    {
      const auto& row_values = values.row(i);
      for (unsigned j = 0; j < unsigned(values.cols()); ++j)
      {
        m_Height = Max(m_Height, i + row + 1);
        m_Width = Max(m_Width, j + col + 1);
        m_Data.push_back(triplet(i + row, j + col, row_values(j)));
      }
    }
    return *this;
  }

  void add(unsigned row, unsigned col, double value)
  {
    m_Height = Max(m_Height, row + 1);
    m_Width = Max(m_Width, col + 1);
    m_Data.push_back(triplet(row, col, value));
  }

  void generate(SparseMatrix& matrix, bool size_to_fit)
  {
    if (size_to_fit)
      matrix = SparseMatrix(m_Height, m_Width);
    matrix.setFromTriplets(m_Data.begin(), m_Data.end());
  }
};

class SparseMatrixBuilderStream
{
  SparseMatrixBuilder& m_Builder;
  unsigned             m_Row, m_Col, m_Width, m_Current;

  void append(double value)
  {
    m_Builder.add(m_Row, m_Col + m_Current, value);
    if (++m_Current == m_Width)
    {
      m_Current = 0;
      ++m_Row;
    }
  }
public:
  SparseMatrixBuilderStream(SparseMatrixBuilder& smb, unsigned row, unsigned col, unsigned width)
  : m_Builder(smb) 
  , m_Row(row)
  , m_Col(col)
  , m_Width(width)
  , m_Current(0)
  {}

  SparseMatrixBuilderStream& operator<< (double value)
  {
    append(value);
    return *this;
  }

  SparseMatrixBuilderStream& operator, (double value)
  {
    append(value);
    return *this;
  }
};

struct Plane
{
  Plane():N(0,0,0),d(0.0){}
  Plane(const Vector3& n=Vector3(0,0,0), double d=0) : N(n), d(d) {}
  Plane(const Vector3& a, const Vector3& b, const Vector3& c)
  {
    N = (b - a).cross(c - b);
    N.normalize();
    d = a.dot(N);
  }
  Vector3 N;
  double  d;
};

typedef std::vector<Plane,Eigen::aligned_allocator<Plane>> plane_vec;

struct null_output_iterator :
  std::iterator< std::output_iterator_tag,
  null_output_iterator > {
  /* no-op assignment */
  template<typename T>
  void operator=(T const&) { }

  null_output_iterator & operator++() {
    return *this;
  }

  null_output_iterator operator++(int) {
    return *this;
  }

  null_output_iterator & operator*() { return *this; }
};

} // namespace cxx

