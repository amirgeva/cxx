#pragma once

#include <cxx/errors.h>

namespace cxx {

  class Polynomial
  {
    typedef std::vector<double> coef_vec;

    // Coefficients from low rank (0) to high (degree)
    coef_vec m_Coefficients;
  public:
    Polynomial(double scalar = 0) : m_Coefficients(1, scalar) {}

    explicit Polynomial(size_t degree, double default_value)
      : m_Coefficients(degree + 1, default_value)
    {}

    size_t degree() const { return m_Coefficients.size() - 1; }

    double& operator[] (size_t index)
    {
      assert(index < m_Coefficients.size());
      return m_Coefficients[index];
    }

    double operator[] (size_t index) const
    {
      assert(index < m_Coefficients.size());
      return m_Coefficients[index];
    }
  };

  inline std::ostream& operator<< (std::ostream& os, const Polynomial& p)
  {
    size_t d = p.degree();
    for (size_t i = d; i <= d; --i)
    {
      if (i < d) os << (p[i] < 0 ? " - " : " + ");
      os << fabs(p[i]);
      if (i > 0)
        os << "X";
      if (i > 1) os << "^" << i;
    }
    return os;
  }

  inline Polynomial operator* (double scalar, const Polynomial& p)
  {
    Polynomial res = p;
    for (size_t i = 0; i <= p.degree(); ++i) res[i] *= scalar;
    return res;
  }

  inline Polynomial operator* (const Polynomial& p, double scalar)
  {
    return scalar*p;
  }

  inline Polynomial operator/ (const Polynomial& p, double scalar)
  {
    return (1.0 / scalar)*p;
  }

  inline Polynomial operator+ (const Polynomial& p, double scalar)
  {
    Polynomial res = p;
    res[0] += scalar;
    return res;
  }

  inline Polynomial operator+ (double scalar, const Polynomial& p)
  {
    return p + scalar;
  }

  inline Polynomial operator- (const Polynomial& p, double scalar)
  {
    scalar = -scalar;
    return p + scalar;
  }

  inline Polynomial operator- (double scalar, const Polynomial& p)
  {
    return -scalar + p;
  }

  inline Polynomial operator* (const Polynomial& a, const Polynomial& b)
  {
    size_t da = a.degree(), db = b.degree();
    Polynomial res(da + db, 0.0);
    for (size_t i = 0; i <= da; ++i)
    {
      for (size_t j = 0; j <= db; ++j)
      {
        res[i + j] += a[i] * b[j];
      }
    }
    return res;
  }

  inline Polynomial operator- (const Polynomial& a, const Polynomial& b)
  {
    size_t da = a.degree(), db = b.degree();
    size_t dr = Max(da, db);
    Polynomial res(dr, 0.0);
    for (size_t i = 0; i <= da; ++i) res[i] += a[i];
    for (size_t i = 0; i <= db; ++i) res[i] -= b[i];
    return res;
  }

  inline Polynomial operator+ (const Polynomial& a, const Polynomial& b)
  {
    size_t da = a.degree(), db = b.degree();
    size_t dr = Max(da, db);
    Polynomial res(dr, 0.0);
    for (size_t i = 0; i <= da; ++i) res[i] += a[i];
    for (size_t i = 0; i <= db; ++i) res[i] += b[i];
    return res;
  }

  inline Polynomial operator^ (const Polynomial& p, int power)
  {
    if (power < 0) THROW_ERROR("Unsupported power: " << power);
    if (power == 0) return 1;
    if (power == 1) return p;
    Polynomial res = p;
    while (--power > 0)
      res = res*p;
    return res;
  }

} // namespace cxx
