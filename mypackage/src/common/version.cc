// $Id$
#include "bril/mypackage/version.h"
#include "config/version.h"
#include "xcept/version.h"
#include "toolbox/version.h"
#include "xdaq/version.h"
#include "xdata/version.h"
#include "xoap/version.h"
#include "xgi/version.h"
#include "b2in/nub/version.h"

GETPACKAGEINFO(brilmypackage)

void brilmypackage::checkPackageDependencies() throw (config::PackageInfo::VersionException){
  CHECKDEPENDENCY(config);
  CHECKDEPENDENCY(xcept);
  CHECKDEPENDENCY(toolbox);
  CHECKDEPENDENCY(xdaq);
  CHECKDEPENDENCY(xdata);
  CHECKDEPENDENCY(xoap);
  CHECKDEPENDENCY(xgi);
  CHECKDEPENDENCY(b2innub);
}

std::set<std::string, std::less<std::string> > brilmypackage::getPackageDependencies(){
  std::set<std::string, std::less<std::string> > dependencies;
  
  ADDDEPENDENCY(dependencies,config);
  ADDDEPENDENCY(dependencies,xcept);
  ADDDEPENDENCY(dependencies,toolbox);
  ADDDEPENDENCY(dependencies,xdaq);
  ADDDEPENDENCY(dependencies,xdata);
  ADDDEPENDENCY(dependencies,xoap);
  ADDDEPENDENCY(dependencies,xgi);
  ADDDEPENDENCY(dependencies,b2innub);
  
  return dependencies;
}
