#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cmath>
#include <vector>
#include <map>
#include <list>
#include <stack>
#include <string>
#include <algorithm>
#include "AuxException.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLread.h"
#include "XMLcollect.h"
#include "IndexIterator.h"
#include "FileReport.h"
#include "GTKreport.h"
#include "OutputLog.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "support.h"
#include "BaseVisit.h"
#include "Surface.h"
#include "Plane.h"
#include "Cylinder.h"
#include "Cone.h"
#include "General.h"
#include "Sphere.h"
#include "Torus.h"
#include "surfaceFactory.h"

namespace MonteCarlo
{

surfaceFactory* surfaceFactory::FOBJ(0);


surfaceFactory*
surfaceFactory::Instance() 
  /*!
    Effective new command / this command 
    \returns Single instance of surfaceFactory
  */
{
  if (!FOBJ)
    {
      FOBJ=new surfaceFactory();
    }
  return FOBJ;
}

surfaceFactory::surfaceFactory() 
  /*!
    Constructor
  */
{
  registerSurface();
}

surfaceFactory::surfaceFactory(const surfaceFactory& A)  :
  ID(A.ID)
  /*! 
    Copy constructor 
    \param A :: Object to copy
  */
{
  MapType::const_iterator vc;
  for(vc=A.SGrid.begin();vc!=A.SGrid.end();vc++)
    SGrid.insert(MapType::value_type
		 (vc->first,vc->second->clone()));

}
  
surfaceFactory::~surfaceFactory()
  /*!
    Destructor removes memory for atom/cluster list
  */
{
  MapType::iterator vc;
  for(vc=SGrid.begin();vc!=SGrid.end();vc++)
    delete vc->second;
}

void
surfaceFactory::registerSurface()
  /*!
    Register tallies to be used
  */
{
  SGrid["Plane"]=new Plane;  
  SGrid["Cylinder"]=new Cylinder;  
  SGrid["Cone"]=new Cone;  
  SGrid["Torus"]=new Torus;  
  SGrid["General"]=new General;  
  SGrid["Sphere"]=new Sphere;  
  
  ID['c']="Cylinder";
  ID['k']="Cone";
  ID['g']="General";
  ID['p']="Plane";
  ID['s']="Sphere";
  ID['t']="torus";
  return;
}
  
Surface*
surfaceFactory::createSurface(const std::string& Key) const
  /*!
    Creates an instance of tally
    given a valid key. 
    
    \param Key :: Item to get 
    \throw InContainerError for the key if not found
    \return new tally object.
  */    
{
  MapType::const_iterator vc;
  vc=SGrid.find(Key);
  if (vc==SGrid.end())
    {
      throw ColErr::InContainerError<std::string>
	(Key,"surfaceFactory::createSurface");
    }
  Surface *X= vc->second->clone();
  return X;
}

Surface*
surfaceFactory::createSurfaceID(const std::string& Key) const
  /*!
    Creates an instance of tally
    given a valid key. 
    
    \param Key :: Form of first ID
    \throw InContainerError for the key if not found
    \return new tally object.
  */    
{
  std::map<char,std::string>::const_iterator mc;
  
  mc=(Key.empty()) ? ID.end() : ID.find(tolower(Key[0]));
  if (mc==ID.end())
    {
      throw ColErr::InContainerError<std::string>
	(Key,"surfaceFactory::createSurfaceID");
    }
  
  return createSurface(mc->second);
}

}  // NAMESPACE MonteCarlo 
 
 

