#pragma once

#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <cmath>

typedef unsigned char byte;

const double PI = 3.1415926535897932384626433832795;

template<class T>
inline const T& Max(const T& a, const T& b)
{
  return a>b?a:b;
}

template<class T>
inline const T& Min(const T& a, const T& b)
{
  return a<b?a:b;
}

template<class T>
class min_func : public std::binary_function<T,T,T>
{
public:
  const T& operator() (const T& a, const T& b) const
  {
    return Min(a,b);
  }
};

template<class T>
class max_func : public std::binary_function<T,T,T>
{
public:
  const T& operator() (const T& a, const T& b) const
  {
    return Max(a,b);
  }
};



template<class T>
inline T udiff(const T& a, const T& b)
{
  return a<b?b-a:a-b;
}


template<class T>
struct gPoint
{
  typedef gPoint<T> self;
  T x, y;
  gPoint(const T& X = 0, const T& Y = 0)
    : x(X), y(Y) {}

  bool operator== (const self& rhs) const { return x == rhs.x && y == rhs.y; }
  bool operator!= (const self& rhs) const { return !(*this == rhs); }
  self& operator+= (const self& rhs) { x += rhs.x; y += rhs.y; return *this; }
  self& operator-= (const self& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
  self operator- () const { return self(-x, -y); }
  T squared_norm() const { return x * x + y * y; }
  double norm() const { return sqrt(squared_norm()); }
};

template<class T>
inline gPoint<T> operator+ (const gPoint<T>& a, const gPoint<T>& b)
{
  gPoint<T> res = a;
  return res += b;
}

template<class T>
inline gPoint<T> operator- (const gPoint<T>& a, const gPoint<T>& b)
{
  gPoint<T> res = a;
  return res -= b;
}

template<class T>
inline std::ostream& operator<< (std::ostream& os, const gPoint<T>& p)
{
  return os << p.x << ',' << p.y;
}

typedef gPoint<unsigned short> sPoint;
typedef gPoint<int> Point;
typedef gPoint<double> dPoint;

struct Size
{
  unsigned width, height;
  Size(unsigned w = 0, unsigned h = 0) : width(w), height(h) {}
};

struct Rect
{
  struct PointIterator : public std::iterator<std::forward_iterator_tag,Point>
  {
    const Rect& m_Rect;
    Point m_Cur;
    PointIterator(const Rect& r, bool end) : m_Rect(r), m_Cur(r.l,r.t) 
    {
      if (end) m_Cur = Point(r.l, r.b);
    }

    const Point& operator* () const { return m_Cur; }
    PointIterator& operator++ () 
    { 
      if (++m_Cur.x == m_Rect.r) 
      { 
        m_Cur.y++; 
        m_Cur.x = m_Rect.l; 
      } 
      return *this; 
    }
    PointIterator operator++(int) { PointIterator i = *this; ++(*this); return i; }
    bool operator== (const PointIterator& rhs) const { return m_Cur == rhs.m_Cur; }
    bool operator!= (const PointIterator& rhs) const { return !(*this == rhs);    }
  };

  typedef PointIterator const_iterator;
  const_iterator begin() const { return PointIterator(*this,false); }
  const_iterator end() const { return PointIterator(*this,true); }

  int l,t,r,b;
  Rect(int L=0, int T=0, int R=0, int B=0) : l(L), t(T), r(R), b(B) {}
  Rect(const Point& tl, const Point& br)
    : l(tl.x)
    , t(tl.y)
    , r(br.x)
    , b(br.y)
  {}
  void init_unite(int x, int y)
  {
    if (l == r || t == b)
    {
      l = x;
      t = y;
      r = l + 1;
      b = t + 1;
    }
    else
      unite(x, y);
  }
  void unite(int x, int y)
  {
    if (x<l) l=x;
    if (x>=r) r=x+1;
    if (y<t) t=y;
    if (y>=b) b=y+1;
  }
  void unite(const Rect& rhs)
  {
    if (rhs.l<l) l=rhs.l;
    if (rhs.r>r) r=rhs.r;
    if (rhs.t<t) t=rhs.t;
    if (rhs.b>b) b=rhs.b;
  }
  bool is_point_inside(int x, int y) const
  {
    return (x>=l && x<r && y>=t && y<b);
  }
  //Vec2D get_center() const { return Vec2D(0.5f*(l+r),0.5f*(t+b)); }
  int width() const { return r-l; }
  int height() const { return b-t; }
  int get_area() const { return width()*height(); }
  double aspect_ratio() const { return double(width())/double(height()); }
  bool valid() const { return (r>l && b>t); }
  int get_coord(int index) const
  {
    switch (index) {
      case 0: return l;
      case 1: return t;
      case 2: return r;
      case 3: return b;
    }
    return -1;
  }

  Rect& offset(int x, int y)
  {
    l+=x; r+=x;
    t+=y; b+=y;
    return *this;
  }
  
  Rect& offset(const Point& p)
  {
    return offset(p.x,p.y);
  }

  Rect& intersect(const Rect& rhs)
  {
    l=Max(l,rhs.l);
    t=Max(t,rhs.t);
    r=Min(r,rhs.r);
    b=Min(b,rhs.b);
    return *this;
  }

  bool overlaps(const Rect& rhs) const
  {
    Rect rect = *this;
    rect.intersect(rhs);
    return rect.valid();
  }

  bool contains(const Point& p) const
  {
    return (p.x >= l && p.y >= t && p.x < r && p.y < b);
  }

  bool contains(const Rect& rhs) const
  {
    return (rhs.l>=l && rhs.t>=t && rhs.r<=r && rhs.b<=b);
  }
};

inline std::ostream& operator<< (std::ostream& os, const Rect& r)
{
  return os << r.l << ',' << r.t << ',' << r.r << ',' << r.b;
}

template<class T>
inline T sqr(const T& t) { return t*t; }

template<class T>
inline T cubed(const T& t) { return t*t*t; }

template<class T>
inline bool is_negative(const T& t) { return t<0; }

class WindowAverage
{
  float m_Value;
  float m_Alpha;
public:
  WindowAverage(float alpha=0.6) : m_Value(0), m_Alpha(alpha) {}
  void set_value(float v) { m_Value=v; }
  void set_alpha(float a) { m_Alpha=a; }
  float update(float v) 
  { 
    m_Value=(v*m_Alpha)+m_Value*(1-m_Alpha);
    return m_Value;
  }
  float get() const { return m_Value; }
};

