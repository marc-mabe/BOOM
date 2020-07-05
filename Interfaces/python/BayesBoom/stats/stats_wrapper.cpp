#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>

#include "LinAlg/Vector.hpp"
#include "LinAlg/Matrix.hpp"
#include "LinAlg/SpdMatrix.hpp"

#include "stats/Spline.hpp"
#include "stats/Bspline.hpp"
#include "stats/moments.hpp"
#include "stats/DataTable.hpp"
#include "stats/IQagent.hpp"

#include "Models/DataTypes.hpp"
#include "cpputil/Ptr.hpp"

namespace py = pybind11;
PYBIND11_DECLARE_HOLDER_TYPE(T, BOOM::Ptr<T>, true);

namespace BayesBoom {
  using namespace BOOM;

  void stats_def(py::module &boom) {

    boom.def("mean", [](const Matrix &m){return mean(m);},
             "Returns the mean of each column of m as a boom.Vector.");
    boom.def("var", [](const Matrix &m){return var(m);},
             "Returns the variance matrix of the data in a boom.Matrix.");
    boom.def("cor", [](const Matrix &m){return cor(m);},
             "Returns the correlation matrix of the data in a boom.Matrix.");

    boom.def("mean", [](const Vector &m){return mean(m);},
             "Returns the mean of a boom.Vector.");
    boom.def("var", [](const Vector &m){return mean(m);},
             "Returns the variance of a boom.Vector.");
    boom.def("sd", [](const Vector &m){return mean(m);},
             "Returns the standard deviation of a boom.Vector.");

    py::class_<SplineBase> (boom, "SplineBase")
        .def("basis", &SplineBase::basis, py::arg("x: float"),
             py::return_value_policy::copy,
             "Spline basis expansion at x.")
        .def("basis_matrix", &SplineBase::basis_matrix, py::arg("x: Vector"),
             py::return_value_policy::copy,
             "Spline basis matrix expansion of the Vector x.")
        .def_property_readonly("dim", &SplineBase::basis_dimension,
                               "The dimension of the expanded basis.")
        .def("add_knot", &SplineBase::add_knot, py::arg("knot: float"),
             "Add a knot at the specified value.  The support of the spline will be "
             "expanded to include 'knot' if necessary.")
        .def("remove_knot", &SplineBase::remove_knot, py::arg("which_knot: int"),
             "Remove the specified knot.  If which_knot corresponds to the "
             "largest or smallest knots then the support of the spline will be "
             "reduced.")
        .def("knots", &SplineBase::knots)
        .def("number_of_knots", &SplineBase::number_of_knots)
        ;


    py::class_<Bspline, SplineBase>(boom, "Bspline")
        .def(py::init<const Vector &, int>(), py::arg("knots"), py::arg("degree") = 3,
             "Create a Bspline basis.\n\n")
        .def("basis", (Vector (Bspline::*)(double)) &Bspline::basis, py::arg("x"),
             py::return_value_policy::copy,
             "The basis function expansion at x.")
        .def_property_readonly("order", &Bspline::order,
                               "The order of the spline. (1 + degree).")
        .def_property_readonly("degree", &Bspline::degree, "The degree of the spline.")
        .def("__repr__",
             [](const Bspline &s) {
               std::ostringstream out;
               out << "A Bspline basis of degree " << s.degree() << " with knots at ["
                   << s.knots() << "].";
               return out.str();
             })
        ;

    py::class_<IQagent>(boom, "IQagent")
        .def(py::init(
            [](int bufsize) {
              return new IQagent(bufsize);
            }),
             py::arg("bufsize") = 20,
             "Args:\n"
             "  bufsize:  The number of data points to store before triggering a CDF "
             "refresh.")
        .def(py::init(
            [](const Vector &probs, int bufsize) {
              return new IQagent(probs, bufsize);
            }),
             py::arg("probs"),
             py::arg("bufsize") = 20,
             "Args\n"
             "  probs: A vector of probabilities defining the quantiles to focus on."
             "  bufsize:  The number of data points to store before triggering a CDF "
             "refresh.")
        .def(py::init(
            [](const IqAgentState &state) {
              return new IQagent(state);
            }),
             py::arg("state"),
             "Args:\n"
             "  state:  An object of class IqAgentState, previously generated by "
             "save_state().")
        .def(py::pickle(
            [](const IQagent &agent) {
              auto state = agent.save_state();
              return py::make_tuple(
                  state.max_buffer_size,
                  state.nobs,
                  state.data_buffer,
                  state.probs,
                  state.quantiles,
                  state.ecdf_sorted_data,
                  state.fplus,
                  state.fminus);
            },
            [](const py::tuple &tup) {
              IqAgentState state;
              state.max_buffer_size = tup[0].cast<int>();
              state.nobs = tup[1].cast<int>();
              state.data_buffer = tup[2].cast<Vector>();
              state.probs = tup[3].cast<Vector>();
              state.quantiles = tup[4].cast<Vector>();
              state.ecdf_sorted_data = tup[5].cast<Vector>();
              state.fplus = tup[6].cast<Vector>();
              state.fminus = tup[7].cast<Vector>();

              return IQagent(state);
            }))
        .def("add",
             [](IQagent &agent, double x) {
               agent.add(x);
             },
             py::arg("x"),
             "Args:\n"
             "  x: A data point to add to the empirical distribution.")
        .def("quantile", &IQagent::quantile, py::arg("prob"),
             "Args:\n"
             "  prob:  The probability for which a quantile is desired.")
        .def("cdf", &IQagent::cdf, py::arg("x"),
             "Args:\n"
             "  x: Return the fraction of data <= x.")
        .def("update_cdf", &IQagent::update_cdf,
             "Merge the data buffer into the CDF.  Update the CDF estimate.  "
             "Clear the data buffer.")
        ;


    py::class_<DataTable,
               Data,
               Ptr<DataTable>>(boom, "DataTable")
        .def(py::init(
            []() {return new DataTable;}),
             "Default constructor.")
        .def("add_numeric",
             [](DataTable &table,
                const Vector &values,
                const std::string &name) {
               table.append_variable(values, name);
             },
             py::arg("values"),
             py::arg("name"),
             "Args:\n"
             "  values: The numeric values to append.\n"
             "  name: The name of the numeric variable.\n")
        .def("add_categorical",
             [](DataTable &table,
                const std::vector<int> &values,
                const std::vector<std::string> &labels,
                const std::string &name){
               NEW(CatKey, key)(labels);
               table.append_variable(CategoricalVariable(values, key), name);
             },
             py::arg("values"),
             py::arg("labels"),
             py::arg("name"),
             "Args:\n"
             "  values:  The numeric codes of the categorical variables.\n"
             "  labels:  The labels corresponding to the unique values in "
             "'values.'\n"
             "  name:  The name of the categorical variable.")
        .def("add_categorical_from_labels",
             [](DataTable &table,
                const std::vector<std::string> &values,
                const std::string &name) {
               table.append_variable(CategoricalVariable(values), name);
             },
             py::arg("values"),
             py::arg("name"),
             "Args:\n"
             "  values:  The values (as strings) of the variable to be added.\n"
             "  name:  The name of the categorical variable.")
        .def_property_readonly(
            "nrow", &DataTable::nobs,
            "Number of rows (observations) in the table.")
        .def_property_readonly(
            "ncol", &DataTable::nvars,
            "Number of columns (variables) in the table.")
        ;

  }  // ends the Spline_def function.

}  // namespace BayesBoom
