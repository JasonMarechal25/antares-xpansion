"""
    Class to control the execution of Benders
"""

import glob
import os
import subprocess
import sys
from pathlib import Path

from antares_xpansion.logger import step_logger
from antares_xpansion.study_output_cleaner import StudyOutputCleaner


class BendersDriver:
    def __init__(self, benders, benders_by_batch, merge_mps, options_file) -> None:

        self.oversubscribe = False
        self.allow_run_as_root = False
        self.benders = benders
        self.merge_mps = merge_mps
        self.benders_by_batch = benders_by_batch
        self.logger = step_logger(__name__, __class__.__name__)

        if (options_file != ""):
            self.options_file = options_file
        else:
            raise BendersDriver.BendersOptionsFileError(
                "Invalid Options File!")

        self.MPI_N = "-n"
        self._initialise_system_specific_mpi_vars()

    def launch(self, simulation_output_path, method, keep_mps=False, n_mpi=1, oversubscribe=False, allow_run_as_root=False):
        """
        launch the optimization of the antaresXpansion problem using the specified solver

        """
        self.logger.info("Benders")
        self.method = method
        self.n_mpi = n_mpi
        self.oversubscribe = oversubscribe
        self.allow_run_as_root = allow_run_as_root
        self.simulation_output_path = simulation_output_path
        old_cwd = os.getcwd()
        lp_path = self.get_lp_path()

        os.chdir(lp_path)
        self.logger.info(f"Current directory is now: {os.getcwd()}")

        self.set_solver()

        # delete execution logs
        self._clean_log_files()

        ret = subprocess.run(
            self._get_solver_cmd(), shell=False, stdout=sys.stdout, stderr=sys.stderr,
            encoding='utf-8')

        if ret.returncode != 0:
            raise BendersDriver.BendersExecutionError(
                f"ERROR: exited solver with status {ret.returncode}"
            )
        elif not keep_mps:
            StudyOutputCleaner.clean_benders_step(self.simulation_output_path)
        os.chdir(old_cwd)

    def set_simulation_output_path(self, simulation_output_path: Path):
        if simulation_output_path.is_dir():
            self._simulation_output_path = simulation_output_path
        else:
            raise BendersDriver.BendersOutputPathError(
                f"Benders Error: {simulation_output_path} not found "
            )

    def get_simulation_output_path(self):
        return self._simulation_output_path

    def get_lp_path(self):
        lp_path = Path(
            os.path.normpath(os.path.join(self.simulation_output_path, "lp"))
        )
        if lp_path.is_dir():
            return lp_path
        else:
            raise BendersDriver.BendersLpPathError(
                f"Error in lp path: {lp_path} not found"
            )

    def set_solver(self):
        if self.method == "benders":
            self.solver = self.benders
        elif self.method == "mergeMPS":
            self.solver = self.merge_mps
        elif self.method == "benders_by_batch":
            self.solver = self.benders_by_batch
        else:
            self.logger.error("Illegal optim method")
            raise BendersDriver.BendersSolverError(
                f" {self.method} method is unavailable !"
            )

    def _clean_log_files(self):
        solver_name = Path(self.solver).name
        logfile_list = glob.glob("./" + solver_name + "Log*")
        for file_path in logfile_list:
            try:
                os.remove(file_path)
            except OSError:
                self.logger.error("Error while deleting file : ", file_path)
        if os.path.isfile(solver_name + ".log"):
            os.remove(solver_name + ".log")

    def _get_solver_cmd(self):
        """
        returns a list consisting of the path to the required solver and its launching options
        """
        bare_solver_command = [self.solver, self.options_file]
        if self.n_mpi > 1:
            mpi_command = self.get_mpi_run_command_root()
            mpi_command.extend(bare_solver_command)
            return mpi_command
        else:
            return bare_solver_command

    def get_mpi_run_command_root(self):

        mpi_command = [self.MPI_LAUNCHER, self.MPI_N, str(self.n_mpi)]
        if sys.platform.startswith("linux"):
            if self.oversubscribe:
                mpi_command.append("--oversubscribe")
            if self.allow_run_as_root:
                mpi_command.append("--allow-run-as-root")
        return mpi_command

    def _initialise_system_specific_mpi_vars(self):
        if sys.platform.startswith("win32"):
            self.MPI_LAUNCHER = "mpiexec"
        elif sys.platform.startswith("linux"):
            self.MPI_LAUNCHER = "mpirun"
        else:
            raise (
                BendersDriver.BendersUnsupportedPlatform(
                    f"Error {sys.platform} platform is not supported \n"
                )
            )

    simulation_output_path = property(
        get_simulation_output_path, set_simulation_output_path
    )

    class BendersOutputPathError(Exception):
        pass

    class BendersUnsupportedPlatform(Exception):
        pass

    class BendersLpPathError(Exception):
        pass

    class BendersSolverError(Exception):
        pass

    class BendersExecutionError(Exception):
        pass

    class BendersOptionsFileError(Exception):
        pass
