/**
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed in accordance with the terms specified in
 *  the LICENSE file found in the root directory of this source tree.
 */

#include <regex>
#include <string>

#include <osquery/logger.h>
#include <osquery/tables.h>

#include <osquery/utils/conversions/split.h>
#include <osquery/utils/conversions/tryto.h>

#include <osquery/core/windows/wmi.h>

namespace osquery {
namespace tables {

const auto kHPBiosSettingRegex = std::regex("\\*([\\w ]*)");
const std::vector<std::string> kHP = {
    "hp", "hewlett-packard", "hewlett packard"};
const std::vector<std::string> kLenovo = {"lenovo"};
const std::vector<std::string> kDell = {"dell inc."};
const std::map<std::string, std::pair<std::string, BSTR>> kQueryMap = {
    {"hp",
     {"select Name,Value from HP_BiosSetting",
      (BSTR) "root\\hp\\instrumentedBIOS"}},
    {"lenovo",
     {"select CurrentSetting from Lenovo_BiosSetting", (BSTR) "root\\wmi"}},
    {"dell",
     {"select AttributeName,CurrentValue,PossibleValues, "
      "PossibleValuesDescription from DCIM_BIOSEnumeration",
      (BSTR) "root\\dcim\\sysman"}}};

std::string getManufacturer(std::string manufacturer) {
  transform(manufacturer.begin(),
            manufacturer.end(),
            manufacturer.begin(),
            ::tolower);

  if (std::find(kHP.begin(), kHP.end(), manufacturer) != kHP.end()) {
    manufacturer = "hp";
  } else if (std::find(kLenovo.begin(), kLenovo.end(), manufacturer) !=
             kLenovo.end()) {
    manufacturer = "lenovo";
  } else if (std::find(kDell.begin(), kDell.end(), manufacturer) !=
             kDell.end()) {
    manufacturer = "dell";
  }

  return manufacturer;
}

Row getHPBiosInfo(const WmiResultItem& item) {
  Row r;

  std::string value;
  std::smatch matches;
  item.GetString("Name", r["name"]);
  item.GetString("Value", value);

  if (std::regex_search(value, matches, kHPBiosSettingRegex)) {
    r["value"] = std::string(matches[1]);
  } else {
    r["value"] = value;
  }

  return r;
}

Row getLenovoBiosInfo(const WmiResultItem& item) {
  Row r;

  std::string currentSetting;
  std::vector<std::string> settings;
  item.GetString("CurrentSetting", currentSetting);
  settings = osquery::split(currentSetting, ",");

  if (settings.size() != 2) {
    return r;
  }
  r["name"] = settings[0];
  r["value"] = settings[1];

  return r;
}

Row getDellBiosInfo(const WmiResultItem& item) {
  Row r;

  std::vector<std::string> vCurrentValue;
  std::vector<std::string> vPossibleValues;
  std::vector<std::string> vPossibleValuesDescription;
  item.GetString("AttributeName", r["name"]);
  item.GetVectorOfStrings("CurrentValue", vCurrentValue);
  item.GetVectorOfStrings("PossibleValues", vPossibleValues);
  item.GetVectorOfStrings("PossibleValuesDescription",
                          vPossibleValuesDescription);

  if (vCurrentValue.size() == 1 && !vPossibleValues.empty()) {
    auto pos = std::find(
        vPossibleValues.begin(), vPossibleValues.end(), vCurrentValue[0]);
    if (pos != vPossibleValues.end()) {
      r["value"] = vPossibleValuesDescription[pos - vPossibleValues.begin()];
    } else {
      r["value"] = "N/A";
    }

  } else if (vCurrentValue.size() > 1) {
    std::ostringstream oValueConcat;
    std::copy(vCurrentValue.begin(),
              vCurrentValue.end() - 1,
              std::ostream_iterator<std::string>(oValueConcat, ","));
    oValueConcat << vCurrentValue.back();

    r["value"] = oValueConcat.str();

  } else {
    r["value"] = "N/A";
  }

  return r;
}

QueryData genBiosInfo(QueryContext& context) {
  QueryData results;
  std::string manufacturer;

  const WmiRequest wmiComputerSystemReq(
      "select Manufacturer from Win32_ComputerSystem");
  const std::vector<WmiResultItem>& wmiComputerSystemResults =
      wmiComputerSystemReq.results();

  if (!wmiComputerSystemResults.empty()) {
    wmiComputerSystemResults[0].GetString("Manufacturer", manufacturer);
    manufacturer = getManufacturer(manufacturer);
  } else {
    return results;
  }

  if (kQueryMap.find(manufacturer) != kQueryMap.end()) {
    const WmiRequest wmiBiosReq(std::get<0>(kQueryMap.at(manufacturer)),
                                (std::get<1>(kQueryMap.at(manufacturer))));
    const std::vector<WmiResultItem>& wmiResults = wmiBiosReq.results();

    for (unsigned int i = 0; i < wmiResults.size(); ++i) {
      Row r;

      if (manufacturer == "hp") {
        r = getHPBiosInfo(wmiResults[i]);

      } else if (manufacturer == "lenovo") {
        r = getLenovoBiosInfo(wmiResults[i]);

      } else if (manufacturer == "dell") {
        r = getDellBiosInfo(wmiResults[i]);
      }
      if (!r.empty()) {
        results.push_back(r);
      }
    }
  } else {
    LOG(INFO) << "Vendor \"" << manufacturer << "\" is currently not supported";
  }
  return results;
}
} // namespace tables
} // namespace osquery
