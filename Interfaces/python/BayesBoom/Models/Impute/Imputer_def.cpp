#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "Models/ModelTypes.hpp"
#include "Models/Glm/Glm.hpp"
#include "Models/Glm/GlmCoefs.hpp"
#include "Models/Glm/PosteriorSamplers/MultivariateRegressionSampler.hpp"

#include "Models/Impute/MvRegCopulaDataImputer.hpp"

#include "cpputil/Ptr.hpp"

namespace py = pybind11;
PYBIND11_DECLARE_HOLDER_TYPE(T, BOOM::Ptr<T>, true);

namespace BayesBoom {
  using namespace BOOM;

  void Imputation_def(py::module &boom) {
    py::class_<MvRegCopulaDataImputer,
               Ptr<MvRegCopulaDataImputer>>(boom, "MvRegCopulaDataImputer")
        .def(py::init(
            [](int num_clusters,
               const std::vector<Vector> &atoms,
               int xdim,
               BOOM::RNG &seeding_rng) {
              return new MvRegCopulaDataImputer(num_clusters, atoms, xdim, seeding_rng);
            }),
             py::arg("num_clusters"),
             py::arg("atoms"),
             py::arg("xdim"),
             py::arg("seeding_rng") = BOOM::GlobalRng::rng,
             "Args:\n"
             "  num_clusters:  The number of clusters in the patern matching model \n"
             "    that handles data errors. \n"
             "  atoms: A collection of Vectors (or 1-d numpy arrays) containing \n"
             "    values that will receive special modeling treatment.  One entry is\n"
             "    needed for each variable.  An entry can be the empty Vector. \n"
             "  xdim:  The dimension of the predictor variable.\n"
             "  seeding_rng:  A boom random number generator used to seed the \n"
             "    RNG in this object.")
        .def("add_data",
             [](MvRegCopulaDataImputer &imputer,
                const Ptr<MvRegData> &data_point) {
               imputer.add_data(data_point);
             },
             py::arg("data_point"),
             "Add a data point to the training data set.\n\n"
             "Args:\n"
             "  data_point:  Object of type boom.MvRegData.  The y variable \n"
             "    should indicate missing values with NaN.\n")
        .def_property_readonly("xdim", &MvRegCopulaDataImputer::xdim,
                               "dimension of the predictor variable")
        .def_property_readonly("ydim", &MvRegCopulaDataImputer::ydim,
                               "dimension of the numeric data")
        .def_property_readonly(
            "coefficients",
            [](MvRegCopulaDataImputer &imputer) {
              Matrix ans = imputer.regression()->Beta();
              return ans;
            },
            "The matrix of regression coefficients.  Rows correspond to Y (output).\n"
            "Columns correspond to X (input).  Coefficients represent the \n"
            "relationship between X and the copula transform of Y.")
        .def_property_readonly(
            "residual_variance",
            [](MvRegCopulaDataImputer &imputer) {
              SpdMatrix ans = imputer.regression()->Sigma();
              return ans;
            },
            "The residual variance matrix on the transformed (copula) scale.")
        .def_property_readonly(
            "nclusters",
            &MvRegCopulaDataImputer::nclusters,
            "The number of clusters in the error pattern matching model.")
        .def_property_readonly("imputed_data",
                               &MvRegCopulaDataImputer::imputed_data,
                               "The numeric portion of the imputed data set.")
        .def_property_readonly("atoms",
                               &MvRegCopulaDataImputer::atoms,
                               "The atoms for each y variable.")
        .def("atom_probs",
             [](const MvRegCopulaDataImputer &imputer, int cluster,
                int variable_index) {
               return imputer.atom_probs(cluster, variable_index);
             },
             "The marginal probability that each atom is the 'truth'.")
        .def("atom_error_probs",
             [](const MvRegCopulaDataImputer &imputer, int cluster, int variable_index) {
               return imputer.atom_error_probs(cluster, variable_index);
             },
             "The marginal probability that each atom is the 'truth'.")
        .def("set_default_priors",
             &MvRegCopulaDataImputer::set_default_priors,
             "Set default priors on everything.")
        .def("set_default_regression_prior",
             &MvRegCopulaDataImputer::set_default_regression_prior,
             "Set a 'nearly flat' prior on the regression coefficients and residual "
             "variance.")
        .def("set_default_prior_for_mixing_weights",
             &MvRegCopulaDataImputer::set_default_prior_for_mixing_weights)
        .def("set_atom_prior",
             [](MvRegCopulaDataImputer &imputer,
                const Vector &prior_counts,
                int variable_index) {
               imputer.set_atom_prior(prior_counts, variable_index);
             },
             "Args:\n"
             "  prior_counts: Vector of prior counts indicating the likelihood \n"
             "    that each atom is the true value.  Negative counts indicate\n"
             "    an a-priori assertion that the level cannot be the true value.\n"
             "    The size of the vector must be one larger than the number of \n"
             "    atoms, with the final element corresponding to the continuous atom.\n"
             "  variable_index:  Index of the variable to which the prior refers.\n")
        .def("set_atom_error_prior",
             [](MvRegCopulaDataImputer &imputer,
                const Matrix &prior_counts,
                int variable_index) {
               imputer.set_atom_error_prior(prior_counts, variable_index);
             },
             "TODO: docstring")
        .def("sample_posterior",
             [](MvRegCopulaDataImputer &imputer) {
               imputer.sample_posterior();
             },
             "Take one draw from the posterior distribution")
        .def("impute_data_set",
             [](MvRegCopulaDataImputer &imputer,
                const std::vector<Ptr<MvRegData>> &data) {
               return imputer.impute_data_set(data);
             },
             "Return a Boom.Matrix containing the imputed draws.")
        .def("set_residual_variance",
             [](MvRegCopulaDataImputer &imputer, const Matrix &Sigma) {
               imputer.regression()->set_Sigma(SpdMatrix(Sigma));
             })
        .def("set_coefficients",
             [](MvRegCopulaDataImputer &imputer, const Matrix &Beta) {
               imputer.regression()->set_Beta(Beta);
             })
        .def("set_atom_probs",
             &MvRegCopulaDataImputer::set_atom_probs)
        .def("set_atom_error_probs",
             &MvRegCopulaDataImputer::set_atom_error_probs)
        .def_property_readonly(
            "empirical_distributions",
            &MvRegCopulaDataImputer::empirical_distributions,
            "The approximate numerical distribution of each numeric variable")
        .def("set_empirical_distributions",
             &MvRegCopulaDataImputer::set_empirical_distributions,
             "Restore the empirical distributions from serialized state.")
        .def("setup_worker_pool",
             &MvRegCopulaDataImputer::setup_worker_pool,
             py::arg("nworkers"),
             "Set up a worker pool to train with 'nworkers' threads.")
        ;

  }  // module boom

}  // namespace BayesBoom
