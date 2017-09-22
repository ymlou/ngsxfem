#include <python_ngstd.hpp>

//using namespace ngcomp;
#include "../utils/ngsxstd.hpp"

void ExportNgsx(py::module &m)
{

  py::enum_<DOMAIN_TYPE>(m, "DOMAIN_TYPE")
    .value("POS", POS)
    .value("NEG", NEG)
    .value("IF", IF)
    .export_values()
    ;
  
  // empty now ;)
}

PYBIND11_PLUGIN(ngsxfem_py)
{
  cout << "importing ngs-xfem" << NGSXFEM_VERSION << endl;
  py::module m("ngsxfem", "pybind ngsxfem");
  ExportNgsx(m);
  return m.ptr();
}
