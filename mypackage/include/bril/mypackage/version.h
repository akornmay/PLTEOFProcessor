// $Id$
#ifndef _bril_mypackage_version_h_
#define _bril_mypackage_version_h_
#include "config/PackageInfo.h"
#define BRILMYPACKAGE_VERSION_MAJOR 1
#define BRILMYPACKAGE_VERSION_MINOR 6
#define BRILMYPACKAGE_VERSION_PATCH 1
#define BRILMYPACKAGE_PREVIOUS_VERSIONS

// Template macros
//
#define BRILMYPACKAGE_VERSION_CODE PACKAGE_VERSION_CODE(BRILMYPACKAGE_VERSION_MAJOR,BRILMYPACKAGE_VERSION_MINOR,BRILMYPACKAGE_VERSION_PATCH)
#ifndef BRILMYPACKAGE_PREVIOUS_VERSIONS
#define BRILMYPACKAGE_FULL_VERSION_LIST  PACKAGE_VERSION_STRING(BRILMYPACKAGE_VERSION_MAJOR,BRILMYPACKAGE_VERSION_MINOR,BRILMYPACKAGE_VERSION_PATCH)
#else
#define BRILMYPACKAGE_FULL_VERSION_LIST  BRILMYPACKAGE_PREVIOUS_VERSIONS "," PACKAGE_VERSION_STRING(BRILMYPACKAGE_VERSION_MAJOR,BRILMYPACKAGE_VERSION_MINOR,BRILMYPACKAGE_VERSION_PATCH)
#endif
namespace brilmypackage{
  const std::string package = "brilmypackage";
  const std::string versions = BRILMYPACKAGE_FULL_VERSION_LIST;
  const std::string summary = "BRIL dummy package";
  const std::string description = "xdaq examples";
  const std::string authors = "ME";
  const std::string link = "";
  config::PackageInfo getPackageInfo ();
  void checkPackageDependencies () throw (config::PackageInfo::VersionException);
  std::set<std::string, std::less<std::string> > getPackageDependencies ();
}
#endif
