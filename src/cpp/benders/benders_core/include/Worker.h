#pragma once

#include <filesystem>

#include "ILogger.h"
#include "OutputWriter.h"
#include "common.h"
#include "multisolver_interface/Solver.h"
class Worker;
typedef std::shared_ptr<Worker> WorkerPtr;

/*!
 * \class Worker
 * \brief Mother-class Worker
 *
 *  This class opens and sets a problem from a mps and a mapping variable map
 */

class Worker {
 public:
  explicit Worker(Logger logger) : logger_(std::move(logger)) {}
  void init(VariableMap const &variable_map,
            const std::filesystem::path &path_to_mps,
            std::string const &solver_name, int log_level,
            const std::filesystem::path &log_name);
  virtual ~Worker() = default;

  void get_value(double &lb) const;

  void get_splex_num_of_ite_last(int &result) const;

  void free();
  void write_basis(const std::filesystem::path &filename) const;
  virtual SolverAbstract::Ptr solver() const { return _solver; }

 public:
  std::filesystem::path _path_to_mps;
  VariableMap
      _name_to_id; /*!< Link between the variable name and its identifier */
  Int2Str
      _id_to_name; /*!< Link between the identifier of a variable and its name*/

 public:
  void solve(int &lp_status, const std::string &outputroot,
             const std::string &output_master_mps_file_name,
             Writer writer) const;

 public:
  SolverAbstract::Ptr _solver =
      nullptr; /*!< Problem stocked in the instance Worker*/
  bool _is_master = false;

  Logger logger_;
};
