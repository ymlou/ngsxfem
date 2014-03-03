#ifndef FILE_ERROR_HPP
#define FILE_ERROR_HPP

#include <ngstd.hpp> // for Array
#include <fem.hpp>   // for ScalarFiniteElement
#include <solve.hpp>

// #include "xintegration.hpp"
#include "../spacetime/spacetimefespace.hpp"
#include "../xfem/xfemIntegrators.hpp"
#include "../xfem/stxfemIntegrators.hpp"
#include "../xfem/setvaluesx.hpp"
#include "../utils/error.hpp"

using namespace ngsolve;

/// from ngsolve

#include "../cutint/xintegration.hpp"
#include "../xfem/xfiniteelement.hpp"
#include "../spacetime/spacetimeintegrators.hpp"

namespace ngfem
{
  template<int D>
  void CalcDxShapeOfCoeff(const CoefficientFunction * coef, const MappedIntegrationPoint<D,D>& mip, 
                          double time,
                          Vec<D>& der, LocalHeap& lh);

  template <int D>
  class SolutionCoefficients
  {
  protected:
    const CoefficientFunction * coef_n = NULL;
    const CoefficientFunction * coef_p = NULL;
    const CoefficientFunction * coef_d_n = NULL;
    const CoefficientFunction * coef_d_p = NULL;
    const CoefficientFunction * lset = NULL;
    bool made_coef_n = false;
    bool made_coef_p = false;
    bool made_coef_d_n = false;
    bool made_coef_d_p = false;
    bool made_lset = false;

  public:
    // SolutionCoefficients(const CoefficientFunction * a_coef_n,
    //                      const CoefficientFunction * a_coef_p,
    //                      const CoefficientFunction * a_coef_d_n,
    //                      const CoefficientFunction * a_coef_d_p,
    //                      const CoefficientFunction * a_lset);
    SolutionCoefficients(PDE & pde, const Flags & flags);
    ~SolutionCoefficients();

    bool HasSolutionNeg() { return ! (coef_n == NULL); }
    bool HasSolutionPos() { return ! (coef_p == NULL); }
    bool HasSolutionDNeg() { return ! (coef_d_n == NULL); }
    bool HasSolutionDPos() { return ! (coef_d_p == NULL); }
    bool HasLevelSet() { return ! (lset == NULL); }

    const CoefficientFunction & GetSolutionNeg() { return *coef_n; }
    const CoefficientFunction & GetSolutionPos() { return *coef_p; }
    const CoefficientFunction & GetSolutionDNeg() { return *coef_d_n; }
    const CoefficientFunction & GetSolutionDPos() { return *coef_d_p; }
    const CoefficientFunction & GetLevelSet() { return *lset; }

  };

  class ErrorTable
  {
  public:
    Array<double> l2err_p;
    Array<double> l2err_n;
    Array<double> l2err;
    Array<double> h1err_p;
    Array<double> h1err_n;
    Array<double> h1err;
    Array<double> iferr;

    ErrorTable();
  };

  template<int D>
  void CalcXError (GridFunction * gfu, SolutionCoefficients<D> & solcoef, double intorder, 
                   double b_neg, double b_pos, double time, ErrorTable & errtab, LocalHeap & lh);
  


}
#endif
