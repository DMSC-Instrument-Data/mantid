#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/BoxControllerAccess.h"
#include "MantidMDAlgorithms/ConvertToDistributedMD.h"
#include "MantidMDAlgorithms/EventToMDEventConverter.h"
#include "MantidMDAlgorithms/RankResponsibility.h"
#include "MantidParallel/Communicator.h"
#include "MantidParallel/Collectives.h"
#include "MantidDataObjects/NullMDBox.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/ListValidator.h"

#include <boost/serialization/utility.hpp>
#include <fstream>

namespace {
#define MEASURE_ALL 1

class TimerParallel {
public:
  TimerParallel() {
    m_start_cpu_total = std::clock();
    m_start_wall_total = std::chrono::high_resolution_clock::now();
  }

  void start() {
    m_start_cpu = std::clock();
    m_start_wall = std::chrono::high_resolution_clock::now();
  }

  void stop() {
    auto stop_cpu = std::clock();
    auto stop_wall = std::chrono::high_resolution_clock::now();
    m_times_cpu.emplace_back((stop_cpu - m_start_cpu) / CLOCKS_PER_SEC);
    m_times_wall.emplace_back(
        std::chrono::duration<double>(stop_wall - m_start_wall).count());
  };

  void recordNumEvents(size_t numEvents) { m_numEvents = numEvents; }

  void dump(const Mantid::Parallel::Communicator &comm) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
      std::string base(cwd);
      // IMPORTANT: Change these file locations to your appropriate location
      if (base.find("scarf") != std::string::npos) {
        fileNameBase = "/home/isisg/scarf672/Mantid2/mpi_test/results/result_";
      } else {
        fileNameBase =
            "/home/anton/builds/Mantid_debug_clion/mpi_test/results/result_";
      }
    }

    // Measure the total time
    auto stop_cpu = std::clock();
    auto stop_wall = std::chrono::high_resolution_clock::now();
    m_times_cpu.emplace_back((stop_cpu - m_start_cpu_total) / CLOCKS_PER_SEC);
    m_times_wall.emplace_back(
        std::chrono::duration<double>(stop_wall - m_start_wall_total).count());

    // -----------------
    // Receive times
    std::vector<Mantid::Parallel::Request> requests;
    const auto numRanks = comm.size();
    const auto size = m_times_cpu.size();
    std::vector<double> times_cpu(numRanks * size);
    std::vector<double> times_wall(numRanks * size);

    if (comm.rank() == 0) {
      std::copy(m_times_cpu.begin(), m_times_cpu.end(), times_cpu.begin());
      std::copy(m_times_wall.begin(), m_times_wall.end(), times_wall.begin());

      for (int rank = 1; rank < numRanks; ++rank) {
        requests.emplace_back(comm.irecv(
            rank, 1, times_cpu.data() + size * rank, static_cast<int>(size)));
        requests.emplace_back(comm.irecv(
            rank, 2, times_wall.data() + size * rank, static_cast<int>(size)));
      }
    } else {
      requests.emplace_back(
          comm.isend(0, 1, m_times_cpu.data(), static_cast<int>(size)));
      requests.emplace_back(
          comm.isend(0, 2, m_times_wall.data(), static_cast<int>(size)));
    }
    wait_all(requests.begin(), requests.end());
    std::vector<Mantid::Parallel::Request>().swap(requests);

    // -----------------
    // Receive other
    std::vector<size_t> numEvents(static_cast<size_t>(numRanks));
    if (comm.rank() == 0) {
      Mantid::Parallel::gather(comm, m_numEvents, numEvents, 0);
    } else {
      Mantid::Parallel::gather(comm, m_numEvents, 0);
    }

    // -----------------
    // Save to file
    if (comm.rank() == 0) {
      // Save times
      const auto fileNumber = comm.size();
      std::string fileName =
          fileNameBase + std::to_string(fileNumber) + std::string(".txt");
      std::fstream stream;
      stream.open(fileName, std::ios::out | std::ios::app);
      for (auto index = 0ul; index < times_cpu.size(); ++index) {
        stream << times_cpu[index] << "," << times_wall[index] << "\n";
      }

      // Save other
      saveOther(stream, numEvents);
      stream.close();
    }
  }

private:
  void saveOther(std::fstream &stream, const std::vector<size_t> &other) {
    for (auto index = 0ul; index < other.size() - 1; ++index) {
      stream << other[index] << ",";
    }
    stream << other[other.size() - 1] << "\n";
  }

  std::clock_t m_start_cpu_total;
  std::chrono::system_clock::time_point m_start_wall_total;
  std::clock_t m_start_cpu;
  std::chrono::system_clock::time_point m_start_wall;
  std::vector<double> m_times_cpu;
  std::vector<double> m_times_wall;
  std::string fileNameBase;
  size_t m_numEvents;
};
}

namespace Mantid {
namespace MDAlgorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;
using DistributedCommon::DIM_DISTRIBUTED_TEST;
using DistributedCommon::BoxStructure;
using DistributedCommon::MDEventList;
using DistributedCommon::BoxStructureInformation;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

const std::string ConvertToDistributedMD::boxSplittingGroupName =
    "Box Splitting Settings";

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToDistributedMD)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ConvertToDistributedMD::name() const {
  return "ConvertToDistributedMD";
}

/// Algorithm's version for identification. @see Algorithm::version
int ConvertToDistributedMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertToDistributedMD::category() const {
  return "TODO: FILL IN A CATEGORY";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ConvertToDistributedMD::summary() const {
  return "TODO: FILL IN A SUMMARY";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertToDistributedMD::init() {
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<DataObjects::EventWorkspace>>(
          "InputWorkspace", "", Direction::Input),
      "An input workspace.");

  declareProperty(make_unique<Mantid::Kernel::PropertyWithValue<bool>>(
                      "LorentzCorrection", false, Direction::Input),
                  "Correct the weights of events by multiplying by the Lorentz "
                  "formula: sin(theta)^2 / lambda^4");

  declareProperty(
      make_unique<Mantid::Kernel::PropertyWithValue<double>>("Fraction", 0.01f),
      "Fraction of pulse time that should be used to build the "
      "initial data set.\n");

  std::vector<std::string> propOptions{"Q (lab frame)", "Q (sample frame)",
                                       "HKL"};
  declareProperty(
      "OutputDimensions", "Q (lab frame)",
      boost::make_shared<Mantid::Kernel::StringListValidator>(propOptions),
      "What will be the dimensions of the output workspace?\n"
      "  Q (lab frame): Wave-vector change of the lattice in the lab frame.\n"
      "  Q (sample frame): Wave-vector change of the lattice in the frame of "
      "the sample (taking out goniometer rotation).\n"
      "  HKL: Use the sample's UB matrix to convert to crystal's HKL indices.");

  // ---------------------------------
  // Box Controller settings
  // ---------------------------------
  this->initBoxControllerProps("2" /*SplitInto*/, 1500 /*SplitThreshold*/,
                               20 /*MaxRecursionDepth*/);

  declareProperty(
      make_unique<PropertyWithValue<int>>("MinRecursionDepth", 0),
      "Optional. If specified, then all the boxes will be split to this "
      "minimum recursion depth. 1 = one level of splitting, etc.\n"
      "Be careful using this since it can quickly create a huge number of "
      "boxes = (SplitInto ^ (MinRercursionDepth x NumDimensions)).\n"
      "But setting this property equal to MaxRecursionDepth property is "
      "necessary if one wants to generate multiple file based workspaces in "
      "order to merge them later\n");
  setPropertyGroup("MinRecursionDepth", boxSplittingGroupName);

  std::vector<double> extents{-50., 50., -50., 50., -50., 50.};
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("Extents", extents),
      "A comma separated list of min, max for each dimension,\n"
      "specifying the extents of each dimension. Optional, default "
      "+-50 in each dimension.");
  setPropertyGroup("Extents", boxSplittingGroupName);
}

