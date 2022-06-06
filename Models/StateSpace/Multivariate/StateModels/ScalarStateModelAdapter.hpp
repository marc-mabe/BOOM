#ifndef BOOM_STATE_SPACE_SCALAR_STATE_MODEL_ADAPTER_HPP_
#define BOOM_STATE_SPACE_SCALAR_STATE_MODEL_ADAPTER_HPP_

/*
  Copyright (C) 2022 Steven L. Scott

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License along
  with this library; if not, write to the Free Software Foundation, Inc., 51
  Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

#include "Models/ZeroMeanGaussianModel.hpp"
#include "Models/StateSpace/Multivariate/MultivariateStateSpaceModelBase.hpp"
#include "Models/Policies/CompositeParamPolicy.hpp"
#include "Models/Policies/NullDataPolicy.hpp"
#include "Models/Policies/PriorPolicy.hpp"

#include "Models/Glm/RegressionModel.hpp"
#include "Models/Glm/WeightedRegressionModel.hpp"

#include "Models/StateSpace/Multivariate/StateModels/SharedStateModel.hpp"

namespace BOOM {

  //===========================================================================
  // Adapts a collection of one or more StateModel objects (designed for use
  // with scalar time series) for use as a SharedStateModel.
  //
  // The model matrices that are specific to the state are all determined by the
  // base StateModel.  The observation coefficients are determined by a
  // collection of linear regressions, with one regression model assigned to
  // each element of the response vector.
  //
  // In notation, the observation equation is
  //   y[t, j] = beta[j] * [Z[t] * alpha[j]] + error[t, j].
  // and the state equation is
  //   alpha[t+1] = T[t] * alpha[t] + R[t] * innovation[t].
  //
  // Each regression model is a one-dimensional model.
  class ScalarStateModelMultivariateAdapter
      : virtual public SharedStateModel,
        public CompositeParamPolicy,
        public NullDataPolicy,
        public PriorPolicy
  {
   public:
    // Args:
    //   nseries:  The number of observed time series being modeled.
    ScalarStateModelMultivariateAdapter(int nseries);
    ScalarStateModelMultivariateAdapter *clone() const override = 0;

    // The number of time series being modeled.
    int nseries() const;

    virtual void add_state(const Ptr<StateModel> &state) {
      component_models_.add_state(state);
    }

    void clear_data() override {
      component_models_.clear_data();
    }

    // Observe the state for the transition part of the model.  Child classes
    // will need to observe for the observation coefficients.
    void observe_state(const ConstVectorView &then,
                       const ConstVectorView &now,
                       int time_now) override;

    //----------------------------------------------------------------------
    // Sizes of things.  The state dimension and the state_error_dimension are
    // both just the number of factors.
    uint state_dimension() const override {
      return component_models_.state_dimension();}
    uint state_error_dimension() const override {
      return component_models_.state_error_dimension();}

    void simulate_state_error(RNG &rng, VectorView eta, int t) const override;
    void simulate_initial_state(RNG &rng, VectorView eta) const override;

    //--------------------------------------------------------------------------
    // Model matrices.
    Ptr<SparseMatrixBlock> state_transition_matrix(int t) const override {
      Ptr<SparseMatrixBlock> ans = component_models_.state_transition_matrix(t);
      return ans;
    }

    Ptr<SparseMatrixBlock> state_variance_matrix(int t) const override {
      return component_models_.state_variance_matrix(t);
    }

    // The state error expander matrix is an identity matrix of the same
    // dimension as the state_transition_matrix, so we just return that matrix.
    Ptr<SparseMatrixBlock> state_error_expander(int t) const override {
      return component_models_.state_error_expander(t);
    }

    // Because the error expander is the identity, the state variance matrix and
    // the state error variance are the same thing.
    Ptr<SparseMatrixBlock> state_error_variance(int t) const override {
      return component_models_.state_error_variance(t);
    }

    //--------------------------------------------------------------------------
    // Initial state mean and variance.  These should have been set by the
    // component models.
    Vector initial_state_mean() const override;
    SpdMatrix initial_state_variance() const override;

    //--------------------------------------------------------------------------
    // Tools for working with the EM algorithm and numerical optimization.
    // These are not currently implemented.
    void update_complete_data_sufficient_statistics(
        int t, const ConstVectorView &state_error_mean,
        const ConstSubMatrix &state_error_variance) override;

    void increment_expected_gradient(
        VectorView gradient, int t, const ConstVectorView &state_error_mean,
        const ConstSubMatrix &state_error_variance) override;

    const Vector &regression_coefficients() const;

   protected:
    // The observation coefficients that would be produced by the
    // component_models_ if they were being used in a scalar model.
    SparseVector component_observation_coefficients(int t) const;

    // Remove any parameter observers that were set by the constructor.
    void remove_observers();

   private:
    //-------------------------------------------------------------------------
    // Data section
    //-------------------------------------------------------------------------
    int nseries_;

    // The individual elements of state (e.g. local linear trend, seasonality,
    // etc).
    StateSpaceUtils::StateModelVector<StateModel> component_models_;

  };

  //===========================================================================
  //
  class ConditionallyIndependentScalarStateModelMultivariateAdapter
      : public ScalarStateModelMultivariateAdapter {
   public:

    using Adapter = ConditionallyIndependentScalarStateModelMultivariateAdapter;
    using Host = ConditionallyIndependentMultivariateStateSpaceModelBase;
    using Base = ScalarStateModelMultivariateAdapter;

    ConditionallyIndependentScalarStateModelMultivariateAdapter(
        Host *host, int nseries);

    Adapter * clone() const override;

    void clear_data() override;

    void add_state(const Ptr<StateModel> &state_model) override;

    Ptr<SparseMatrixBlock> observation_coefficients(
        int t,
        const Selector &observed) const override;

    // Observe the state for observation part of the model, calling the base
    // class 'observe state' to observe the state for the transition equation.
    void observe_state(const ConstVectorView &then,
                       const ConstVectorView &now,
                       int time_now) override;


   private:
    // The host is the model object in which *this is a state component.  The
    // host is needed for this model to properly implement observe_state,
    // because the coefficient models needs to subtract away the contributions
    // from other state models.
    Host *host_;

    // raw_observation_coefficients_ contains one element per series.  Each
    // element is a set of regression coefficients giving weight

    std::vector<Ptr<WeightedRegSuf>> sufficient_statistics_;

    Vector observation_coefficient_slopes_;

    Ptr<DenseSparseRankOneMatrixBlock> observation_coefficients_;
    Ptr<EmptyMatrix> empty_;
  };

}  // namespace BOOM

#endif  //  BOOM_STATE_SPACE_SCALAR_STATE_MODEL_ADAPTER_HPP_
