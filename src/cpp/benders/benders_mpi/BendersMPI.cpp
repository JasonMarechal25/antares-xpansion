
#include "BendersMPI.h"

#include <algorithm>
#include <utility>

#include "Timer.h"
#include "glog/logging.h"

BendersMpi::BendersMpi(BendersBaseOptions const &options, Logger logger,
                       Writer writer, mpi::environment &env,
                       mpi::communicator &world)
    : BendersBase(options, logger, std::move(writer)),
      _env(env),
      _world(world) {}

/*!
 *  \brief Method to load each problem in a thread
 *
 *  The initialization of each problem is done sequentially
 *
 */

void BendersMpi::InitializeProblems() {
  MatchProblemToId();

  BuildMasterProblem();
  int current_problem_id = 0;
  // Dispatch subproblems to process
  for (const auto &problem : coupling_map) {
    // In case there are more subproblems than process
    if (auto process_to_feed = current_problem_id % _world.size();
        process_to_feed ==
        _world.rank()) {  // Assign  [problemNumber % processCount] to processID

      const auto subProblemFilePath = GetSubproblemPath(problem.first);
      AddSubproblem(problem);
      AddSubproblemName(problem.first);
    }
    current_problem_id++;
  }
}
void BendersMpi::BuildMasterProblem() {
  if (_world.rank() == rank_0) {
    reset_master(new WorkerMaster(master_variable_map, get_master_path(),
                                  get_solver_name(), get_log_level(),
                                  _data.nsubproblem, log_name(), IsResumeMode(),
                                  _logger));
  }
}
/*!
 *  \brief Solve, get and send solution of the Master Problem to every thread
 *
 *  \param _env : environment variable for mpi communication
 *
 *  \param _world : communicator variable for mpi communication
 */
void BendersMpi::step_1_solve_master() {
  int success = 1;
  try {
    do_solve_master_create_trace_and_update_cuts();
  } catch (std::exception const &ex) {
    success = 0;
    write_exception_message(ex);
  }
  check_if_some_proc_had_a_failure(success);
  BroadcastXCut();
}

void BendersMpi::do_solve_master_create_trace_and_update_cuts() {
  if (_world.rank() == rank_0) {
    if (SwitchToIntegerMaster(_data.is_in_initial_relaxation)) {
      _logger->LogAtSwitchToInteger();
      ActivateIntegrityConstraints();
      ResetDataPostRelaxation();
    }
    solve_master_and_create_trace();
  }
}

void BendersMpi::BroadcastXCut() {
  if (!_exceptionRaised) {
    Point x_cut = get_x_cut();
    mpi::broadcast(_world, x_cut, rank_0);
    set_x_cut(x_cut);
  }
}

void BendersMpi::solve_master_and_create_trace() {
  _logger->log_at_initialization(_data.it + GetNumIterationsBeforeRestart());
  _logger->display_message("\tSolving master...");
  get_master_value();
  _logger->log_master_solving_duration(get_timer_master());

  ComputeXCut();
  _logger->log_iteration_candidates(bendersDataToLogData(_data));
}

/*!
 *  \brief Get cut information from each Subproblem and add it to the Master
 * problem
 *
 * Get cut information of every Subproblem in each thread and send it to
 * thread 0 to build new Master's cuts
 *
 */
void BendersMpi::step_2_solve_subproblems_and_build_cuts() {
  int success = 1;
  SubProblemDataMap subproblem_data_map;
  Timer walltime;
  Timer subproblems_timer_per_proc;
  try {
    subproblem_data_map = get_subproblem_cut_package();
    SetSubproblemsCpuTime(subproblems_timer_per_proc.elapsed());

  } catch (std::exception const &ex) {
    success = 0;
    write_exception_message(ex);
  }
  check_if_some_proc_had_a_failure(success);
  gather_subproblems_cut_package_and_build_cuts(subproblem_data_map, walltime);
  if (Rank() == rank_0) {
    _data.number_of_subproblem_resolved += _data.nsubproblem;
    _logger->cumulative_number_of_sub_problem_resolved(
        _data.number_of_subproblem_resolved +
        GetNumOfSubProblemsResolvedBeforeResume());
  }
}

