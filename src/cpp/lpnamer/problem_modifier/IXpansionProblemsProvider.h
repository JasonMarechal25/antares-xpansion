//
// Created by marechaljas on 25/11/22.
//

#pragma once

#include "../model/Problem.h"
#include "LpsFromAntares.h"

class IXpansionProblemsProvider {
 public:
  virtual ~IXpansionProblemsProvider() = default;
  [[nodiscard]] virtual std::vector<std::shared_ptr<Problem>> provideProblems(
      const std::string& solver_name,
      const std::filesystem::path& log_file_path) const = 0;
};
