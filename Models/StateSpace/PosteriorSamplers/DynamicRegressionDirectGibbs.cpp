/*
  Copyright (C) 2005-2020 Steven L. Scott

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

#include "Models/StateSpace/PosteriorSamplers/DynamicRegressionDirectGibbs.hpp"

namespace BOOM {

  namespace {
    using DRDGS = DynamicRegressionDirectGibbsSampler;
  }

  void DRDGS::draw() {
    draw_inclusion_indicators();
    draw_coefficients_given_inclusion();
    draw_residual_variance();
    draw_state_innovation_variance();
    draw_transition_probabilities();
  }

  double DRDGS::logpri() const {
    report_error("Not implemented.");
    return -1;
  }

  void DRDGS::draw_inclusion_indicators() {
    for (j = 0; j < model_->xdim(); ++j) {

      // Can we do forward-backward here?

      // Do Direct Gibbs first and see where the trouble spots are.


    }
  }

  void DRDGS::draw_coefficients_given_inclusion() {
  }

  void DRDGS::draw_residual_variance() {
  }

  void DRDGS::draw_state_innovation_variance() {
  }

  void DRDGS::draw_transition_probabilities() {
    report_error("Not implemented.");
  }


}  // namespace BOOM
