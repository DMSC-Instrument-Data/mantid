#ifndef MANTID_CRYSTAL_SAVEHKL_H_
#define MANTID_CRYSTAL_SAVEHKL_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid {
namespace Crystal {

/** Save a PeaksWorkspace to a Gsas-style ASCII .hkl file.
 *
 * @author Vickie Lynch, SNS
 * @date 2011-09-28
 */

class DLLExport SaveHKL : public API::Algorithm {
public:
  SaveHKL();
  ~SaveHKL() override;

  /// Algorithm's name for identification
  const std::string name() const override { return "SaveHKL"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Save a PeaksWorkspace to a ASCII .hkl file.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Crystal\\DataHandling;DataHandling\\Text";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  double absor_sphere(double &twoth, double &wl, double &tbar);
  double m_smu;    // in 1/cm
  double m_amu;    // in 1/cm
  double m_radius; // in cm
  double m_power_th;
  double spectrumCalc(double TOF, int iSpec,
                      std::vector<std::vector<double>> time,
                      std::vector<std::vector<double>> spectra, size_t id);
  DataObjects::PeaksWorkspace_sptr ws;
  void sizeBanks(std::string bankName, int &nCols, int &nRows);
};

} // namespace Mantid
} // namespace Crystal

#endif /* MANTID_CRYSTAL_SAVEHKL_H_ */
