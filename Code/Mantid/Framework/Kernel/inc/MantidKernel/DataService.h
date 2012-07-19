#ifndef MANTID_KERNEL_DATASERVICE_H_
#define MANTID_KERNEL_DATASERVICE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <boost/shared_ptr.hpp>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <algorithm>

#include <Poco/NotificationCenter.h>
#include <Poco/Notification.h>
#include <Poco/ScopedLock.h>
#include <Poco/Mutex.h>

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ConfigService.h"

namespace Mantid
{
namespace Kernel
{
/** DataService is the base class for storing DataObjects. It stores instances of DataObjects
    (Workspace, Instrument, MappingTables,...). This is a templated class, implemented as a
    singleton. For simplicity and naming conventions, specialized classes must be constructed. The specialized
    classes (see example MantidAPI/InstrumentDataService.h) must simply :
    1) call the BaseClass constructor with the Name of the service
    2) Support the SingletonHolder templated class.
    This is the primary data service that  the users will interact with either through writing scripts or directly
    through the API. It is implemented as a singleton class.

    @author Laurent C Chapon, ISIS, Rutherford Appleton Laboratory
    @date 30/05/2008

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template <typename T>
class DLLExport DataService
{
private:
  /// Typedef for the map holding the names of and pointers to the data objects
  typedef std::map<std::string,  boost::shared_ptr<T> > svcmap;
  /// Iterator for the data store map
  typedef typename svcmap::iterator svc_it;
  /// Const iterator for the data store map
  typedef typename svcmap::const_iterator svc_constit;

public:

    /// Class for named object notifications
    class NamedObjectNotification: public Poco::Notification
    {
    public:
      NamedObjectNotification(const std::string& name) :
        Poco::Notification(), m_name(name) {}

      /// Returns the name of the object
      std::string object_name()const{return m_name;}
    private:
      std::string m_name;///< object's name
    };

    /// Base class for DataService notifications that also stores a pointer to the object
    class DataServiceNotification : public NamedObjectNotification
    {
    public:
        /// Constructor
        DataServiceNotification(const std::string& name, const boost::shared_ptr<T> obj)
          : NamedObjectNotification(name), m_object(obj){}
        /// Returns the const pointer to the object concerned or 0 if it is a general notification
        const boost::shared_ptr<T> object()const{return m_object;}
    private:
        boost::shared_ptr<T> m_object;///< shared pointer to the object
    };

    /// AddNotification is sent after an new object is added to the data service.
    /// name() and object() return name and pointer to the new object.
    class AddNotification: public DataServiceNotification
    {
    public:
        AddNotification(const std::string& name,const boost::shared_ptr<T> obj):DataServiceNotification(name,obj){}///< Constructor
    };


    /// BeforeReplaceNotification is sent before an object is replaced in the addOrReplace() function.
    class BeforeReplaceNotification: public DataServiceNotification
    {
    public:
        /** Constructor.

            @param name :: The name of the replaced object
            @param obj ::  The pointer to the old object
            @param new_obj :: The pointer to the new object

            Both old and new objects are guaranteed to exist when an observer receives the notification.
        */
        BeforeReplaceNotification(const std::string& name, const boost::shared_ptr<T> obj,const boost::shared_ptr<T> new_obj):DataServiceNotification(name,obj),m_new_object(new_obj){}
        const boost::shared_ptr<T> new_object()const{return m_new_object;}///< Returns the pointer to the new object.
    private:
        boost::shared_ptr<T> m_new_object;///< shared pointer to the object
    };

    /// AfterReplaceNotification is sent after an object is replaced in the addOrReplace() function.
    class AfterReplaceNotification: public DataServiceNotification
    {
    public:
        /** Constructor.
          * @param name :: The name of the replaced object
          *  @param new_obj :: The pointer to the new object
          *  Only new objects are guaranteed to exist when an observer receives the notification.
        */
        AfterReplaceNotification(const std::string& name, const boost::shared_ptr<T> new_obj):DataServiceNotification(name,new_obj) {}
    };

    /// PreDeleteNotification is sent before an object is deleted from the data service.
    /// name() and object() return name and pointer to the object being deleted.
    /// The object still exists when the notification is received by an observer.
    class PreDeleteNotification: public DataServiceNotification
    {
    public:
      /// Constructor
      PreDeleteNotification(const std::string& name,const boost::shared_ptr<T> obj):DataServiceNotification(name,obj){}
    };

    /// PostDeleteNotification is sent after an object is deleted from the data service.
    /// name() returns the name of the object being deleted.
    /// The object no longer exists when the notification is received by an observer.
    class PostDeleteNotification: public NamedObjectNotification
    {
    public:
      /// Constructor
      PostDeleteNotification(const std::string& name):NamedObjectNotification(name) {}
    };

