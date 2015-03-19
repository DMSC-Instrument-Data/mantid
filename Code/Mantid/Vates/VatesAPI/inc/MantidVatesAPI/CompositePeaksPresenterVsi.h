#ifndef MANTID_VATES_COMPOSITE_PEAKS_PRESENTER_VSI_H
#define MANTID_VATES_COMPOSITE_PEAKS_PRESENTER_VSI_H

#include "MantidKernel/System.h"
#include "MantidVatesAPI/PeaksPresenterVsi.h"
#include "MantidAPI/PeakTransform.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidVatesAPI/ViewFrustum.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include <vector>
#include <string>

namespace Mantid
{
namespace VATES
{
class DLLExport CompositePeaksPresenterVsi : public PeaksPresenterVsi
{
public:
  CompositePeaksPresenterVsi();
  ~CompositePeaksPresenterVsi();
  virtual Mantid::API::IPeaksWorkspace_sptr getPeaksWorkspace(){throw std::runtime_error("The composite peaks presenter has no single peaks workspace.");}
  std::vector<Mantid::API::IPeaksWorkspace_sptr> getPeaksWorkspaces();
  virtual std::vector<bool> getViewablePeaks();
  virtual void updateViewFrustum(ViewFrustum frustum);
  virtual std::string getFrame();
  virtual std::string getPeaksWorkspaceName(){throw std::runtime_error("The composite peaks presenter has no peaks workspace");} 
  std::vector<std::string> getPeaksWorkspaceNames();
  virtual void getPeaksInfo(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace, int row, Mantid::Kernel::V3D& position, double& radius);
  void addPresenter(PeaksPresenterVsi_sptr presenter);
  std::map<std::string, std::vector<bool>> getInitializedViewablePeaks();
  void removePresenter(std::string peaksWorkspaceName);
  void updateWorkspaces(std::vector<std::string> peaksWorkspaceNames);
  bool hasPeaks();
private:
  /// The list of presenters
  std::vector<PeaksPresenterVsi_sptr> m_peaksPresenters;
};
}
}
#endif