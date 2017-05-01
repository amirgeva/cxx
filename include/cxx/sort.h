#pragma once

#include <vector>
#include <algorithm>
#include <numeric>
#include <functional>

namespace cxx {

template <typename T, typename Compare>
std::vector<std::size_t> sort_permutation(const std::vector<T>& vec, Compare& compare)
{
  std::vector<std::size_t> p(vec.size());
  std::iota(p.begin(), p.end(), 0);
  std::sort(p.begin(), p.end(),
    [&](std::size_t i, std::size_t j) { return compare(vec[i], vec[j]); });
  return p;
}

template <typename T>
std::vector<T> apply_permutation(const std::vector<T>& vec, const std::vector<std::size_t>& p)
{
  std::vector<T> sorted_vec(p.size());
  std::transform(p.begin(), p.end(), sorted_vec.begin(),
    [&](std::size_t i) { return vec[i]; });
  return sorted_vec;
}

template<class T, class I, class P>
void sort_vector_pair(std::vector<T>& v, std::vector<I>& i, P pred=std::less<T>())
{
  auto perm = sort_permutation(v, pred);
  v=apply_permutation(v, perm);
  i=apply_permutation(i, perm);
}

} // namespace cxx


