#include <string>
#include <map>

#include "../../parser/msg_parser.h"

/*
$var(egress_ip) = "52.49.92.167";
        $var(egress_port) = "5060";

        ## Outbound Routing Information
        ##$var(termination_ip) = "185.32.78.68";        # Adam Testing Gateway
        $var(termination_ip) = "185.32.78.25";          # GreenfieldTech Optimus
        $var(termination_port) = "5060";
        $var(termination_suffix) = "";
        $var(termination_prefix) = "92162";
*/



class RouteData {
  int mEgressPort;
  std::string mTerminationIp;
  int mTerminationPort;
  std::string mSuffix;
  std::string mPrefix;
  std::string mRouteString;
public:
  RouteData(int egressPort, std::string terminationIp, int terminationPort, std::string suffix, std::string prefix, std::string routeString);
  int getEgressPort();
  std::string getEgressIp();
  std::string getTerminationIp();
  int getTerminationPort();
  std::string  getSuffix();
  std::string  getPrefix();
  std::string  getRouteString();
};



class SphereDB {
  static SphereDB * sInstance;
  std::map<std::string,RouteData *> mRoutes;
public:
  static SphereDB * getInstance();
  int allocateRoute(hdr_field * callid, char * ip, char * number);
  int releaseRoute(hdr_field * callid);
  RouteData * routeDipp(std::string ip, std::string number);
  RouteData * getRouteDataForCall(hdr_field * callid);
  char * getEgressIp(hdr_field * callid);
  char * getTerminationIp(hdr_field * callid);
  char * getSuffix(hdr_field * callid);
  char * getPrefix(hdr_field * callid);
  char * getRoute(hdr_field * callid);
  unsigned int getTerminationPort(hdr_field * callid);
  unsigned int getEgressPort(hdr_field * callid);
};
