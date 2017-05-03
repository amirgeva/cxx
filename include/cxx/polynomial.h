#pragma once

#include <cxx/errors.h>
#include <cxx/prims.h>
#include <assert.h>

// For a high precision scalar, use the following includes:
//    #include <boost/math/constants/constants.hpp>
//    #include <boost/multiprecision/cpp_dec_float.hpp>
// and the following scalar typedef
//    boost::multiprecision::cpp_dec_float_50

namespace cxx {
  
  template<class T=double>
  class Polynomial
  {
  public:
    typedef T real;
  private:
    typedef std::vector<real> coef_vec;
    typedef Polynomial<T>     self;

    // Coefficients from low rank (0) to high (degree)
    coef_vec m_Coefficients;

    void set_degree(size_t d)
    {
      m_Coefficients.resize(d+1,0.0);
    }
  public:
    Polynomial(real scalar = 0.0)
    : m_Coefficients(1, scalar)
    {}

    explicit Polynomial(size_t degree, real default_value)
    : m_Coefficients(degree + 1, default_value)
    {}

    size_t degree() const { return m_Coefficients.size() - 1; }
    
    self& operator+= (const self& rhs)
    {
      if (rhs.degree() > degree()) set_degree(rhs.degree());
      for (size_t i = 0; i <= rhs.degree(); ++i) 
        m_Coefficients[i]+=rhs[i];
      return *this;
    }
    
    self& operator-= (const self& rhs)
    {
      return *this += (-rhs);
    }

    self& operator*= (const self& rhs)
    {
      // Since we have to make a copy of the coeffients anyway,
      // may as well use the binary operator
      *this = *this * rhs;
      return *this;
    }

    double evaluate(double x) const
    {
      real sum = m_Coefficients[0];
      real xp=x;
      for (size_t i = 1; i <= degree(); ++i)
      {
        sum += xp * m_Coefficients[i];
        xp*=x;
      }
      return double(sum);
    }
    
    self operator- () const
    {
      Polynomial res=*this;
      for(size_t i=0; i<=degree(); ++i)
        res[i]=-res[i];
      return res;
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

    self derivative() const
    {
      self res(degree() - 1, 0);
      for (size_t i = 0; i < degree(); ++i)
        res[i] = (i + 1)*m_Coefficients[i + 1];
      return res;
    }
    
    static const self& unit()
    {
      static const self res=self(1,1)-=1;
      return res;
    }
  };

  template<class T>
  inline std::ostream& operator<< (std::ostream& os, const Polynomial<T>& p)
  {
    size_t d = p.degree();
    for (size_t i = d; i <= d; --i)
    {
      if (i < d || p[i]<0) os << (p[i] < 0 ? " - " : " + ");
      if (i==0 || (i>0 && fabs(p[i])!=1)) 
      {
        os << fabs(p[i]);
        if (i > 0)
          os << "*";
      }
      if (i > 0) os << "x";
      if (i > 1) os << "^" << i;
    }
    return os;
  }

  template<class T>
  inline Polynomial<T> operator/ (const Polynomial<T>& p, typename Polynomial<T>::real scalar)
  {
    return (1.0 / scalar)*p;
  }

  template<class T>
  inline Polynomial<T> operator* (const Polynomial<T>& a, const Polynomial<T>& b)
  {
    size_t da = a.degree(), db = b.degree();
    Polynomial<T> res(da + db, 0.0);
    for (size_t i = 0; i <= da; ++i)
    {
      for (size_t j = 0; j <= db; ++j)
      {
        res[i + j] += a[i] * b[j];
      }
    }
    return res;
  }

  template<class T>
  inline Polynomial<T> operator+ (const Polynomial<T>& a, const Polynomial<T>& b)
  {
    auto res=a;
    return res+=b;
  }

  template<class T>
  inline Polynomial<T> operator- (const Polynomial<T>& a, const Polynomial<T>& b)
  {
    return a+(-b);
  }
  
  template<class T>
  inline Polynomial<T> operator+ (const Polynomial<T>& p, typename Polynomial<T>::real scalar)
  {
    return p+Polynomial<T>(scalar);
  }

  template<class T>
  inline Polynomial<T> operator+ (typename Polynomial<T>::real scalar, const Polynomial<T>& p)
  {
    return Polynomial<T>(scalar)+p;
  }

  template<class T>
  inline Polynomial<T> operator- (const Polynomial<T>& p, typename Polynomial<T>::real scalar)
  {
    return p-Polynomial<T>(scalar);
  }

  template<class T>
  inline Polynomial<T> operator- (typename Polynomial<T>::real scalar, const Polynomial<T>& p)
  {
    return Polynomial<T>(scalar)-p;
  }

  template<class T>
  inline Polynomial<T> operator* (const Polynomial<T>& p, typename Polynomial<T>::real scalar)
  {
    return p*Polynomial<T>(scalar);
  }

  template<class T>
  inline Polynomial<T> operator* (typename Polynomial<T>::real scalar, const Polynomial<T>& p)
  {
    return Polynomial<T>(scalar)*p;
  }


  // Important note: with C++ operator precedence, the ^ operator does not have a high priority,
  //                 so typical expressions such as    a*b^2  would be interpreted as:  (a*b)^2
  //                 Therefore, the overloaded function pow is used instead
  template<class T>
  inline Polynomial<T> pow(const Polynomial<T>& p, int power)
  {
    if (power < 0) THROW_ERROR("Unsupported power: " << power);
    if (power == 0) return Polynomial<T>(1.0);
    if (power == 1) return p;
    Polynomial<T> res = p;
    while (--power > 0)
      res = res*p;
    return res;
  }

  template<class T=double>
  class ScaledPolynomial
  {
    typedef typename Polynomial<T>::real real;
    
    const Polynomial<T>& m_Polynomial;
    real                 m_CoefMultiplier;
    real                 m_VarMultiplier;
  public:
    ScaledPolynomial(const Polynomial<T>& poly)
    : m_Polynomial(poly)
    {
      real first=log(fabs(poly[0]))/log(2);
      real d = ::pow(2.0,int(first));
      real range=log(fabs(poly[0]/poly[poly.degree()]))/log(2);
      real m=::pow(2.0,int(range/(1+poly.degree())));
      m_VarMultiplier = m;
      m_CoefMultiplier = d;
    }
    
    double evaluate(double x)
    {
      real y = x / m_VarMultiplier;
      real sum = 0.0;
      real yp = 1.0, mp = 1.0/m_CoefMultiplier;
      for(size_t i=0;i<=m_Polynomial.degree();++i)
      {
        sum+=m_Polynomial[i]*yp*mp;
        yp*=y;
        mp*=m_VarMultiplier;
      }
      return double(m_CoefMultiplier * sum);
    }
  };


} // namespace cxx
