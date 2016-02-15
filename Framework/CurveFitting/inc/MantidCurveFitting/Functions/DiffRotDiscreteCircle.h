#ifndef MANTID_DIFFROTDISCRETECIRCLE_H_
#define MANTID_DIFFROTDISCRETECIRCLE_H_

#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/ImmutableCompositeFunction.h"
#include "DeltaFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
@author Jose Borreguero, NScD
@date December 02 2013

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

/* Class representing the elastic portion of DiffRotDiscreteCircle
 * Contains a Delta Dirac.
 */
class DLLExport ElasticDiffRotDiscreteCircle : public DeltaFunction {
public:
  /// Constructor
  ElasticDiffRotDiscreteCircle();

  /// overwrite IFunction base class methods
  std::string name() const override { return "ElasticDiffRotDiscreteCircle"; }

  const std::string category() const override { return "QuasiElastic"; }

  /// overwrite IFunction base class method, which declare function parameters
  void init() override;

  /// A rescaling of the peak intensity
  double HeightPrefactor() const override;
};

/* Class representing the inelastic portion of DiffRotDiscreteCircle
 * Contains a linear combination of Lorentzians.
 */
class DLLExport InelasticDiffRotDiscreteCircle : public API::ParamFunction,
                                                 public API::IFunction1D {
public:
  /// Constructor
  InelasticDiffRotDiscreteCircle();

  std::string name() const override { return "InelasticDiffRotDiscreteCircle"; }

  const std::string category() const override { return "QuasiElastic"; }

  void init() override;

protected:
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;

private:
  /// Cache Q values from the workspace
  void setWorkspace(boost::shared_ptr<const API::Workspace> ws) override;

  const double m_hbar; // Plank constant, in meV*THz (or ueV*PHz)

  std::vector<double> m_qValueCache; // List of calculated Q values
};

/* Class representing the dynamics structure factor of a particle undergoing
 * discrete jumps on N-sites evenly distributed in a circle. The particle can
 * only
 * jump to neighboring sites. This is the most common type of discrete
 * rotational diffusion in a circle.
 */
class DLLExport DiffRotDiscreteCircle : public API::ImmutableCompositeFunction {
public:

  std::string name() const override { return "DiffRotDiscreteCircle"; }

  const std::string category() const override { return "QuasiElastic"; }

  virtual int version() const { return 1; }

  void init() override;

  /// Propagate an attribute to member functions
  virtual void trickleDownAttribute(const std::string &name);

  /// Override parent definition
  virtual void declareAttribute(const std::string &name,
                                const API::IFunction::Attribute &defaultValue);

  /// Override parent definition
  void setAttribute(const std::string &attName, const Attribute &att) override;

private:
  boost::shared_ptr<ElasticDiffRotDiscreteCircle> m_elastic;

  boost::shared_ptr<InelasticDiffRotDiscreteCircle> m_inelastic;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_DIFFROTDISCRETECIRCLE_H_*/
