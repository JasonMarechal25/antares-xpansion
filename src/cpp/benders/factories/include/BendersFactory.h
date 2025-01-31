#ifndef ANTARES_XPANSION_SRC_CPP_BENDERS_FACTORIES_INCLUDE_BENDERSFACTORY_H
#define ANTARES_XPANSION_SRC_CPP_BENDERS_FACTORIES_INCLUDE_BENDERSFACTORY_H
#include "BendersByBatch.h"
#include "BendersMPI.h"
#include "BendersSequential.h"
#include "ILogger.h"
#include "OutputWriter.h"
enum class BENDERSMETHOD { BENDERS, BENDERSBYBATCH, MERGEMPS };

class BendersMainFactory {
 private:
  char** argv_;
  BENDERSMETHOD method_;
  std::filesystem::path options_file_;
  boost::mpi::environment* penv_ = nullptr;
  boost::mpi::communicator* pworld_ = nullptr;

 public:
  explicit BendersMainFactory(int argc, char** argv,
                              const BENDERSMETHOD& method,
                              boost::mpi::environment& env,
                              boost::mpi::communicator& world);
  explicit BendersMainFactory(int argc, char** argv,
                              const BENDERSMETHOD& method,
                              const std::filesystem::path& options_file,
                              boost::mpi::environment& env,
                              boost::mpi::communicator& world);
  int Run() const;
};
#endif  // ANTARES_XPANSION_SRC_CPP_BENDERS_FACTORIES_INCLUDE_BENDERSFACTORY_H