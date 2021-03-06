#ifndef MANTID_MDALGORITHMS_WEIGHTEDMEANMD_H_
#define MANTID_MDALGORITHMS_WEIGHTEDMEANMD_H_

#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/BinaryOperationMD.h"

namespace Mantid {
namespace MDAlgorithms {

/** WeightedMeanMD : Find the weighted mean of two MDWorkspaces

  @date 2012-06-26

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport WeightedMeanMD : public BinaryOperationMD {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "WeightedMeanMD"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Find weighted mean of two MDHistoWorkspaces.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"WeightedMean"};
  }

private:
  /// Is the operation commutative?
  bool commutative() const override;

  /// Check the inputs and throw if the algorithm cannot be run
  void checkInputs() override;

  /// Run the algorithm with an MDEventWorkspace as output
  void execEvent() override;

  /// Run the algorithm with a MDHisotWorkspace as output and operand
  void execHistoHisto(
      Mantid::DataObjects::MDHistoWorkspace_sptr out,
      Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) override;

  /// Run the algorithm with a MDHisotWorkspace as output, scalar and operand
  void execHistoScalar(
      Mantid::DataObjects::MDHistoWorkspace_sptr out,
      Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar) override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_WEIGHTEDMEANMD_H_ */