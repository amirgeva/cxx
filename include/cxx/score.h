#pragma once

#include <iostream>
#include <cxx/xstring.h>

namespace cxx {

class Score
{
  int tp, tn, fp, fn, n;

  xstring perc(double d)
  {
    return xstring("{}%") << (d*100);
  }
public:
  Score()
  {
    tp = tn = fp = fn = n = 0;
  }

  void add(bool res, bool expected_res)
  {
    ++n;
    if (res == expected_res)
    {
      if (expected_res) ++tp;
      else ++tn;
    }
    else
    {
      if (expected_res) ++fn;
      else ++fp;
    }
  }

  void print(std::ostream& os=std::cout)
  {
    double tpr = double(tp) / (tp + fn);
    double fpr = double(fp) / (fp + tn);
    double precision = double(tp) / (tp + fp);
    double f1 = 2 * tp / double(2 * tp + fp + fn);
    os << "TPR=" << perc(tpr) << "  FPR=" << perc(fpr) << "  F1=" << f1 << "  N=" << n << std::endl;
  }
};

} // namespace cxx


