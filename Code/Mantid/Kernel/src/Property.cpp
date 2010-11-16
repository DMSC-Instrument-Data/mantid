#include "MantidKernel/Property.h"
#include "MantidKernel/Exception.h"
#include <string>
#include <sstream>

namespace Mantid
{
namespace Kernel
{

/** Constructor
 *  @param name The name of the property
 *  @param type The type of the property
 *  @param direction Whether this is a Direction::Input, Direction::Output or Direction::InOut (Input & Output) property
 */
Property::Property( const std::string &name, const std::type_info &type, const unsigned int direction ) :
  m_name( name ),
  m_documentation( "" ),
  m_typeinfo( &type ),
  m_direction( direction ),
  m_units("")
{
    // Make sure a random int hasn't been passed in for the direction
    // Property & PropertyWithValue destructors will be called in this case
    if (m_direction > 2) throw std::out_of_range("direction should be a member of the Direction enum");
}

/// Copy constructor
Property::Property( const Property& right ) :
  m_name( right.m_name ),
  m_documentation( right.m_documentation ),
  m_typeinfo( right.m_typeinfo ),
  m_direction( right.m_direction ),
  m_units( right.m_units)
{
}

/// Virtual destructor
Property::~Property()
{
}

/** Copy assignment operator. Does nothing.
* @param right The right hand side value
* @return pointer to this
*/
Property& Property::operator=( const Property& right )
{
  (void)right; // Get rid of compiler warning but we need the argument for doxygen
  return *this;
}

/** Get the property's name
 *  @return The name of the property
 */
const std::string& Property::name() const
{
  return m_name;
}

/** Get the property's documentation string
 *  @return The documentation string
 */
const std::string& Property::documentation() const
{
  return m_documentation;
}

/** Get the property type_info
 *  @return The type of the property
 */
const std::type_info* Property::type_info() const
{
  return m_typeinfo;
}

/** Returns the type of the property as a string.
 *  Note that this is implementation dependent.
 *  @return The property type
 */
const std::string Property::type() const
{
  return Mantid::Kernel::getUnmangledTypeName(*m_typeinfo);
}

/** Overridden functions checks whether the property has a valid value.
 *  
 *  @return empty string ""
 */
std::string Property::isValid() const
{
  // the no error condition
  return "";
}
/**
* Whether to remember this property input
* @return whether to remember this property's input
*/
bool Property::remember() const
{
  return true;
}

/** Sets the property's (optional) documentation string
 *  @param documentation The string containing the descriptive comment
 */
void Property::setDocumentation( const std::string& documentation )
{
  m_documentation = documentation;
}

/** Returns the set of valid values for this property, if such a set exists.
 *  If not, it returns an empty set.
 * @return the set of valid values for this property or an empty set
 */
std::set<std::string> Property::allowedValues() const
{
  return std::set<std::string>();
}

/// Create a PropertyHistory object representing the current state of the Property.
const PropertyHistory Property::createHistory() const
{
  return PropertyHistory(this->name(),this->value(),this->type(),this->isDefault(),this->direction());
}

//-------------------------------------------------------------------------------------------------
/** Return the size of this property.
 * Single-Value properties return 1.
 * TimeSeriesProperties return the # of entries.
 * @return the size of the property
 */
int Property::size() const
{
  return 1;
}

//-------------------------------------------------------------------------------------------------
/** Returns the units of the property, if any, as a string.
 * Units are optional, and will return empty string if they have
 * not been set before.
 * @return the property's units
 */
std::string Property::units() const
{
  return m_units;
}

//-------------------------------------------------------------------------------------------------
/** Sets the units of the property, as a string. This is optional.
 *
 * @param unit string to set for the units.
 */
void Property::setUnits(std::string unit)
{
  m_units = std::string(unit); //force the copy constructor
}




//-------------------------------------------------------------------------------------------------
/** Add to the property.
 * @param rhs the property to get more information
 * @return the augmented property
 * @throw NotImplementedError always, since this should have been overridden
 */
Property& Property::operator+=( Property * rhs )
{
  (void) rhs; //Avoid compiler warning
  std::stringstream msg;
  msg << "Property object '" << m_name << "' of type '" << type() << "' has not implemented a operator+=() method.";
  throw Exception::NotImplementedError(msg.str());
}


//-------------------------------------------------------------------------------------------------
/** Filter out a property by time. Will be overridden by TimeSeriesProperty (only)
 * @param start the beginning time to filter from
 * @param stop the ending time to filter to
 * */
void Property::filterByTime(const Kernel::dateAndTime start, const Kernel::dateAndTime stop)
{
  (void) start; //Avoid compiler warning
  (void) stop; //Avoid compiler warning
  //Do nothing in general
  return;
}


//-----------------------------------------------------------------------------------------------
/** Split a property by time. Will be overridden by TimeSeriesProperty (only)
 * For any other property type, this does nothing.
 * @param splitter time splitter
 * @param outputs holder for splitter output
 */
void Property::splitByTime(TimeSplitterType& splitter, std::vector< Property * > outputs) const
{
  (void) splitter; //Avoid compiler warning
  (void) outputs; //Avoid compiler warning
  return;
}




} // namespace Kernel
} // namespace Mantid
