#pragma once

#include <memory>
#include <random>
#include <chrono>

namespace cxx {

class RandomGenerator
{
public:
  static RandomGenerator* instance()
  {
    static std::unique_ptr<RandomGenerator> ptr(new RandomGenerator);
    return ptr.get();
  }

  double gen() { return uniform(engine); }
  int    geni(int range) { return int(gen()*range); }
  double gen_gaussian(double sigma)
  {
    return gaussian(engine)*sigma;
  }

  void set_seed(unsigned seed)
  {
    engine.seed(seed);
  }

  void set_seed()
  {
    auto t=std::chrono::system_clock::now();
    unsigned s=unsigned(std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch()).count());
    set_seed(s);
  }

private:
  friend struct std::default_delete<RandomGenerator>;
  RandomGenerator() 
  : engine(unsigned(std::chrono::high_resolution_clock::now().time_since_epoch().count())) 
  {}
  ~RandomGenerator() {}
  RandomGenerator(const RandomGenerator&) {}
  RandomGenerator& operator= (const RandomGenerator&) { return *this; }

  std::mt19937 engine;
  std::uniform_real_distribution<double> uniform;
  std::normal_distribution<double> gaussian;
};

inline double R(double s = 2.0)
{
  return s*(RandomGenerator::instance()->gen() - 0.5);
}

inline double G(double sigma=1.0)
{
  return RandomGenerator::instance()->gen_gaussian(sigma);
}

inline double U(double mx=1.0) { return mx*RandomGenerator::instance()->gen(); }

inline int    UI(int v) { return int(RandomGenerator::instance()->gen()*v); }


} // namespace cxx
