/******************************************************************************
 * Python module wrapper for the WorkspaceCreationHelper methods
 ******************************************************************************/
#include <boost/python/docstring_options.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/module.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_value_policy.hpp>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/PeaksWorkspace.h"


BOOST_PYTHON_MODULE(WorkspaceCreationHelper)
{
  using namespace boost::python;
  using namespace WorkspaceCreationHelper;

  // Doc string options - User defined, python arguments, C++ call signatures
  docstring_options docstrings(true, true, false);

  // 2D workspaces
  using namespace Mantid::DataObjects;
  register_ptr_to_python<boost::shared_ptr<Workspace2D> >();
  class_<Workspace2D, bases<Mantid::API::MatrixWorkspace>,boost::noncopyable>("Workspace2D", no_init);
  def("create2DWorkspaceWithFullInstrument", &create2DWorkspaceWithFullInstrument);

  // EventWorkspaces
  register_ptr_to_python<boost::shared_ptr<EventWorkspace> >();
  class_<EventWorkspace, bases<Mantid::API::IEventWorkspace>,boost::noncopyable>("EventWorkspace", no_init);


  def("CreateEventWorkspace", (EventWorkspace_sptr (*)())&CreateEventWorkspace);
  def("CreateEventWorkspace2", &CreateEventWorkspace2);

  // Peaks workspace
  register_ptr_to_python<boost::shared_ptr<PeaksWorkspace> >();
  class_<PeaksWorkspace, bases<Mantid::API::IPeaksWorkspace>,boost::noncopyable>("PeaksWorkspace", no_init);

  def("createPeaksWorkspace", &createPeaksWorkspace);
}