    /// Clear notification is sent when the service is cleared
    class ClearNotification: public NamedObjectNotification
    {
    public:
      ///Constructor
      ClearNotification() :  NamedObjectNotification("") {}
    };
    /// Rename notification is sent from Renameworkspaces algorithm after a workspace is renamed
    class RenameNotification: public NamedObjectNotification
    {
    public:
        /// Constructor
      RenameNotification(const std::string& name,const std::string& newwsname) :
        NamedObjectNotification(name), m_outwsname(newwsname){}

      ///name of the new workspace
      const std::string& new_objectname()const{return m_outwsname;}
    private:
      std::string m_outwsname; ///< output workspace name
    };

  //--------------------------------------------------------------------------
  /** Add an object to the service
   * @param name :: name of the object
   * @param Tobject :: shared pointer to object to add
   * @throw std::runtime_error if name is empty
   * @throw std::runtime_error if name exists in the map
   */
  virtual void add( const std::string& name, const boost::shared_ptr<T>& Tobject)
  {
    // Don't permit an empty name for the workspace
    if (name.empty())
    {
      std::string error="Add Data Object with empty name";
      g_log.debug() << error << std::endl;
      throw std::runtime_error(error);
    }

    // Make DataService access thread-safe
    m_mutex.lock();

    // At the moment, you can't overwrite a workspace (i.e. pass in a name
    // that's already in the map with a pointer to a different workspace).
    // Also, there's nothing to stop the same workspace from being added
    // more than once with different names.
    if ( ! datamap.insert(typename svcmap::value_type(name, Tobject)).second)
    {
      std::string error=" add : Unable to insert Data Object : '"+name+"'";
      g_log.error(error);
      m_mutex.unlock();
      throw std::runtime_error(error);
    }
    else
    {
      g_log.debug() << "Add Data Object " << name << " successful" << std::endl;
      m_mutex.unlock();

      //check the workspace invisible option set
      std::string name_startswith=name.substr(0,2);
      if(!isInvisbleWorkspaceOptionsSet())
      {
        //if the name of workspace starts with __
        if(!name_startswith.compare("__"))
        {
          return;
        }
      }
     
      notificationCenter.postNotification(new AddNotification(name,Tobject));
    }
    return;
  }

  //--------------------------------------------------------------------------
  /** Add or replace an object to the service.
   * Does NOT throw if the name was already used.
   *
   * @param name :: name of the object
   * @param Tobject :: shared pointer to object to add
   * @throw std::runtime_error if name is empty
   */
  virtual void addOrReplace( const std::string& name, const boost::shared_ptr<T>& Tobject)
  {
    // Make DataService access thread-safe
    m_mutex.lock();

    //find if the Tobject already exists
    std::string foundName;
    svc_it it = this->findNameWithCaseSearch(name, foundName);
    if (it!=datamap.end())
    {
      g_log.debug("Data Object '"+ foundName +"' replaced in data service.\n");
      m_mutex.unlock();

      notificationCenter.postNotification(new BeforeReplaceNotification(name,it->second,Tobject));

      m_mutex.lock();
      datamap[foundName] = Tobject;

      std::string name_startswith=name.substr(0,2);
      //check the workspace invisible option set
      if(!isInvisbleWorkspaceOptionsSet())
      {
        //if the name of workspace starts with __
        if(!name_startswith.compare("__"))
        {
          m_mutex.unlock();
          return;
        }
      }
      m_mutex.unlock();
      notificationCenter.postNotification(new AfterReplaceNotification(name,Tobject));
    }
    else
    {
      // Avoid double-locking
      m_mutex.unlock();
      DataService::add(name,Tobject);
    }
    return;
  }

  //--------------------------------------------------------------------------
  /** Remove an object from the service.
   * @param name :: name of the object */
  void remove( const std::string& name)
  {
    // Make DataService access thread-safe
    m_mutex.lock();

    std::string foundName;
    svc_it it = this->findNameWithCaseSearch(name, foundName);
    if (it==datamap.end())
    {
      g_log.warning(" remove '" + name + "' cannot be found");
      m_mutex.unlock();
      return;
    }
    m_mutex.unlock();
    notificationCenter.postNotification(new PreDeleteNotification(foundName,it->second));
    m_mutex.lock();

    g_log.information("Data Object '"+ foundName +"' deleted from data service.");
    datamap.erase(it);

    m_mutex.unlock();
    notificationCenter.postNotification(new PostDeleteNotification(foundName));
    return;
  }

  //--------------------------------------------------------------------------
  /** Rename an object within the service.
   * @param oldName :: The old name of the object 
   * @param newName :: The new name of the object 
   */
  void rename( const std::string& oldName, const std::string& newName)
  {
    // Make DataService access thread-safe
    m_mutex.lock();

    std::string foundName;
    svc_it it = this->findNameWithCaseSearch(oldName, foundName);
    if (it==datamap.end())
    {
      g_log.warning(" rename '" + oldName + "' cannot be found");
      m_mutex.unlock();
      return;
    }

    // delete the object with the old name
    auto object = it->second;
    datamap.erase( it );

    // if there is another object which has newName delete it
    it = datamap.find( newName );
    if ( it != datamap.end() )
    {
      datamap.erase( it );
    }

    // insert the old object with the new name
    if ( ! datamap.insert(typename svcmap::value_type(newName, object)).second )
    {
      std::string error=" add : Unable to insert Data Object : '"+newName+"'";
      g_log.error(error);
      m_mutex.unlock();
      throw std::runtime_error(error);
    }
    g_log.information("Data Object '"+ foundName +"' renamed to '" + newName + "'");

    m_mutex.unlock();
    notificationCenter.postNotification(new RenameNotification(oldName, newName));
    return;
  }

