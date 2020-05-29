/**
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed in accordance with the terms specified in
 *  the LICENSE file found in the root directory of this source tree.
 */

#include <osquery/logger.h>

#include <osquery/tables/applications/browser_utils.h>
#include <osquery/utils/info/platform_type.h>

namespace fs = boost::filesystem;

namespace osquery {
namespace tables {

#ifdef WIN32
#pragma warning(disable : 4503)
#endif

static std::vector<fs::path> getChromePaths() {
  std::vector<fs::path> chromePaths;

  /// Each home directory will include custom extensions.
  if (isPlatform(PlatformType::TYPE_WINDOWS)) {
    chromePaths.push_back(
        "\\AppData\\Local\\Google\\Chrome\\User Data\\%\\Extensions\\");
  } else if (isPlatform(PlatformType::TYPE_OSX)) {
    chromePaths.push_back(
        "/Library/Application Support/Google/Chrome/%/Extensions/");
  } else {
    chromePaths.push_back("/.config/google-chrome/%/Extensions/");
  }

  if (isPlatform(PlatformType::TYPE_WINDOWS)) {
    chromePaths.push_back("\\AppData\\Roaming\\brave\\Extensions\\");
  } else if (isPlatform(PlatformType::TYPE_OSX)) {
    chromePaths.push_back(
        "/Library/Application "
        "Support/BraveSoftware/Brave-Browser/%/Extensions/");
  } else {
    chromePaths.push_back("/.config/BraveSoftware/Brave-Browser/%/Extensions/");
  }

  if (isPlatform(PlatformType::TYPE_WINDOWS)) {
    chromePaths.push_back("\\AppData\\Local\\Chromium\\Extensions\\");
  } else if (isPlatform(PlatformType::TYPE_OSX)) {
    chromePaths.push_back(
        "/Library/Application Support/Chromium/%/Extensions/");
  } else {
    chromePaths.push_back("/.config/chromium/%/Extensions/");
  }

  return chromePaths;
}

QueryData genChromeExtensions(QueryContext& context) {
  return genChromeBasedExtensions(context, getChromePaths());
}

QueryData genChromeExtensionContentScripts(QueryContext& context) {
  return genChromeBasedExtensionContentScripts(context, getChromePaths());
}

} // namespace tables
} // namespace osquery
