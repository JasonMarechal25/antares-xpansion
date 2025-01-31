#include "Worker.h"

#include "LogUtils.h"
#include "glog/logging.h"
#include "solver_utils.h"
/*!
 *  \brief Free the problem
 */
void Worker::free() {
  if (_solver) {
    _solver.reset();
    _solver = nullptr;
  }
}

/*!
 *  \brief Return the optimal value of a problem
 *
 *  \param lb : double which receives the optimal value
 */
void Worker::get_value(double &lb) const {
  if (_is_master && _solver->get_n_integer_vars() > 0) {
    lb = _solver->get_mip_value();
  } else {
    lb = _solver->get_lp_value();
  }
}

/*!
 *  \brief Initialization of a problem
 *
 *  \param variable_map : map linking each problem name to its variables and
 * their ids
 *
 *  \param problem_name : name of the problem
 */
void Worker::init(VariableMap const &variable_map,
                  const std::filesystem::path &path_to_mps,
                  std::string const &solver_name, int log_level,
                  const std::filesystem::path &log_name) {
  _path_to_mps = path_to_mps;
  SolverFactory factory;
  if (_is_master) {
    _solver =
        factory.create_solver(solver_name, SOLVER_TYPE::INTEGER, log_name);
  } else {
    _solver =
        factory.create_solver(solver_name, SOLVER_TYPE::CONTINUOUS, log_name);
  }

  _solver->init();
  _solver->set_threads(1);
  _solver->set_output_log_level(log_level);
  _solver->read_prob_mps(path_to_mps);

  int var_index;
  for (auto const &kvp : variable_map) {
    var_index = _solver->get_col_index(kvp.first);
    _id_to_name[var_index] = kvp.first;
    _name_to_id[kvp.first] = var_index;
  }
}

/*!
 *  \brief Method to solve a problem
 *
 *  \param lp_status : problem status after optimization
 */
void Worker::solve(int &lp_status, const std::string &outputroot,
                   const std::string &output_master_mps_file_name,
                   Writer writer) const {
  if (_is_master && _solver->get_n_integer_vars() > 0) {
    lp_status = _solver->solve_mip();
  } else {
    lp_status = _solver->solve_lp();
  }

  if (lp_status != SOLVER_STATUS::OPTIMAL) {
    std::filesystem::path error_file_path;
    auto problem_status = _solver->SOLVER_STRING_STATUS[lp_status];
    error_file_path = std::filesystem::path(outputroot) /
                      (_path_to_mps.filename().string() + "_lp_status_" +
                       problem_status + MPS_SUFFIX);
    std::ostringstream msg;
    msg << "lp_status is : " << problem_status << std::endl;

    msg << "written in " << error_file_path.string() << std::endl;
    logger_->display_message(msg.str());
    _solver->write_prob_mps(error_file_path);
    Output::ProblemData data;
    data.name = _path_to_mps.filename().string();
    data.path = error_file_path;
    data.status = problem_status;
    writer->WriteProblem(data);
    writer->dump();
    auto log_location = LOGLOCATION;
    msg.str("");
    msg << "Invalid solver status " + problem_status + " optimality expected";
    logger_->display_message(log_location + msg.str());
    throw InvalidSolverStatusException(msg.str(), log_location);
  }

  if (_is_master) {
    _solver->write_prob_mps(std::filesystem::path(outputroot) /
                            output_master_mps_file_name);
  }
}
/*!
 *  \brief Get the number of iteration needed to solve a problem
 *
 *  \param result : result
 */
void Worker::get_splex_num_of_ite_last(int &result) const {
  result = _solver->get_splex_num_of_ite_last();
}

void Worker::write_basis(const std::filesystem::path &filename) const {
  _solver->write_basis(filename);
}