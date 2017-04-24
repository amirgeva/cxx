#ifndef optimization_h__
#define optimization_h__

#include <types.h>

template<class M>
struct mat_traits
{
  static SparseMatrix identity(int n) 
  { 
    SparseMatrixBuilder smb;
    for (int i = 0; i < n; ++i)
      smb.add(i, i, 1.0);
    SparseMatrix m;
    smb.generate(m, true);
    return m;
  }
  static SparseMatrix diagonal(const Vector& v)
  {
    SparseMatrixBuilder smb;
    for(int i=0;i<v.size();++i)
      smb.add(i,i,v(i));
    SparseMatrix m;
    smb.generate(m,true);
    return m;
  }
  typedef Eigen::SimplicialLDLT<SparseMatrix, Eigen::Upper> solver;
};

template<>
struct mat_traits<Matrix>
{
  static Matrix identity(int n) { return Matrix::Identity(n, n); }
  static Matrix diagonal(const Vector& v) { return Matrix(v.asDiagonal()); }

  class solver : public Eigen::JacobiSVD<Matrix>
  {
  public:
    solver(const Matrix& m) 
    : Eigen::JacobiSVD<Matrix>(m, Eigen::ComputeThinU | Eigen::ComputeThinV)
    {}
  };
};

struct OptimizationParameter
{
  OptimizationParameter(double* p=nullptr, double w=1)
  : ptr(p)
  , weight(w)
  {}
  
  double   weight;
  double*  ptr;
  double& value() { return *ptr; }
  double value() const { return *ptr; }
};

typedef std::vector<OptimizationParameter> param_vec;

template<class M>
class CostFunction
{
public:
  virtual ~CostFunction() {}
  virtual param_vec& get_parameters() = 0;
  virtual bool calculate_error(Vector& err, const param_vec& params) const = 0;
  virtual bool calculate_jacobian(M& J, const param_vec& params) const = 0;
};

enum OptimizationResult
{
  OR_MAX_ITERS, OR_LOW_DELTA, OR_LOW_GRADIENT, OR_FAIL
};

inline const char* result_text(const OptimizationResult& res)
{
  switch (res)
  {
    case OR_MAX_ITERS: return "Max iters";
    case OR_LOW_DELTA: return "Low delta";
    case OR_LOW_GRADIENT: return "Low gradient";
    case OR_FAIL: return "Fail";
  }
  return "Unknown";
}


template<class M>
class LevenbergMarquardt
{
  typedef M Mat;
  typedef CostFunction<M> cost_function;
  cost_function& m_Function;
  double m_LowDeltaThres;
  double m_LowGradientThres;
  double m_StartingMu;
  std::unique_ptr<std::ofstream> m_Log;
  int m_Verbose;
  //Mat m_Weights;
public:
  LevenbergMarquardt(cost_function& f)
  : m_Function(f)
  , m_LowDeltaThres(1e-15)
  , m_LowGradientThres(1e-5)
  , m_StartingMu(1e3)
  , m_Verbose(0)
  {
    //m_Weights=mat_traits<Mat>::identity(f.get_parameters().size());
  }
  
//  void set_weights(const Vector& W)
//  {
//    if (W.size() != m_Function.get_parameters().size()) return;
//    m_Weights=mat_traits<Mat>::diagonal(W);
//  }
  
  void enable_logging(const xstring& filename)
  {
    m_Log.reset(new std::ofstream(filename));
  }

  void set_low_delta_thershold(double t)
  {
    m_LowDeltaThres = t;
  }

  void set_low_gradient_threshold(double t)
  {
    m_LowGradientThres = t;
  }

  void set_starting_mu(double mu)
  {
    m_StartingMu = mu;
  }
  
  void set_verbose(int level)
  {
    m_Verbose=level;
  }

  OptimizationResult optimize(unsigned& max_iters)
  {
    OptimizationResult res = OR_MAX_ITERS;
    Mat H, J;
    Vector err, next_err;
    double err_norm, next_err_norm;
    double Mu = m_StartingMu;
    param_vec& params=m_Function.get_parameters();
    Vector next_param_values(params.size());
    param_vec P(params.size());
    for(unsigned i=0;i<params.size();++i)
    {
      next_param_values(i)=params[i].value();
      P[i].ptr=&next_param_values(i);
    }

    if (!m_Function.calculate_error(err, P)) return OR_FAIL;
    if (!m_Function.calculate_jacobian(J, P)) return OR_FAIL;
    err_norm = err.squaredNorm();
    H = J.transpose() * J;
    Mat I = mat_traits<Mat>::identity(int(H.rows()));
    Vector g = 2 * J.transpose()*err;
    double grad_norm = g.squaredNorm();
    unsigned iter = 0;
    for (; iter < max_iters; ++iter)
    {
      if (grad_norm < m_LowGradientThres)
      {
        res = OR_LOW_GRADIENT;
        break;
      }
      typename mat_traits<M>::solver solver(H+Mu*I);
      Vector delta=solver.solve(-g);
      if (iter<10)
      {
        //delta=m_Weights*delta;
        for(int i=0;i<delta.size();++i)
          delta(i)*=params[i].weight;
      }
      if (delta.norm() < m_LowDeltaThres)
      {
        res = OR_LOW_DELTA;
        break;
      }
//      if (m_Log)
//        (*m_Log) << csv(P) << csv(gradient) << csv(
      for(unsigned i=0;i<P.size();++i)
        P[i].value() = params[i].value() + delta(i);

      if (!m_Function.calculate_error(next_err, P)) return OR_FAIL;
      next_err_norm = next_err.squaredNorm();
      if (next_err_norm < err_norm)
      {
        //if ((iter%100)==0)
        if (m_Verbose>1)
        {
          double avge=sqrt(next_err_norm / next_err.size());
          std::cout << iter << ": g=" << grad_norm << "  e=" << next_err_norm << "  ae=" << avge << "  Mu=" << Mu << " params=";
          for(int i=0;i<3;++i)
            std::cout << P[i].value() << ' ';
          std::cout << std::endl;
        }
        //std::cout << iter << ": " << grad_norm << "  Mu=" << Mu << "  C0=" << C0.transpose() << "  C1=" << C1.transpose() << std::endl;
        err_norm = next_err_norm;
        err = next_err;
        for(unsigned i=0;i<P.size();++i)
          params[i].value() = P[i].value();
        if (!m_Function.calculate_jacobian(J, P)) return OR_FAIL;
        H = J.transpose()*J;
        g = 2 * J.transpose()*err;
        grad_norm = g.squaredNorm();
        if (Mu > 1) Mu *= 0.8;
        else
          Mu = Mu*0.99;
        //if (U() < 0.1) Mu *= 2;
      }
      else
      {
        Mu = Mu*1.2;
      }
    }
    max_iters = iter;
    return res;
  }
};

#endif // optimization_h__