void BendersMpi::gather_subproblems_cut_package_and_build_cuts(
    const SubProblemDataMap &subproblem_data_map, const Timer &walltime) {
  if (!_exceptionRaised) {
    std::vector<SubProblemDataMap> gathered_subproblem_map;
    mpi::gather(_world, subproblem_data_map, gathered_subproblem_map, rank_0);
    SetSubproblemsWalltime(walltime.elapsed());
    double cumulative_subproblems_timer_per_iter(0);
    Reduce(GetSubproblemsCpuTime(), cumulative_subproblems_timer_per_iter,
           std::plus<double>(), rank_0);
    SetSubproblemsCumulativeCpuTime(cumulative_subproblems_timer_per_iter);
    master_build_cuts(gathered_subproblem_map);
  }
}

SubProblemDataMap BendersMpi::get_subproblem_cut_package() {
  SubProblemDataMap subproblem_data_map;
  GetSubproblemCut(subproblem_data_map);
  return subproblem_data_map;
}

void BendersMpi::master_build_cuts(
    std::vector<SubProblemDataMap> gathered_subproblem_map) {
  SetSubproblemCost(0);
  for (const auto &subproblem_data_map : gathered_subproblem_map) {
    for (auto &&[_, subproblem_data] : subproblem_data_map) {
      SetSubproblemCost(GetSubproblemCost() + subproblem_data.subproblem_cost);
    }
  }

  _logger->display_message("\tSolving subproblems...");

  _data.ub = 0;
  for (const auto &subproblem_data_map : gathered_subproblem_map) {
    BuildCutFull(subproblem_data_map);
  }
  _logger->LogSubproblemsSolvingCumulativeCpuTime(
      GetSubproblemsCumulativeCpuTime());
  _logger->LogSubproblemsSolvingWalltime(GetSubproblemsWalltime());
}

/*!
 *  \brief Gather, store and sort all process results in a set
 *
 *  \param _env : environment variable for mpi communication
 *
 *  \param _world : communicator variable for mpi communication
 */

void BendersMpi::check_if_some_proc_had_a_failure(int success) {
  int global_success;
  mpi::all_reduce(_world, success, global_success, mpi::bitwise_and<int>());
  if (global_success == 0) {
    _exceptionRaised = true;
  }
}

void BendersMpi::write_exception_message(const std::exception &ex) const {
  std::string error = "Exception raised : " + std::string(ex.what());
  LOG(WARNING) << error << std::endl;
  _logger->display_message(error);
}

void BendersMpi::step_4_update_best_solution(int rank,
                                             const Timer &timer_master) {
  if (rank == rank_0) {
    compute_ub();
    update_best_ub();
    _logger->log_at_iteration_end(bendersDataToLogData(_data));

    UpdateTrace();

    _data.elapsed_time = GetBendersTime();
    set_timer_master(timer_master.elapsed());
    _data.stop = ShouldBendersStop();
  }
}

/*!
 *  \brief Method to free the memory used by each problem
 */
void BendersMpi::free() {
  if (_world.rank() == rank_0) {
    free_master();
  } else {
    free_subproblems();
  }
  _world.barrier();
}

/*!
 *  \brief Run Benders algorithm in parallel
 *
 *  Method to run Benders algorithm in parallel
 *
 */
void BendersMpi::Run() {
  PreRunInitialization();
  while (!_data.stop) {
    Timer timer_master;
    ++_data.it;

    /*Solve Master problem, get optimal value and cost and send it to process*/
    step_1_solve_master();

    /*Gather cut from each subproblem in master thread and add them to Master
     * problem*/
    if (!_exceptionRaised) {
      step_2_solve_subproblems_and_build_cuts();
    }

    if (!_exceptionRaised) {
      step_4_update_best_solution(_world.rank(), timer_master);
    }
    _data.stop |= _exceptionRaised;

    broadcast(_world, _data.is_in_initial_relaxation, rank_0);
    broadcast(_world, _data.stop, rank_0);
    if (_world.rank() == rank_0) {
      SaveCurrentBendersData();
    }
  }
  if (_world.rank() == rank_0) {
    CloseCsvFile();
    EndWritingInOutputFile();
    write_basis();
  }
  _world.barrier();
}
void BendersMpi::PreRunInitialization() {
  init_data();

  if (_world.rank() == rank_0) {
    if (is_initial_relaxation_requested()) {
      _logger->LogAtInitialRelaxation();
      DeactivateIntegrityConstraints();
      SetDataPreRelaxation();
    }
  }

  _world.barrier();

  if (_world.rank() == rank_0) {
    ChecksResumeMode();
    if (is_trace()) {
      OpenCsvFile();
    }
  }
}
void BendersMpi::launch() {
  build_input_map();
  _world.barrier();

  InitializeProblems();
  _world.barrier();

  Run();
  _world.barrier();

  post_run_actions();

  free();
  _world.barrier();
}
