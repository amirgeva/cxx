#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include <cxx/optimizer.h>
#include <cxx/prims.h>

namespace cxx {

template<class P, class D=typename P::value_type>
inline D sqdist(const P& a, const P& b)
{
  D dx = a.x - b.x, dy = a.y - b.y;
  return dx*dx + dy*dy;
}

template<class T>
struct PayloadPoint
{
  typedef double value_type;
  double x, y;
  T      payload;
  PayloadPoint(double X = 0, double Y = 0, const T& pl=T()) : x(X), y(Y), payload(pl) {}
};

template<class POINT>
class TwoDTree
{
  typedef POINT point;
  typedef typename POINT::value_type value_type;

  struct Node;
  typedef std::shared_ptr<Node> node_ptr;

  struct Node
  {
    Node() : erased(false) {}

    point    plane;
    node_ptr left, right;
    bool     erased;

    void split(const point& middle)
    {
      plane = middle;
      left = node_ptr(new Node);
      right = node_ptr(new Node);
    }
  };
  node_ptr m_Root;

  class Iterator : public std::iterator<std::forward_iterator_tag,point>
  {
    typedef std::vector<node_ptr> stack_type;
    stack_type stack;

    void add_branch(node_ptr root)
    {
      while (root)
      {
        stack.push_back(root);
        root = root->left;
      }
    }
  public:
    Iterator() {}

    Iterator(node_ptr root)
    {
      add_branch(root);
      if (!stack.empty() && stack.back()->erased)
        ++(*this);
    }

    void erase()
    {
      if (!stack.empty())
        stack.back()->erased = true;
    }

    const point& operator* () const { return stack.back()->plane; }
    const point* operator-> () const { return &(stack.back()->plane); }
    Iterator& operator++()
    {
      bool done = false;
      while (!done)
      {
        stack.pop_back();
        if (!stack.empty())
        {
          node_ptr next = stack.back()->right;
          stack.pop_back();
          add_branch(next);
        }
        if (stack.empty()) done = true;
        else
        if (!stack.back()->erased) done = true;
      }
      return *this;
    }

    bool operator!= (const Iterator& rhs) const { return stack != rhs.stack; }
  };

  struct axe_pred : public std::binary_function<point, point, bool>
  {
    int m_Axe;
    axe_pred(int depth) : m_Axe(depth&1) {}
    bool operator() (const point& p1, const point& p2) const
    {
      if (m_Axe == 0) return p1.x < p2.x;
      return p1.y < p2.y;
    }
  };

  static value_type axe_point(const point& p, int depth)
  {
    if ((depth & 1) == 0) return p.x;
    return p.y;
  }

  template<class II>
  static value_type find_median(II b, II e, int depth)
  {
    unsigned n = unsigned(std::distance(b, e));
    unsigned n2 = n / 2;
    std::nth_element(b, b + n2, e,axe_pred(depth));
    return axe_point(*(b + n2), depth);
  }

  template<class II>
  void build(node_ptr& root, II b, II e, int depth)
  {
    int n = int(std::distance(b, e));
    if (n == 1) root->plane = *b;
    else
    {
      value_type median = find_median(b, e, depth);
      II mid=std::partition(b, e, [depth, median](const point& p) { return axe_point(p, depth) < median; });
      if (mid == b) ++mid;
      root->split(*mid);
      build(root->left, b, mid, depth + 1);
      build(root->right, mid, e, depth + 1);
    }
  }

  int find_nn(const node_ptr& node, const point& p, Optimizer<node_ptr>& opt, int depth) const
  {
    int res = 0;
    if (!node->left)
    {
      res = 1;
      if (!node->erased)
        opt.add(node, sqdist(node->plane, p));
      return res;
    }
    value_type dist = axe_point(p, depth) - axe_point(node->plane, depth);
    value_type d2 = dist*dist;
    if (dist < 0)
    {
      res+=find_nn(node->left, p, opt, depth + 1);
      if (d2 <= opt.get_best_score()) 
        res += find_nn(node->right, p, opt, depth + 1);
    }
    else
    {
      res += find_nn(node->right, p, opt, depth + 1);
      if (d2 <= opt.get_best_score())
        res += find_nn(node->left, p, opt, depth + 1);
    }
    return res;
  }

public:
  typedef Iterator iterator;
  iterator begin() { return Iterator(m_Root); }
  iterator end()   { return Iterator(node_ptr()); }

  void find_knn(const point& p, int k, iterator* res, double* scores=0) const
  {
    Optimizer<node_ptr> opt(k);
    opt.set_best_score(std::numeric_limits<float>::max());
    find_nn(m_Root, p, opt, 0);
    std::vector<node_ptr> pres(k);
    opt.get_best(&pres[0]);
    for (int i = 0; i < k; ++i)
      res[i] = Iterator(pres[i]);
    if (scores)
      opt.get_best_scores(scores);
  }

  iterator find_nn(const point& p) const
  {
    Optimizer<node_ptr> opt;
    opt.set_best_score(std::numeric_limits<float>::max());
    find_nn(m_Root, p, opt, 0);
    return iterator(opt.get_best());
  }

  void erase(iterator it)
  {
    it.erase();
  }

  template<class II>
  void build(II b, II e)
  {
    m_Root = node_ptr(new Node);
    build(m_Root, b, e, 0);
  }

};


template<class POINT>
class BruteKDTree
{
  typedef POINT point;
  typedef std::vector<point> point_seq;
  point_seq m_Points;
public:
  template<class II>
  void build(II b, II e)
  {
    m_Points.assign(b, e);
  }

  const point& find_nn(const point& p) const
  {
    Optimizer<int> opt;
    int n = m_Points.size();
    for (int i = 0; i < n;++i) opt.add(i, sqdist(m_Points[i], p));
    return m_Points[opt.get_best()];
  }

  void find_knn(const point& p, int k, point* res) const
  {
    Optimizer<int> opt(k);
    int n = m_Points.size();
    for (int i = 0; i < n; ++i) opt.add(i, sqdist(m_Points[i], p));
    std::vector<int> residx(k);
    opt.get_best(&residx[0]);
    for (int i = 0; i < k; ++i) res[i] = m_Points[residx[i]];
  }
};

} // namespace cxx