  //--------------------------------------------------------------------------
  /// Empty the service
  void clear()
  {
    // Make DataService access thread-safe
    m_mutex.lock();

    datamap.clear();
    m_mutex.unlock();
    notificationCenter.postNotification(new ClearNotification());
    g_log.debug() << typeid(this).name() << " cleared.\n";
  }

  //--------------------------------------------------------------------------
  /** Get a shared pointer to a stored data object
   * @param name :: name of the object */
  boost::shared_ptr<T> retrieve( const std::string& name) const
  {
    // Make DataService access thread-safe
    Poco::Mutex::ScopedLock _lock(m_mutex);

    std::string foundName;
    svc_it it = this->findNameWithCaseSearch(name, foundName);
    if (it != datamap.end())
    {
      return it->second;
    }
    else
    {
      throw Kernel::Exception::NotFoundError("Data Object",name);
    }
  }

  /// Check to see if a data object exists in the store
  bool doesExist(const std::string& name) const
  {
    // Make DataService access thread-safe
    Poco::Mutex::ScopedLock _lock(m_mutex);

    std::string foundName;
    svc_it it = this->findNameWithCaseSearch(name, foundName);
    if (it!=datamap.end())
        return true;
    return false;
  }

  /// Return the number of objects stored by the data service
  size_t size() const
  {
    return datamap.size();
  }

  /// Get a vector of the names of the data objects stored by the service
  std::set<std::string> getObjectNames() const
  {
    // Make DataService access thread-safe
    Poco::Mutex::ScopedLock _lock(m_mutex);

    std::set<std::string> names;
    svc_constit it;
    for( it = datamap.begin(); it != datamap.end(); ++it)
    {
      names.insert(it->first);
    }
    return names;
  }

  /// returns true if the InvisibleWorkspaces option set
  bool isInvisbleWorkspaceOptionsSet()
  {
    std::string isvisblews = Mantid::Kernel::ConfigService::Instance().getString("MantidOptions.InvisibleWorkspaces");
    return ((!isvisblews.compare("1"))?true:false);
  }

  /// Sends notifications to observers. Observers can subscribe to notificationCenter
  /// using Poco::NotificationCenter::addObserver(...)
  ///@return nothing
  Poco::NotificationCenter notificationCenter;

protected:
  /// Protected constructor (singleton)
  DataService(const std::string& name):svc_name(name),g_log(Kernel::Logger::get(svc_name)){}
  virtual ~DataService(){}

private:
  /// Private, unimplemented copy constructor
  DataService(const DataService&);
  /// Private, unimplemented copy assignment operator
  DataService& operator=(const DataService&);

  /// Recursive mutex to avoid simultaneous access or notifications
  mutable Poco::Mutex m_mutex;


  /**
   * Find a name in the map and return an iterator pointing to it. This takes
   * the string and if it does not find it then it looks for a match disregarding
   * the case (const version)
   * @param name The string name to search for
   * @param foundName [Output] Stores the name here if one was found. It will be empty if not
   * @returns An iterator pointing at the element or the end iterator
   */
  svc_it findNameWithCaseSearch(const std::string & name, std::string &foundName) const
  {
    const svcmap& constdata = this->datamap;
    svcmap& data = const_cast<svcmap&>(constdata);
    if(name.empty()) return data.end();

    //Exact match
    foundName = name;
    svc_it match = data.find(name);
    if( match != data.end() ) return match;

    // Try UPPER case
    std::transform(foundName.begin(), foundName.end(), foundName.begin(),toupper);
    match = data.find(foundName);
    if( match != data.end() ) return match;

    // Try lower case
    std::transform(foundName.begin(), foundName.end(), foundName.begin(),tolower);
    match = data.find(foundName);
    if( match != data.end() ) return match;

    // Try Sentence case
    foundName = name;
    // Upper cases the first letter
    std::transform(foundName.begin(), foundName.begin() + 1, foundName.begin(),toupper);
    match = data.find(foundName);
    if( match == data.end() ) foundName = "";
    return match;
  }

  /// DataService name. This is set only at construction. DataService name should be provided when construction of derived classes
  std::string svc_name;
  
  /// Map of objects in the data service
  svcmap datamap;

  /// Static reference to the logger for this DataService
  Logger& g_log;

}; // End Class Data service

} // Namespace Kernel
} // Namespace Mantid

#endif /*MANTID_KERNEL_DATASERVICE_H_*/
