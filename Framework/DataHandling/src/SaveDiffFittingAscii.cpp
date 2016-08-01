
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveDiffFittingAscii.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "Poco/File.h"
#include <boost/tokenizer.hpp>
#include <fstream>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace Mantid::API;

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveDiffFittingAscii)

using namespace Kernel;
using namespace API;

/// Empty constructor
SaveDiffFittingAscii::SaveDiffFittingAscii()
    : Mantid::API::Algorithm(), m_sep(','), m_endl('\n') {}

/// Initialisation method.
void SaveDiffFittingAscii::init() {

  declareProperty(make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The name of the workspace containing the data you want to "
                  "save to a TBL file.");

  // Declare required parameters, filename with ext {.his} and input
  // workspace
  const std::vector<std::string> exts{".txt", ".csv", ""};
  declareProperty(Kernel::make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::Save, exts),
                  "The filename to use for the saved data");

  declareProperty("RunNumber", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "Run number of the focused file used to generate the "
                  "parameters table workspace.");

  declareProperty("Bank", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "Bank of the focused file used to generate the parameters.");

  std::vector<std::string> formats;

  formats.push_back("AppendToExistingFile");
  formats.push_back("WriteGroupWorkspace");
  formats.push_back("OverwriteFile");
  declareProperty(
      "OutFormat", "AppendToExistingFile",
      boost::make_shared<Kernel::StringListValidator>(formats),
      "Append data to existing file or save multiple table workspaces"
      "in a group workspace");
}

/**
*   Executes the algorithm.
*/
void SaveDiffFittingAscii::exec() {
  // Get the workspace

  // Process properties

  // Retrieve the input workspace
  /// Workspace
  const ITableWorkspace_sptr tbl_ws = getProperty("InputWorkspace");
  if (!tbl_ws)
    throw std::runtime_error("Please provide an input workspace to be saved.");

  const std::string filename = getProperty("Filename");

  Poco::File pFile = (filename);
  bool exist = pFile.exists();
  g_log.error() << "exist " << exist << std::endl; // @shahroz

  // Initialize the file stream
  std::string outFormat = getProperty("OutFormat");

  bool appendToFile = false;
  if (outFormat == "AppendToExistingFile" || outFormat == "WriteGroupWorkspace")
    appendToFile = true;

  std::ofstream file(filename.c_str(),
                     (appendToFile ? std::ios::app : std::ios::out));

  if (!file) {
    throw Exception::FileError("Unable to create file: ", filename);
  }

  if (exist && !appendToFile) {
    g_log.warning() << "File " << filename << " exists and will be overwritten."
                    << "\n";
  }

  if (exist && appendToFile) {
    file << "\n";
    appendToFile = false;
  }

  const std::string runNum = getProperty("RunNumber");
  const std::string bank = getProperty("Bank");
  writeInfo(runNum, bank, file);

  // write header
  std::vector<std::string> columnHeadings = tbl_ws->getColumnNames();
  writeHeader(columnHeadings, file);

  // write out the data form the table workspace
  size_t columnSize = columnHeadings.size();
  writeData(tbl_ws, file, columnSize);
}

void SaveDiffFittingAscii::writeInfo(const std::string &runNumber,
                                     const std::string &bank,
                                     std::ofstream &file) {

  file << "run number: " << runNumber << m_endl;
  file << "bank: " << bank << m_endl;
}

void SaveDiffFittingAscii::writeHeader(std::vector<std::string> &columnHeadings,
                                       std::ofstream &file) {
  for (auto &heading : columnHeadings) {
    if (heading == "Chi") {
      writeVal(heading, file, true);
    } else {
      writeVal(heading, file, false);
    }
  }
}

void SaveDiffFittingAscii::writeData(API::ITableWorkspace_sptr workspace,
                                     std::ofstream &file, size_t columnSize) {

  for (size_t rowIndex = 0; rowIndex < workspace->rowCount(); ++rowIndex) {
    TableRow row = workspace->getRow(rowIndex);
    for (size_t columnIndex = 0; columnIndex < columnSize; columnIndex++) {

      auto row_str = boost::lexical_cast<std::string>(row.Double(columnIndex));
      g_log.debug() << row_str << std::endl;

      if (columnIndex == columnSize - 1)
        writeVal(row_str, file, true);
      else
        writeVal(row_str, file, false);
    }
  }
}

void SaveDiffFittingAscii::writeVal(std::string &val, std::ofstream &file,
                                    bool endline) {
  std::string valStr = boost::lexical_cast<std::string>(val);

  // checking if it needs to be surrounded in
  // quotes due to a comma being included
  size_t comPos = valStr.find(',');
  if (comPos != std::string::npos) {
    file << '"' << val << '"';
  } else {
    file << boost::lexical_cast<std::string>(val);
  }

  if (endline) {
    file << m_endl;
  } else {
    file << m_sep;
  }
}

} // namespace DataHandling
} // namespace Mantid
