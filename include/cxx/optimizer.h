#pragma once

#include <vector>
#include <limits>
#include <functional>

namespace cxx {

template<class T, class D = double, class PRED=std::less<D>>
class Optimizer
{
  typedef T value_type;
  typedef D score_type;
  typedef std::vector<value_type> value_type_vec;
  typedef std::vector<score_type> score_type_vec;

  bool           empty;
  value_type_vec best;
  score_type_vec best_score;
  PRED           pred;
public:
  Optimizer(int k=1) 
  : empty(true)
  , best(k)
  , best_score(k,std::numeric_limits<D>::max())
  {}

  void set_best_score(const score_type& s) 
  { 
    best_score.assign(best_score.size(),s);
    empty = false; 
  }

  void add(const T& value, const D& score)
  {
    if (empty) 
    { 
      empty = false; 
      best[0] = value;
      best_score[0] = score;
    }
    else
    if (pred(score, best_score.back()))
    {
      if (best_score.size() == 1)
      {
        best_score.back() = score;
        best.back() = value;
      }
      else
      {
        auto it=std::lower_bound(best_score.begin(), best_score.end(), score);
        int idx = int(std::distance(best_score.begin(), it));
        best_score.insert(it, score);
        best.insert(best.begin() + idx, value);
        best_score.pop_back();
        best.pop_back();
      }
    }
  }

  bool found() const { return !empty; }

  const T& get_best() const 
  { 
    return best.back(); 
  }

  const D& get_best_score() const 
  { 
    return best_score.back(); 
  }

  void get_best_scores(D* scores)
  {
    std::copy(best_score.begin(), best_score.end(), scores);
  }

  void get_best(T* all) const 
  { 
    std::copy(best.begin(), best.end(), all); 
  }
};


} // namespace cxx