/**
 * TODO: Find a good way to share with BoxSettingsAlgorithm
 */
void ConvertToDistributedMD::initBoxControllerProps(
    const std::string &SplitInto, int SplitThreshold, int MaxRecursionDepth) {
  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  auto mustBeMoreThen1 = boost::make_shared<BoundedValidator<int>>();
  mustBeMoreThen1->setLower(1);

  // Split up comma-separated properties
  typedef Mantid::Kernel::StringTokenizer tokenizer;
  tokenizer values(SplitInto, ",",
                   tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
  std::vector<int> valueVec;
  valueVec.reserve(values.count());
  for (const auto &value : values)
    valueVec.push_back(boost::lexical_cast<int>(value));

  declareProperty(
      Kernel::make_unique<ArrayProperty<int>>("SplitInto", valueVec),
      "A comma separated list of into how many sub-grid elements each "
      "dimension should split; "
      "or just one to split into the same number for all dimensions. Default " +
          SplitInto + ".");

  declareProperty(
      make_unique<PropertyWithValue<int>>("SplitThreshold", SplitThreshold,
                                          mustBePositive),
      "How many events in a box before it should be split. Default " +
          Kernel::Strings::toString(SplitThreshold) + ".");

  declareProperty(make_unique<PropertyWithValue<int>>(
                      "MaxRecursionDepth", MaxRecursionDepth, mustBeMoreThen1),
                  "How many levels of box splitting recursion are allowed. "
                  "The smallest box will have each side length :math:`l = "
                  "(extents) / (SplitInto^{MaxRecursionDepth}).` "
                  "Default " +
                      Kernel::Strings::toString(MaxRecursionDepth) + ".");
  setPropertyGroup("SplitInto", boxSplittingGroupName);
  setPropertyGroup("SplitThreshold", boxSplittingGroupName);
  setPropertyGroup("MaxRecursionDepth", boxSplittingGroupName);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConvertToDistributedMD::exec() {
// The generation of the distributed MD data requires the following steps:
// 1. Convert the data which corresponds to the first n percent of the total
// measurement
// 2. Build a preliminary box structure on the master rank
// 3. Determine how the data will be split based on the preliminary box
// structure and share this information with all
//    ranks
// 4. Share the preliminary box structure with all ranks
// 5. Convert all events
// 6. Add events to the local box structure
// 7. Send data from the each rank to the correct rank
// 8. Split the data
// 9. Ensure that the fileIDs and the box controller stats are correct.
// 10. Maybe save in this algorithm already
#if MEASURE_ALL
  TimerParallel timer;
#endif

  const auto localRank = this->communicator().rank();
  std::cout << "Starting rank " << localRank << "\n";
  // Get the users's inputs
  EventWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  {
    // ----------------------------------------------------------
    // 1. Get a n-percent fraction
    // ----------------------------------------------------------
    double fraction = getProperty("Fraction");
#if MEASURE_ALL
    timer.start();
#endif
    auto nPercentEvents = getFractionEvents(*inputWorkspace, fraction);
#if MEASURE_ALL
    timer.stop();
#endif

// -----------------------------------------------------------------
// 2. + 3. = 4.  Get the preliminary box structure and the partition
// behaviour
// -----------------------------------------------------------------
#if MEASURE_ALL
    timer.start();
#endif
    setupPreliminaryBoxStructure(nPercentEvents);
#if MEASURE_ALL
    timer.stop();
#endif
  }

  // ----------------------------------------------------------
  // 5. Convert all events
  // ----------------------------------------------------------
  {
#if MEASURE_ALL
    timer.start();
#endif
    auto allEvents = getFractionEvents(*inputWorkspace, 1.);
#if MEASURE_ALL
    timer.stop();
#endif

// ----------------------------------------------------------
// 6. Add the local data to the preliminary box structure
// ----------------------------------------------------------
#if MEASURE_ALL
    timer.start();
#endif
    addEventsToPreliminaryBoxStructure(allEvents);
#if MEASURE_ALL
    timer.stop();
#endif
  }

// ------------------------------------------------------
// 7. Redistribute data
// ----------------------------------------------------------
#if MEASURE_ALL
  timer.start();
#endif
  redistributeData();
#if MEASURE_ALL
  timer.stop();
#endif

// ----------------------------------------------------------
// 8. Continue to split locally
// ----------------------------------------------------------
#if MEASURE_ALL
  timer.start();
#endif
  continueSplitting();
#if MEASURE_ALL
  timer.stop();
#endif

// --------------------------------------- -------------------
// 9. Ensure that box controller and fileIDs are correct
// ----------------------------------------------------------
#if MEASURE_ALL
  timer.start();
#endif
  updateMetaData();
#if MEASURE_ALL
  timer.stop();
#endif

#if MEASURE_ALL
  const auto numEvents = m_boxStructureInformation.boxStructure->getNPoints();
  timer.recordNumEvents(numEvents);
  const auto &comm = this->communicator();
  timer.dump(comm);
#endif

  // ----------------------------------------------------------
  // 9. Save: THIS NEEDS TO BE STILL IMPLEMENTED
  // ----------------------------------------------------------
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// Definitions for: 1. Convert first n percent of the data
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
/**
 * Generates an MDEvent list from the events of an EventWorkspace. When a
 * fraction is specified, only events from
 * the start of the measurement until the fraction of the end of the measurement
 * are considered.
 * @param workspace : An EventWorkspace
 * @param fraction : The fraction of the total measurement time which should be
 * considered.
 * @return A collection of MDEvents
 */
MDEventList
ConvertToDistributedMD::getFractionEvents(const EventWorkspace &workspace,
                                          double fraction) const {
  // Get user inputs
  auto extents = getWorkspaceExtents();

  EventToMDEventConverter converter;
  return converter.getEvents(workspace, extents, fraction, QFrame::QLab);
}

/**
 * Fetches the user-specified workspace extents
 * @return a vector with extents (6 values)
 */
std::vector<Mantid::coord_t>
ConvertToDistributedMD::getWorkspaceExtents() const {
  std::vector<double> extentsInput = getProperty("Extents");
  // Replicate a single min,max into several
  if (extentsInput.size() != DIM_DISTRIBUTED_TEST * 2)
    throw std::invalid_argument(
        "You must specify multiple of 2 extents, ie two for each dimension.");

  // Convert input to coordinate type
  std::vector<coord_t> extents;
  std::transform(extentsInput.begin(), extentsInput.end(),
                 std::back_inserter(extents),
                 [](double value) { return static_cast<coord_t>(value); });

  return extents;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// Definitions for: 2 + 3 + 4. Get the prelim. box structure
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
/**
 * Sets up a preliminary box structure and establishes which ranks are
 * associated with which boxes.
 * @param mdEvents : A collection of MDEvents
 */
void ConvertToDistributedMD::setupPreliminaryBoxStructure(
    MDEventList &mdEvents) {
  // The following steps are performed
  // 1. The first data is sent from all the ranks to the master rank
  // 2. The box structure is built on the master rank
  // 3. The responsibility of the ranks is determined, it is decided which ranks
  // will take care of which boxes
  // 4. This responsibility is shared with all ranks.
  // 5. The master rank serializes the box structure
  // 6. The serialized box structure is broadcast from the master rank to all
  // other ranks
  // 7. The other ranks deserialize the box structure (but without the event
  // information)
  const auto &communicator = this->communicator();
  const boost::mpi::communicator &boostComm = communicator;
  MPI_Comm comm = boostComm;
  MPI_Barrier(comm);

  SerialBoxStructureInformation serialBoxStructureInformation;
  if (communicator.rank() == 0) {
    // 1.b Receive the data from all the other ranks

    auto allEvents = receiveMDEventsOnMaster(communicator, mdEvents);

    // 2. Build the box structure on the master rank
    auto boxStructureInformation = generatePreliminaryBoxStructure(allEvents);

    // 3. Determine splitting
    RankResponsibility rankResponsibility;
    m_responsibility = rankResponsibility.getResponsibilites(
        communicator.size(), boxStructureInformation.boxStructure.get());

    // 4.a Broadcast splitting behaviour
    sendRankResponsibility(communicator, m_responsibility);

    // 5. Serialize the box structure
    serialBoxStructureInformation =
        serializeBoxStructure(boxStructureInformation);

    // 6.a Broadcast serialized box structure to all other ranks
    sendSerializedBoxStructureInformation(communicator,
                                          serialBoxStructureInformation);
  } else {
    // 1.a Send the event data to the master rank
    sendMDEventsToMaster(communicator, mdEvents);

    // 4.b Receive the splitting behaviour from the broadcast
    m_responsibility = receiveRankResponsibility(communicator);

    // 6.b Receive the box structure from master
    serialBoxStructureInformation =
        receiveSerializedBoxStructureInformation(communicator);
  }

  // 7. Deserialize on all ranks
  auto hollowBoxStructureInformation =
      deserializeBoxStructure(serialBoxStructureInformation);

  // Create a rank responsibility map
  setRankResponsibilityMap();

  // Set results
  m_boxStructureInformation = std::move(hollowBoxStructureInformation);
}

void ConvertToDistributedMD::setRankResponsibilityMap() {
  for (auto rank = 0ul; rank < m_responsibility.size(); ++rank) {
    m_endBoxIndexRangeVsRank.emplace(m_responsibility[rank].second, rank);
    m_endBoxIndexRange.push_back(m_responsibility[rank].second);
  }
}

/**
 * Sends a collection of MDEvents to the master rank. This method is paired with
 * `receiveMDEventsOnMaster`
 * @param communicator : The MPI communicator
 * @param mdEvents : A collection of MDEvents
 */
void ConvertToDistributedMD::sendMDEventsToMaster(
    const Mantid::Parallel::Communicator &communicator,
    MDEventList &mdEvents) const {
  // Send the totalNumberOfEvents
  auto totalNumberEvents = mdEvents.size();
  gather(communicator, totalNumberEvents, 0);

  // Send the vector of md events
  const boost::mpi::communicator &boostComm = communicator;
  MPI_Comm comm = boostComm;
  auto sizeMdEvents =
      sizeof(std::remove_reference<decltype(mdEvents)>::type::value_type);
  MPI_Send(reinterpret_cast<char *>(mdEvents.data()),
           static_cast<int>(mdEvents.size() * sizeMdEvents), MPI_CHAR, 0, 2,
           comm);
}

/**
 * Sends a collection of MDEvents to the master rank. This method is paired with
 * `sendMDEventsToMaster`
 * @param communicator : The MPI communicator
 * @param mdEvents : A collection of MDEvents
 * @return a collection of MDEvents
 */
MDEventList ConvertToDistributedMD::receiveMDEventsOnMaster(
    const Mantid::Parallel::Communicator &communicator,
    const MDEventList &mdEvents) const {
  // In order to get all relevant MDEvents onto the master rank we need to:
  // 1. Inform the master rank about the number of events on all ranks (gather)
  // 2. Create a sufficiently large buffer on master
  // 3. Determine the stride that each rank requires, ie which offset each rank
  // requires
  // 4. Send the data to master. This would be a natural operation for gatherv,
  // however this is not available in
  //    boost 1.58 (in 1.59+ it is).

  // 1. Get the totalNumberOfEvents for each rank
  std::vector<size_t> numberOfEventsPerRank;
  auto masterNumberOfEvents = mdEvents.size();
  gather(communicator, masterNumberOfEvents, numberOfEventsPerRank, 0);

  // 2. Create a buffer
  auto totalNumberOfEvents = std::accumulate(numberOfEventsPerRank.begin(),
                                             numberOfEventsPerRank.end(), 0ul);
  MDEventList totalEvents(totalNumberOfEvents);
  std::copy(mdEvents.begin(), mdEvents.end(), totalEvents.begin());

  // 3. Determine the strides
  std::vector<int> strides;
  strides.reserve(numberOfEventsPerRank.size());
  strides.push_back(0);
  {
    std::vector<int> tempStrides(numberOfEventsPerRank.size() - 1);
    std::partial_sum(numberOfEventsPerRank.begin(),
                     numberOfEventsPerRank.end() - 1, tempStrides.begin());
    std::move(tempStrides.begin(), tempStrides.end(),
              std::back_inserter(strides));
  }

  // 4. Send the data from all ranks to the master rank
  const auto numberOfRanks = communicator.size();
  const boost::mpi::communicator &boostComm = communicator;
  MPI_Comm comm = boostComm;
  std::vector<MPI_Request> mpi_requests;
  mpi_requests.reserve(static_cast<size_t>(numberOfRanks) - 1);
  std::vector<MPI_Status> mpi_status;
  mpi_requests.reserve(static_cast<size_t>(numberOfRanks) - 1);
  auto sizeMdEvents =
      sizeof(std::remove_reference<decltype(mdEvents)>::type::value_type);
  for (int rank = 1; rank < numberOfRanks; ++rank) {
    // Determine where to insert the array
    auto start = totalEvents.data() + strides[rank];
    auto length = numberOfEventsPerRank[rank];
    mpi_requests.emplace_back();
    mpi_status.emplace_back();
    MPI_Irecv(reinterpret_cast<char *>(start),
              static_cast<int>(length * sizeMdEvents), MPI_CHAR, rank, 2, comm,
              &mpi_requests.back());
  }

  MPI_Waitall(static_cast<int>(mpi_requests.size()), mpi_requests.data(),
              mpi_status.data());
  return totalEvents;
}

/**
 * Generates the preliminiary box structure.
 * @param allEvents : A collection of all contributing events
 * @return information regarding the box structure and the box controller
 */
BoxStructureInformation ConvertToDistributedMD::generatePreliminaryBoxStructure(
    const MDEventList &allEvents) const {
  // To build the box structure we need to:
  // 1. Generate a temporary MDEventWorkspace locally on the master rank
  // 2. Populate the workspace with the MDEvents
  // 3. Extract the box structure and the box controller from the workspace, ie
  //    remove ownership from the underlying data structures

  // 1. Create temporary MDEventWorkspace
  auto tempWorkspace = createTemporaryWorkspace();

  // 2. Populate workspace with MDEvents
  addMDEventsToMDEventWorkspace(*tempWorkspace, allEvents);

  // 3. Extract box structure and box controller
  return extractBoxStructure(*tempWorkspace);
}

void ConvertToDistributedMD::sendSerializedBoxStructureInformation(
    const Mantid::Parallel::Communicator &communicator,
    SerialBoxStructureInformation &serializedBoxStructureInformation) const {
  boost::mpi::broadcast(communicator, serializedBoxStructureInformation, 0);
}

SerialBoxStructureInformation
ConvertToDistributedMD::receiveSerializedBoxStructureInformation(
    const Mantid::Parallel::Communicator &communicator) const {
  SerialBoxStructureInformation serializedBoxStructureInformation;
  boost::mpi::broadcast(communicator, serializedBoxStructureInformation, 0);
  return serializedBoxStructureInformation;
}

void ConvertToDistributedMD::sendRankResponsibility(
    const Mantid::Parallel::Communicator &communicator,
    std::vector<std::pair<size_t, size_t>> &responsibility) const {
  boost::mpi::broadcast(communicator, responsibility.data(),
                        communicator.size(), 0);
}

std::vector<std::pair<size_t, size_t>>
ConvertToDistributedMD::receiveRankResponsibility(
    const Mantid::Parallel::Communicator &communicator) const {
  std::vector<std::pair<size_t, size_t>> responsibility;
  responsibility.resize(static_cast<size_t>(communicator.size()));
  boost::mpi::broadcast(communicator, responsibility.data(),
                        communicator.size(), 0);
  return responsibility;
}

SerialBoxStructureInformation ConvertToDistributedMD::serializeBoxStructure(
    const BoxStructureInformation &boxStructureInformation) const {
  BoxStructureSerializer serializer;
  return serializer.serializeBoxStructure(boxStructureInformation);
}

BoxStructureInformation ConvertToDistributedMD::deserializeBoxStructure(
    SerialBoxStructureInformation &serializedBoxStructureInformation) const {
  BoxStructureSerializer serializer;
  return serializer.deserializeBoxStructure(serializedBoxStructureInformation);
}

void ConvertToDistributedMD::addMDEventsToMDEventWorkspace(
    MDEventWorkspace3Lean &workspace, const MDEventList &allEvents) const {
  // We could call the addEvents method, but it seems to behave slightly
  // differently
  // to addEvent. TODO: Investigate if addEvents could be used.
  size_t eventsAddedSinceLastSplit = 0;
  size_t eventsAddedTotal = 0;
  auto boxController = workspace.getBoxController();
  size_t lastNumBoxes = boxController->getTotalNumMDBoxes();

  // TODO: This part of the code is a bottle neck in the code. There are two
  // ways to improve the situation:
  //       1. We can adapt the n% threshold and which we should do, depending on
  //       the data size
  //       2. We could try and see if we can enable multi-threading at this
  //       point.
  for (const auto &event : allEvents) {
    // We have a different situation here compared to ConvertToDiffractionMD,
    // since we already have all events converted
    if (boxController->shouldSplitBoxes(
            eventsAddedTotal, eventsAddedSinceLastSplit, lastNumBoxes)) {
      workspace.splitAllIfNeeded(nullptr);
      eventsAddedSinceLastSplit = 0;
      lastNumBoxes = boxController->getTotalNumMDBoxes();
    }
    ++eventsAddedSinceLastSplit;
    ++eventsAddedTotal;
    workspace.addEvent(event);
  }
  workspace.splitAllIfNeeded(nullptr);
  workspace.refreshCache();
}

BoxStructureInformation ConvertToDistributedMD::extractBoxStructure(
    MDEventWorkspace3Lean &workspace) const {
  // Extract the box controller
  BoxStructureInformation boxStructureInformation;
  workspace.transferInternals(boxStructureInformation.boxController,
                              boxStructureInformation.boxStructure);
  return boxStructureInformation;
}

/**
 * Generate a temporary MDEventWorkspace which is used in order to build up the
 * preliminary box structure.
 * @return an MDEventWorkspace
 */
MDEventWorkspace3Lean::sptr
ConvertToDistributedMD::createTemporaryWorkspace() const {
  auto imdWorkspace = DataObjects::MDEventFactory::CreateMDWorkspace(
      DIM_DISTRIBUTED_TEST, "MDLeanEvent");
  auto workspace =
      boost::dynamic_pointer_cast<DataObjects::MDEventWorkspace3Lean>(
          imdWorkspace);
  auto frameInformation = createFrame();
  auto extents = getWorkspaceExtents();
  for (size_t d = 0; d < DIM_DISTRIBUTED_TEST; d++) {
    MDHistoDimension *dim = new MDHistoDimension(
        frameInformation.dimensionNames[d], frameInformation.dimensionNames[d],
        *frameInformation.frame, static_cast<coord_t>(extents[d * 2]),
        static_cast<coord_t>(extents[d * 2 + 1]), 10);
    workspace->addDimension(MDHistoDimension_sptr(dim));
  }
  workspace->initialize();

  // Build up the box controller
  auto bc = workspace->getBoxController();
  this->setBoxController(bc);
  workspace->splitBox();

  // Perform minimum recursion depth splitting
  int minDepth = this->getProperty("MinRecursionDepth");
  int maxDepth = this->getProperty("MaxRecursionDepth");
  if (minDepth > maxDepth)
    throw std::invalid_argument(
        "MinRecursionDepth must be <= MaxRecursionDepth ");
  workspace->setMinRecursionDepth(size_t(minDepth));

  workspace->setCoordinateSystem(frameInformation.specialCoordinateSystem);
  return workspace;
}

FrameInformation ConvertToDistributedMD::createFrame() const {
  using Mantid::Kernel::SpecialCoordinateSystem;
  FrameInformation information;
  std::string outputDimensions = getProperty("OutputDimensions");
  auto frameFactory = makeMDFrameFactoryChain();

  if (outputDimensions == "Q (sample frame)") {
    // Names
    information.dimensionNames[0] = "Q_sample_x";
    information.dimensionNames[1] = "Q_sample_y";
    information.dimensionNames[2] = "Q_sample_z";
    information.specialCoordinateSystem = Mantid::Kernel::QSample;
    // Frame
    MDFrameArgument frameArgQSample(QSample::QSampleName, "");
    information.frame = frameFactory->create(frameArgQSample);
  } else if (outputDimensions == "HKL") {
    information.dimensionNames[0] = "H";
    information.dimensionNames[1] = "K";
    information.dimensionNames[2] = "L";
    information.specialCoordinateSystem = Mantid::Kernel::HKL;
    MDFrameArgument frameArgQLab(HKL::HKLName,
                                 Mantid::Kernel::Units::Symbol::RLU.ascii());
    information.frame = frameFactory->create(frameArgQLab);
  } else {
    information.dimensionNames[0] = "Q_lab_x";
    information.dimensionNames[1] = "Q_lab_y";
    information.dimensionNames[2] = "Q_lab_z";
    information.specialCoordinateSystem = Mantid::Kernel::QLab;
    MDFrameArgument frameArgQLab(QLab::QLabName, "");
    information.frame = frameFactory->create(frameArgQLab);
  }

  return information;
}

void ConvertToDistributedMD::setBoxController(
    Mantid::API::BoxController_sptr bc) const {
  size_t numberOfDimensions = bc->getNDims();

  int splitThreshold = this->getProperty("SplitThreshold");
  bc->setSplitThreshold(static_cast<size_t>(splitThreshold));
  int maxRecursionDepth = this->getProperty("MaxRecursionDepth");
  bc->setMaxDepth(static_cast<size_t>(maxRecursionDepth));

  std::vector<int> splits = getProperty("SplitInto");
  if (splits.size() == 1) {
    bc->setSplitInto(static_cast<size_t>(splits[0]));
  } else if (splits.size() == numberOfDimensions) {
    for (size_t d = 0; d < numberOfDimensions; ++d)
      bc->setSplitInto(d, static_cast<size_t>(splits[d]));
  } else
    throw std::invalid_argument("SplitInto parameter has " +
                                Strings::toString(splits.size()) +
                                " arguments. It should have either 1, or the "
                                "same as the number of dimensions.");
  bc->resetNumBoxes();
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// Definitions for: 5 + 6 Add all events to the box structure
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
/**
 * This adds all MDEvents on each rank to their preliminary box structure.
 * @param allEvents : A collection of MDEvents
 */
void ConvertToDistributedMD::addEventsToPreliminaryBoxStructure(
    DistributedCommon::MDEventList &allEvents) {
  // Add all events to the hollow box structure
  for (auto &event : allEvents) {
    // TODO: create a move-enabled addEvent.
    m_boxStructureInformation.boxStructure->addEvent(std::move(event));
  }
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// Definitions for: 7. Redistribute data
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
/**
 * Redistributes the data which is stored in the MDBoxes of the preliminary box
 * structure.
 */
void ConvertToDistributedMD::redistributeData() {
  // This step has the most complex communication pattern since all ranks need
  // to communicate with each other.
  // Note that we found that coalescing as many messages as possible improves
  // performance considerably.
  // The steps are:
  // 1. Exchange information between ranks about how many events will be
  // transmitted
  // 2. Exchange the events beween ranks
  // 3. Add the sent data and remove data in boxes for which the local rank is
  // not responsible for

  const auto &communicator = this->communicator();
  // Generate the MDBoxes
  std::vector<Mantid::API::IMDNode *> boxes;
  m_boxStructureInformation.boxStructure->getBoxes(boxes, 1000, true);
  std::vector<MDBox<MDLeanEvent<DIM_DISTRIBUTED_TEST>, DIM_DISTRIBUTED_TEST> *>
      mdBoxes;
  for (auto &box : boxes) {
    mdBoxes.emplace_back(dynamic_cast<
        MDBox<MDLeanEvent<DIM_DISTRIBUTED_TEST>, DIM_DISTRIBUTED_TEST> *>(box));
  }

  // 1. Determine the number of events per rank per box
  auto relevantEventsPerRankPerBox =
      getRelevantEventsPerRankPerBox(communicator, boxes);

  // 2. Send the actual data
  auto boxVsMDEvents =
      sendDataToCorrectRank(communicator, relevantEventsPerRankPerBox, mdBoxes);

  // 3. Place the data into the correct boxes
  auto localRank = communicator.rank();
  auto startIndex = m_responsibility[localRank].first;
  auto stopIndex = m_responsibility[localRank].second;

  for (auto index = startIndex; index <= stopIndex; ++index) {
    auto mdBox = mdBoxes[index];
    auto &events = boxVsMDEvents.at(index);
    auto &oldEvents = mdBox->getEvents();
    oldEvents.swap(events);
    MDEventList().swap(events);
  }

  // Remove the data which is not relevant for the local rank which is before
  // the startIndex
  auto const numberOfDimensions = mdBoxes[0]->getNumDims();
  setNullMDBox(mdBoxes, 0ul, startIndex, numberOfDimensions);

  // Remove the data which is not relevant for the local rank which is after the
  // stopIndex
  setNullMDBox(mdBoxes, stopIndex + 1, boxes.size(), numberOfDimensions);

  // We need to refresh the cache
  m_boxStructureInformation.boxStructure->refreshCache(nullptr);

  // Cache the max ID, we need it later when updating the fileIDs on all the
  // ranks. Note that getMaxId will return the next available id, ie it is
  // not really the max id.
  m_maxIDBeforeSplit = m_boxStructureInformation.boxController->getMaxId() - 1;
}

void ConvertToDistributedMD::setNullMDBox(
    std::vector<MDBox<MDLeanEvent<DIM_DISTRIBUTED_TEST>,
                      DIM_DISTRIBUTED_TEST> *> &mdBoxes,
    size_t startIndex, size_t stopIndex, const size_t numberOfDimensions) {
  for (auto index = startIndex; index < stopIndex; ++index) {
    auto mdBox = mdBoxes[index];

    // On this rank the box has to be an MDEventBox
    if (!mdBox->isBox()) {
      throw std::runtime_error("Expected an MDBox, but got an MDGridBox");
    }

    const auto &boxController = mdBox->getBoxController();
    const auto depth = mdBox->getDepth();

    std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> extents;
    for (auto dimension = 0ul; dimension < numberOfDimensions; ++dimension) {
      extents.emplace_back(mdBox->getExtents(dimension));
    }

    const auto boxID = mdBox->getID();
    auto responsibleRank = getResponsibleRank(index);

    // Create a new NullMDBox
    auto parent = mdBox->getParent();
    auto newNullMDBox = Mantid::Kernel::make_unique<
        NullMDBox<MDLeanEvent<DIM_DISTRIBUTED_TEST>, DIM_DISTRIBUTED_TEST>>(
        boxController, depth, extents, boxID, responsibleRank);
    newNullMDBox->setShowGlobalValues(false);
    newNullMDBox->setParent(parent);

    // Find the child index and replace it in the parent
    auto mdGridBoxParent = dynamic_cast<
        MDGridBox<MDLeanEvent<DIM_DISTRIBUTED_TEST>, DIM_DISTRIBUTED_TEST> *>(
        parent);
    const auto childIndex = mdGridBoxParent->getChildIndexFromID(boxID);
    mdGridBoxParent->setChild(childIndex, newNullMDBox.release());
  }
}

int ConvertToDistributedMD::getResponsibleRank(size_t leafNodeIndex) {
  auto result = std::lower_bound(m_endBoxIndexRange.begin(),
                                 m_endBoxIndexRange.end(), leafNodeIndex);
  return m_endBoxIndexRangeVsRank[*result];
}

std::unordered_map<size_t, std::vector<uint64_t>>
ConvertToDistributedMD::getRelevantEventsPerRankPerBox(
    const Mantid::Parallel::Communicator &communicator,
    const std::vector<Mantid::API::IMDNode *> &boxes) {
  // We want to get the number of events per rank per box for the boxes which
  // are relevant for the local rank, ie the boxes for which the local rank is
  // responsible.

  const auto localRank = communicator.rank();
  const auto numberOfRanks = static_cast<size_t>(communicator.size());

  // -------------------------------------------------------------------------------------------------------------------
  // Now we share the number of events per box with the relevant boxes. Since a
  // rank is responsible for a set of
  // boxes, we can group the information together and have a single
  // communication between a pair of ranks. On the
  // the receiving end, we need to unravel this information.
  // Sending the information one by one has shown to be a performance issue.
  // In order to achieve the exchange we:
  // Setup receive buffer
  std::vector<std::vector<uint64_t>> receiveBuffer(numberOfRanks);
  const auto numberOfBoxesThatThisRankIsResponsibleFor =
      m_responsibility[localRank].second - m_responsibility[localRank].first +
      1;
  for (auto &element : receiveBuffer) {
    element.resize(numberOfBoxesThatThisRankIsResponsibleFor);
  }

  // Set up send buffer
  std::vector<std::vector<uint64_t>> sendBuffer(numberOfRanks);

  for (auto rank = 0ul; rank < numberOfRanks; ++rank) {
    auto startIndex = m_responsibility[rank].first;
    auto stopIndex = m_responsibility[rank].second;

    auto &sendBufferForRank = sendBuffer[rank];
    std::transform(boxes.begin() + startIndex, boxes.begin() + stopIndex + 1,
                   std::back_inserter(sendBufferForRank),
                   [](API::IMDNode *box) { return box->getNPoints(); });
  }

  // Send the data
  const boost::mpi::communicator &boostComm = communicator;
  MPI_Comm comm = boostComm;
  std::vector<MPI_Request> mpiRequests;
  std::vector<MPI_Status> mpiStatus;
  mpiRequests.reserve(numberOfRanks);
  mpiStatus.reserve(numberOfRanks);

  for (auto rank = 0ul; rank < numberOfRanks; ++rank) {
    if (rank == static_cast<size_t>(localRank)) {
      // We need to setup a receive for all the other ranks
      for (auto receiveFromRank = 0ul; receiveFromRank < numberOfRanks;
           ++receiveFromRank) {
        // We don't want to receive or send anything from ourselves, so just
        // copy
        if (receiveFromRank == static_cast<size_t>(localRank)) {
          std::copy(sendBuffer[rank].begin(), sendBuffer[rank].end(),
                    receiveBuffer[rank].begin());
        } else {
          mpiRequests.emplace_back();
          mpiStatus.emplace_back();
          MPI_Irecv(receiveBuffer[receiveFromRank].data(),
                    static_cast<int>(receiveBuffer[receiveFromRank].size()),
                    MPI_UINT64_T, static_cast<int>(receiveFromRank),
                    static_cast<int>(receiveFromRank), comm,
                    &mpiRequests.back());
        }
      }
    } else {
      // We need to send to this rank
      mpiRequests.emplace_back();
      mpiStatus.emplace_back();
      MPI_Isend(sendBuffer[rank].data(),
                static_cast<int>(sendBuffer[rank].size()), MPI_UINT64_T,
                static_cast<int>(rank), localRank, comm, &mpiRequests.back());
    }
  }
  auto resultSend = MPI_Waitall(static_cast<int>(mpiRequests.size()),
                                mpiRequests.data(), mpiStatus.data());
  if (resultSend != MPI_SUCCESS) {
    std::string message =
        "There has been an issue sharing the event numbers on rank " +
        std::to_string(localRank);
    throw std::runtime_error(message);
  }

  // Unravel the data such that it can be consumed later on
  std::unordered_map<size_t, std::vector<uint64_t>> relevantEventsPerRankPerBox;
  auto startIndex = m_responsibility[localRank].first;
  auto stopIndex = m_responsibility[localRank].second;
  for (auto index = startIndex; index <= stopIndex; ++index) {
    // Add for each box the relevant number of events
    auto &elements = relevantEventsPerRankPerBox[index];
    for (auto rank = 0ul; rank < numberOfRanks; ++rank) {
      elements.push_back(receiveBuffer[rank][index - startIndex]);
    }
  }

  auto sync = MPI_Barrier(comm);
  if (sync != MPI_SUCCESS) {
    throw std::runtime_error("Sync failed");
  }

  return relevantEventsPerRankPerBox;
}

/**
 * Exchanges data between ranks. Each rank knows which rank is responsible for a
 * particular box and sends the data
 * to that box. If the local rank is responsible for a particular box, then it
 * will receive data from all other ranks
 * regarding the box.
 * @param communicator : An MPI communicator
 * @param relevantEventsPerRankPerBox : Maps boxId to the number of events per
 * rank. Note that the index into the vector is the rank.
 * @param mdBoxes : A collection of boxes
 * @return a map with a boxId as the key and a vector MDEvents as the value
 */
std::unordered_map<size_t, DistributedCommon::MDEventList>
ConvertToDistributedMD::sendDataToCorrectRank(
    const Mantid::Parallel::Communicator &communicator,
    const std::unordered_map<size_t, std::vector<uint64_t>> &
        relevantEventsPerRankPerBox,
    const std::vector<MDBox<MDLeanEvent<DIM_DISTRIBUTED_TEST>,
                            DIM_DISTRIBUTED_TEST> *> &mdBoxes) {
  // We send the data between all nodes. For this we:
  // 1. Set up a buffer which will accumulate the data. The buffer is a map from
  // boxIndex to an event vector
  // 2. Determine the strides. Since we accumulate data for one box from several
  // ranks into a single vector
  //    we need to determine at which starting position of the array the data of
  //    a particular rank should be added.
  // 3. Iterate over all boxes and determine if a box is associated with the
  // local rank, in which case we want to
  //    receive data from all other ranks, else we need to send it

  // --------------------
  // 1. Set up the buffer
  // --------------------
  // We want to group the data that is sent between the ranks in a similar way
  // that we did when sending the length
  // number of events information in the previous step.
  const auto localRank = communicator.rank();
  const auto numberOfRanks = static_cast<size_t>(communicator.size());

  // Setup a receive buffer
  std::vector<MDEventList> receiveBuffer(numberOfRanks);
  auto startIndex = m_responsibility[localRank].first;
  auto stopIndex = m_responsibility[localRank].second;
  for (auto rank = 0ul; rank < numberOfRanks; ++rank) {
    auto totalNumberEvents = 0ul;
    for (auto index = startIndex; index <= stopIndex; ++index) {
      totalNumberEvents += relevantEventsPerRankPerBox.at(index)[rank];
    }
    receiveBuffer[rank].resize(totalNumberEvents);
  }

  // Setup a send buffer
  std::vector<MDEventList> sendBuffer(numberOfRanks);
  for (auto rank = 0ul; rank < numberOfRanks; ++rank) {
    auto start = m_responsibility[rank].first;
    auto stop = m_responsibility[rank].second;
    auto &eventsList = sendBuffer[rank];
    auto totalNumberOfEventsForRank = 0ul;
    for (auto index = start; index <= stop; ++index) {
      totalNumberOfEventsForRank += mdBoxes[index]->getEvents().size();
    }
    eventsList.reserve(totalNumberOfEventsForRank);

    for (auto index = start; index <= stop; ++index) {
      auto &events = mdBoxes[index]->getEvents();
      std::move(events.begin(), events.end(), std::back_inserter(eventsList));
    }
  }

  // ---------------------------------------
  // 2. Exchange the data between the ranks
  // ---------------------------------------
  const boost::mpi::communicator &boostComm = communicator;
  MPI_Comm comm = boostComm;
  std::vector<MPI_Request> mpiRequests;
  std::vector<MPI_Status> mpiStatus;
  const auto sizeOfMDLeanEvent =
      sizeof(Mantid::DataObjects::MDLeanEvent<DIM_DISTRIBUTED_TEST>);

  auto sync = MPI_Barrier(comm);
  if (sync != MPI_SUCCESS) {
    throw std::runtime_error("Sync failed");
  }

  for (auto rank = 0ul; rank < numberOfRanks; ++rank) {
    if (rank == static_cast<size_t>(localRank)) {
      for (auto receiveFromRank = 0ul; receiveFromRank < numberOfRanks;
           ++receiveFromRank) {
        // We don't want to receive or send anything from ourselves, so just
        // copy
        if (receiveFromRank == static_cast<size_t>(localRank)) {
          std::copy(sendBuffer[rank].begin(), sendBuffer[rank].end(),
                    receiveBuffer[rank].begin());
        } else {
          mpiRequests.emplace_back();
          mpiStatus.emplace_back();
          MPI_Irecv(
              reinterpret_cast<char *>(receiveBuffer[receiveFromRank].data()),
              static_cast<int>(receiveBuffer[receiveFromRank].size() *
                               sizeOfMDLeanEvent),
              MPI_CHAR, static_cast<int>(receiveFromRank),
              static_cast<int>(receiveFromRank), comm, &mpiRequests.back());
        }
      }
    } else {
      // We send the data to the receiving rank
      mpiRequests.emplace_back();
      mpiStatus.emplace_back();
      MPI_Isend(reinterpret_cast<char *>(sendBuffer[rank].data()),
                static_cast<int>(sendBuffer[rank].size() * sizeOfMDLeanEvent),
                MPI_CHAR, static_cast<int>(rank), localRank, comm,
                &mpiRequests.back());
    }
  }

  auto resultSend = MPI_Waitall(static_cast<int>(mpiRequests.size()),
                                mpiRequests.data(), mpiStatus.data());
  if (resultSend != MPI_SUCCESS) {
    std::string message =
        "There has been an issue sharing the event numbers on rank " +
        std::to_string(localRank);
    throw std::runtime_error(message);
  }

  sync = MPI_Barrier(comm); // Should not really be  required, can be removed
                            // when there is time to measure that there is no
                            // difference.
  if (sync != MPI_SUCCESS) {
    throw std::runtime_error("Sync failed");
  }

  // Free up data that is not required any longer. Note that all the data we need is in the receive buffer, even the
  // local data
  for (auto index = startIndex; index <= stopIndex; ++index) {
    auto mdBox = mdBoxes[index];
    auto& oldEvents = mdBox->getEvents();
    MDEventList().swap(oldEvents);
  }

  // -------------------------
  // 2. Break up the data
  // -------------------------
  // First rearrange the data. The strides are a vector of vectors where the
  // first index is the rank and the second
  // index is the box index but with a startIndex offset !!
  std::vector<std::vector<uint64_t>> rearrangedNumberOfEvents(numberOfRanks);
  for (auto rank = 0ul; rank < numberOfRanks; ++rank) {
    rearrangedNumberOfEvents[rank].reserve(stopIndex - startIndex + 1);
  }

  for (auto index = startIndex; index <= stopIndex; ++index) {
    auto &numberOfEventsPerRank = relevantEventsPerRankPerBox.at(index);
    for (auto rank = 0ul; rank < numberOfRanks; ++rank) {
      // Recall that the box index is offset by startIndex
      rearrangedNumberOfEvents[rank].push_back(numberOfEventsPerRank[rank]);
    }
  }

  // Now calculate the actual stride
  std::vector<std::vector<uint64_t>> offsets(numberOfRanks);
  for (auto rank = 0ul; rank < numberOfRanks; ++rank) {
    auto &offset = offsets[rank];
    offset.push_back(0);
    std::vector<uint64_t> tempOffsets(rearrangedNumberOfEvents[rank].size() -
                                      1);
    std::partial_sum(rearrangedNumberOfEvents[rank].begin(),
                     rearrangedNumberOfEvents[rank].end() - 1,
                     tempOffsets.begin());
    std::move(tempOffsets.begin(), tempOffsets.end(),
              std::back_inserter(offset));
  }

  std::unordered_map<size_t, MDEventList> boxVsMDEvents;
  for (auto index = startIndex; index <= stopIndex; ++index) {
    auto numberOfEventsInBox = 0ul;
    auto &numberOfEventsPerRank = relevantEventsPerRankPerBox.at(index);
    for (auto rank = 0ul; rank < numberOfRanks; ++rank) {
      numberOfEventsInBox += numberOfEventsPerRank[rank];
    }
    auto &eventList = boxVsMDEvents[index];
    eventList.reserve(numberOfEventsInBox);
  }

  auto tot = 0ul;
  for (auto index = startIndex; index <= stopIndex; ++index) {
    auto &eventList = boxVsMDEvents[index];
    auto &numberOfEventsPerRank = relevantEventsPerRankPerBox.at(index);
    for (auto rank = 0ul; rank < numberOfRanks; ++rank) {
      const auto offset = offsets[rank][index - startIndex];
      const auto length = numberOfEventsPerRank[rank];
      const auto total = offset + length;
      tot += length;
      std::move(receiveBuffer[rank].begin() + offset,
                receiveBuffer[rank].begin() + total,
                std::back_inserter(eventList));
    }
  }
  sync = MPI_Barrier(comm);
  if (sync != MPI_SUCCESS) {
    throw std::runtime_error("Sync failed");
  }

  return boxVsMDEvents;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// Definitions for: 8. Continue splitting
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
void ConvertToDistributedMD::continueSplitting() {
  auto root = m_boxStructureInformation.boxStructure.get();
  root->splitAllIfNeeded(nullptr);
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// Definitions for: 9. Update meta data
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
/**
 * Update the meta data stored in the boxes
 */
void ConvertToDistributedMD::updateMetaData() {
  // Since the box controller was duplicated onto several ranks we have several
  // things that we need to correct
  // 1. The fileID in the boxes will be incorrect for most boxes.
  // 2. A box controller which has the information about all of the boxes needs
  // to be created
  // 3. Update the data in the NullMDBoxes

  // --------------------------
  // 1. Updating file IDs
  //   a. Get the max file ID on the each rank and share
  //   b. Have each rank update its file IDs based on the file ID range of the
  //   other ranks
  // --------------------------
  const auto &communicator = this->communicator();
  std::vector<size_t> maxIds;
  all_gather(communicator,
             m_boxStructureInformation.boxController->getMaxId() - 1, maxIds);

  const auto localRank = communicator.rank();
  const auto offset = getOffset(localRank, m_maxIDBeforeSplit, maxIds);
  std::vector<API::IMDNode *> boxes;
  m_boxStructureInformation.boxStructure->getBoxes(boxes, 1000,
                                                   false /*leaves only*/);
  for (auto box : boxes) {
    // When only need to update the boxID if it is part of a newly created box
    auto boxID = box->getID();
    if (boxID > m_maxIDBeforeSplit) {
      boxID += offset;
      box->setID(boxID);
    }
  }

  // --------------------------------------
  // 2. Update the box controller settings. This requires updating:
  //   a. The maxID
  //   b. Number of boxes per level
  //   c. Number of grid boxes per level
  //   d. The maximum number of boxes per level / this can be reset via a method
  // --------------------------------------
  // MaxID
  const auto offsetLastRank =
      getOffset(communicator.size() - 1, m_maxIDBeforeSplit, maxIds);

  const auto maxId = maxIds[communicator.size() - 1] + offsetLastRank;
  // Number of boxes per level
  const auto mdBoxPerDepth = getBoxPerDepthInformation(
      communicator, m_boxStructureInformation.boxController->getNumMDBoxes());

  // Number of grid boxes per level
  const auto mdGridBoxPerDepth = getBoxPerDepthInformation(
      communicator,
      m_boxStructureInformation.boxController->getNumMDGridBoxes());

  // Apply changes to the box controller
  Mantid::API::BoxControllerAccess access;
  auto &boxController = *m_boxStructureInformation.boxController;
  boxController.setMaxId(maxId);
  access.setNumMDBoxes(boxController, mdBoxPerDepth);
  access.setNumMDGridBoxes(boxController, mdGridBoxPerDepth);

  // --------------------------------------
  // 2. Update values in NullMDBoxes
  //    TODO: Update the signals stored in the NullMDBoxes. This would be nice
  //    to have as a check in order to
  //          to evaluate scaling up to this point.
  // --------------------------------------
}

size_t
ConvertToDistributedMD::getOffset(int rank, size_t initialMaxID,
                                  const std::vector<size_t> &maxIDs) const {
  // Calculate what the number of new boxes on each rank is
  auto numberOfBoxesOnRanks = maxIDs;
  std::transform(numberOfBoxesOnRanks.begin(),
                 numberOfBoxesOnRanks.begin() + rank,
                 numberOfBoxesOnRanks.begin(),
                 [initialMaxID](size_t value) { return value - initialMaxID; });
  return std::accumulate(numberOfBoxesOnRanks.begin(),
                         numberOfBoxesOnRanks.begin() + rank, 0ul);
}

std::vector<size_t> ConvertToDistributedMD::getBoxPerDepthInformation(
    const Mantid::Parallel::Communicator &communicator,
    std::vector<size_t> numberBoxesPerDepth) const {
  std::vector<size_t> depthBoxes;
  all_gather(communicator, numberBoxesPerDepth.size(), depthBoxes);
  const auto maxDepth = *std::max_element(depthBoxes.begin(), depthBoxes.end());

  numberBoxesPerDepth.resize(maxDepth);
  std::vector<size_t> allNumberBoxes;
  all_gather(communicator, numberBoxesPerDepth.data(),
             static_cast<int>(numberBoxesPerDepth.size()), allNumberBoxes);

  std::vector<size_t> accumulatedAllNumberBoxes;
  const auto numberOfRanks = static_cast<size_t>(communicator.size());
  for (auto depth = 0ul; depth < maxDepth; ++depth) {
    auto sum = 0ul;
    for (auto rank = 0ul; rank < numberOfRanks; ++rank) {
      sum += allNumberBoxes[depth + rank * maxDepth];
    }
    accumulatedAllNumberBoxes.push_back(sum);
  }
  return accumulatedAllNumberBoxes;
}

} // namespace MDAlgorithms
} // namespace Mantid
