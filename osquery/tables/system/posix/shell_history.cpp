/**
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed in accordance with the terms specified in
 *  the LICENSE file found in the root directory of this source tree.
 */

#include <regex>
#include <string>
#include <vector>

#include <osquery/core.h>
#include <osquery/filesystem/filesystem.h>
#include <osquery/logger.h>
#include <osquery/tables.h>
#include <osquery/tables/system/system_utils.h>
#include <osquery/tables/system/posix/shell_history.h>
#include <osquery/utils/conversions/split.h>
#include <osquery/utils/system/system.h>

namespace osquery {
namespace tables {

const std::vector<std::string> kShellHistoryFiles = {
    ".bash_history", ".zsh_history", ".zhistory", ".history", ".sh_history",
};

void genShellHistoryFromFile(const std::string& uid,
                             const boost::filesystem::path& history_file,
                             QueryData& results) {
  std::string history_content;
  if (forensicReadFile(history_file, history_content).ok()) {
    std::regex bash_timestamp_rx("^#([0-9]+)$");
    std::regex zsh_timestamp_rx("^: {0,10}([0-9]{1,11}):[0-9]+;(.*)$");

    std::string prev_bash_timestamp;
    for (const auto& line : split(history_content, "\n")) {
      std::smatch bash_timestamp_matches;
      std::smatch zsh_timestamp_matches;

      if (prev_bash_timestamp.empty() &&
          std::regex_search(line, bash_timestamp_matches, bash_timestamp_rx)) {
        prev_bash_timestamp = bash_timestamp_matches[1];
        continue;
      }

      Row r;

      if (!prev_bash_timestamp.empty()) {
        r["time"] = INTEGER(prev_bash_timestamp);
        r["command"] = line;
        prev_bash_timestamp.clear();
      } else if (std::regex_search(
                     line, zsh_timestamp_matches, zsh_timestamp_rx)) {
        std::string timestamp = zsh_timestamp_matches[1];
        r["time"] = INTEGER(timestamp);
        r["command"] = zsh_timestamp_matches[2];
      } else {
        r["time"] = INTEGER(0);
        r["command"] = line;
      }

      r["uid"] = uid;
      r["history_file"] = history_file.string();
      results.push_back(r);
    }
  }
}

void genShellHistoryForUser(const std::string& uid,
                            const std::string& gid,
                            const std::string& directory,
                            QueryData& results) {
  for (const auto& hfile : kShellHistoryFiles) {
    boost::filesystem::path history_file = directory;
    history_file /= hfile;
    genShellHistoryFromFile(uid, history_file, results);
  }
}

void genShellHistoryFromBashSessions(const std::string& uid,
                                     const std::string& directory,
                                     QueryData& results) {
  boost::filesystem::path bash_sessions = directory;
  bash_sessions /= ".bash_sessions";

  if (pathExists(bash_sessions)) {
    bash_sessions /= "*.history";
    std::vector<std::string> session_hist_files;
    resolveFilePattern(bash_sessions, session_hist_files);

    for (const auto& hfile : session_hist_files) {
      boost::filesystem::path history_file = hfile;
      genShellHistoryFromFile(uid, history_file, results);
    }
  }
}

QueryData genShellHistory(QueryContext& context) {
  QueryData results;

  // Iterate over each user
  QueryData users = usersFromContext(context);
  for (const auto& row : users) {
    auto uid = row.find("uid");
    auto gid = row.find("gid");
    auto dir = row.find("directory");
    if (uid != row.end() && gid != row.end() && dir != row.end()) {
      genShellHistoryForUser(uid->second, gid->second, dir->second, results);
      genShellHistoryFromBashSessions(uid->second, dir->second, results);
    }
  }

  return results;
}
}
}
