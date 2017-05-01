#pragma once

#include <cxx/errors.h>
#include <cxx/prims.h>
#include <assert.h>

#include <boost/math/constants/constants.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>

namespace cxx {


  class Polynomial
  {
  public:
    typedef boost::multiprecision::cpp_dec_float_50 real;
    //typedef double real;
  private:
    typedef std::vector<real> coef_vec;

    // Coefficients from low rank (0) to high (degree)
    coef_vec m_Coefficients;
  public:
    Polynomial(real scalar = 0) : m_Coefficients(1, scalar) {}

    explicit Polynomial(size_t degree, real default_value)
      : m_Coefficients(degree + 1, default_value)
    {}

    size_t degree() const { return m_Coefficients.size() - 1; }

    double evaluate(double x) const
    {
      real sum = 0;
      //real xp=1;
      for (size_t i = 0; i <= degree(); ++i)
      {
        real term = m_Coefficients[i];
        for(size_t j=0;j<i;++j)
          term*=x;
        sum+=term;
        //sum += xp * m_Coefficients[i];
        //xp*=x;
      }
      return double(sum);
    }

    real& operator[] (size_t index)
    {
      assert(index < m_Coefficients.size());
      return m_Coefficients[index];
    }

    real operator[] (size_t index) const
    {
      assert(index < m_Coefficients.size());
      return m_Coefficients[index];
    }

    Polynomial derivative() const
    {
      Polynomial res(degree() - 1, 0);
      for (size_t i = 0; i < degree(); ++i)
        res[i] = (i + 1)*m_Coefficients[i + 1];
      return res;
    }
  };

  inline std::ostream& operator<< (std::ostream& os, const Polynomial& p)
  {
    size_t d = p.degree();
    for (size_t i = d; i <= d; --i)
    {
      if (i < d || p[i]<0) os << (p[i] < 0 ? " - " : " + ");
      if (i==0 || (i>0 && fabs(p[i])!=1)) os << fabs(p[i]);
      if (i > 0)
        os << "*x";
      if (i > 1) os << "^" << i;
    }
    return os;
  }

  inline Polynomial operator* (Polynomial::real scalar, const Polynomial& p)
  {
    Polynomial res = p;
    for (size_t i = 0; i <= p.degree(); ++i) res[i] *= scalar;
    return res;
  }

  inline Polynomial operator* (const Polynomial& p, Polynomial::real scalar)
  {
    return scalar*p;
  }

  inline Polynomial operator/ (const Polynomial& p, Polynomial::real scalar)
  {
    return (1.0 / scalar)*p;
  }

  inline Polynomial operator+ (const Polynomial& p, Polynomial::real scalar)
  {
    Polynomial res = p;
    res[0] += scalar;
    return res;
  }

  inline Polynomial operator+ (Polynomial::real scalar, const Polynomial& p)
  {
    return p + scalar;
  }

  inline Polynomial operator- (const Polynomial& p, Polynomial::real scalar)
  {
    scalar = -scalar;
    return p + scalar;
  }

  inline Polynomial operator- (Polynomial::real scalar, const Polynomial& p)
  {
    return scalar + (-1.0 * p);
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

  // Important note: with C++ operator precedence, this operator does not have a high priority,
  //                 so typical expressions such as    a*b^2  would be interpreted as:  (a*b)^2
  //                 When using this operator, use parentheses to avoid problems.
  //                                                   a*(b^2)
  inline Polynomial operator^ (const Polynomial& p, int power)
  {
    if (power < 0) THROW_ERROR("Unsupported power: " << power);
    if (power == 0) return Polynomial(1.0);
    if (power == 1) return p;
    Polynomial res = p;
    while (--power > 0)
      res = res*p;
    return res;
  }

} // namespace cxx
