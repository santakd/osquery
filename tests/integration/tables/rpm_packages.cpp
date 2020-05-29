/**
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed in accordance with the terms specified in
 *  the LICENSE file found in the root directory of this source tree.
 */

// Sanity check integration test for rpm_packages
// Spec file: specs/linux/rpm_packages.table

#include <osquery/tests/integration/tables/helper.h>

#include <osquery/logger.h>

namespace osquery {
namespace table_tests {

class rpmPackages : public testing::Test {
 protected:
  void SetUp() override {
    setUpEnvironment();
  }
};

TEST_F(rpmPackages, test_sanity) {
  auto const rows = execute_query("select * from rpm_packages");
  if (rows.size() > 0) {
    ValidationMap row_map = {{"name", NonEmptyString},
                             {"version", NormalType},
                             {"release", NormalType},
                             {"source", NormalType},
                             {"size", IntType},
                             {"sha1", NonEmptyString},
                             {"arch", NonEmptyString},
                             {"epoch", IntType},
                             {"install_time", IntType},
                             {"vendor", NonEmptyString},
                             {"package_group", NonEmptyString}};

    validate_rows(rows, row_map);
  } else {
    LOG(WARNING) << "Empty results of query from 'rpm_packages', assume there "
                    "is no rpm in the system";
  }
}
} // namespace table_tests
} // namespace osquery
