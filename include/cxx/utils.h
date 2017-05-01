#pragma once

#include <cxx/types.h>

namespace cxx {

inline std::string format_vector(const Vector& v)
{
  size_t n = v.size();
  std::ostringstream os;
  for (size_t i = 0; i < n; ++i)
    os << v(i) << " ";
  return os.str();
}

inline std::string format_matrix(const SparseMatrix& smat)
{
  Matrix mat(smat);
  int n = int(mat.rows()), m = int(mat.cols());
  std::ostringstream os;
  for (int i = 0; i < n;++i)
  {
    for (int j = 0; j < m;++j)
    {
      double d=mat(i, j);
      os << d << ' ';
    }
    os << ";" << std::endl;
  }
  return os.str();
}


inline Matrix3 get_x_rotation_matrix(double a)
{
  Matrix3 R;
  R <<
    1, 0, 0,
    0, cos(a), sin(a),
    0, -sin(a), cos(a);
  return R;
}

inline Matrix3 get_y_rotation_matrix(double a)
{
  Matrix3 R;
  R <<
    cos(a), 0, -sin(a),
    0, 1, 0,
    sin(a), 0, cos(a);
  return R;
}

inline Matrix3 get_z_rotation_matrix(double a)
{
  Matrix3 R;
  R <<
    cos(a), sin(a), 0,
    -sin(a), cos(a), 0,
    0, 0, 1;
  return R;
}


inline Matrix3 get_rotation_matrix(double rx, double ry, double rz)
{
  Matrix3 RX = get_x_rotation_matrix(rx);
  Matrix3 RY = get_y_rotation_matrix(ry);
  Matrix3 RZ = get_z_rotation_matrix(rz);
  // Composed rotation matrix with (RX, RY, RZ)
  Matrix3 R = RX * RY * RZ;
  return R;
}

inline Matrix3 get_rotation_matrix(const Vector3& a)
{
  return get_rotation_matrix(a(0), a(1), a(2));
}

inline Matrix extract_euler_angles(const Matrix& R)
{
  const double pi = 3.1415926535897932384626433832795;
  Matrix res(3, 1);
  double threshold = 1.0e-5;
  if ((1-abs(R(0,2))) < threshold)
  {
    res(1) = R(0, 2) > 0 ? -0.5*pi : 0.5*pi;
    res(2) = 0;
    res(0) = atan2(-R(2, 1), R(1, 1));
  }
  else
  {
    res(0) = atan2(R(1, 2), R(2, 2));
    double c2 = sqrt(sqr(R(0, 0)) + sqr(R(0, 1)));
    res(1) = atan2(-R(0, 2), c2);
    res(2) = atan2(R(0, 1), R(0, 0));
  }
  return res;
}


} // namespace cxx


