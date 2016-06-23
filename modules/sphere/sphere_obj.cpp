#include "sphere.h"
#include <curl/curl.h>
#include <json-c/json.h>


SphereDB * SphereDB::sInstance = NULL;



RouteData::RouteData(int egressPort, std::string terminationIp, int terminationPort, std::string suffix, std::string prefix, std::string routeString) {
  mEgressPort = egressPort;
  mTerminationIp = terminationIp;
  mTerminationPort = terminationPort;
  mSuffix = suffix;
  mPrefix  = prefix;
  mRouteString = routeString;
}
int RouteData::getEgressPort() {
  return mEgressPort;
}

std::string RouteData::getEgressIp() {
  return mTerminationIp;
}

std::string RouteData::getTerminationIp() {
  return mTerminationIp;
}
int RouteData::getTerminationPort() {
  return mTerminationPort;
}
std::string  RouteData::getSuffix() {
  return mSuffix;
}
std::string  RouteData::getPrefix() {
  return mPrefix;
}
std::string  RouteData::getRouteString() {
  return mRouteString;
}

SphereDB *SphereDB::getInstance() {
  if(sInstance==NULL) {
    sInstance = new SphereDB();
  }
  return sInstance;
}

/* holder for curl fetch */
struct curl_fetch_st {
    char *payload;
    size_t size;
};

/* callback for curl fetch */
size_t curl_callback (void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;                             /* calculate buffer size */
    struct curl_fetch_st *p = (struct curl_fetch_st *) userp;   /* cast pointer to fetch struct */

    /* expand buffer */
    p->payload = (char *) realloc(p->payload, p->size + realsize + 1);

    /* check buffer */
    if (p->payload == NULL) {
      /* this isn't good */
      fprintf(stderr, "ERROR: Failed to expand buffer in curl_callback");
      /* free buffer */
      free(p->payload);
      /* return */
      return -1;
    }

    /* copy contents to buffer */
    memcpy(&(p->payload[p->size]), contents, realsize);

    /* set new buffer size */
    p->size += realsize;

    /* ensure null termination */
    p->payload[p->size] = 0;

    /* return size */
    return realsize;
}

/* fetch and return url body via curl */
CURLcode curl_fetch_url(CURL *ch, const char *url, struct curl_fetch_st *fetch) {
    CURLcode rcode;                   /* curl result code */

    /* init payload */
    fetch->payload = (char *) calloc(1, sizeof(fetch->payload));

    /* check payload */
    if (fetch->payload == NULL) {
        /* log error */
        fprintf(stderr, "ERROR: Failed to allocate payload in curl_fetch_url");
        /* return error */
        return CURLE_FAILED_INIT;
    }

    /* init size */
    fetch->size = 0;

    /* set url to fetch */
    curl_easy_setopt(ch, CURLOPT_URL, url);

    /* set calback function */
    curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, curl_callback);

    /* pass fetch struct pointer */
    curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void *) fetch);

    /* set default user agent */
    curl_easy_setopt(ch, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    /* set timeout */
    curl_easy_setopt(ch, CURLOPT_TIMEOUT, 5);

    /* enable location redirects */
    curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1);

    /* set maximum allowed redirects */
    curl_easy_setopt(ch, CURLOPT_MAXREDIRS, 1);

    /* fetch the url */
    rcode = curl_easy_perform(ch);

    /* return */
    return rcode;
}


RouteData * SphereDB::routeDipp(std::string ip, std::string number) {

    CURL *ch;                                               /* curl handle */
    CURLcode rcode;                                         /* curl result code */

    json_object *json;                                      /* json post body */
    enum json_tokener_error jerr = json_tokener_success;    /* json parse error */

    struct curl_fetch_st curl_fetch;                        /* curl fetch struct */
    struct curl_fetch_st *cf = &curl_fetch;                 /* pointer to fetch struct */
    struct curl_slist *headers = NULL;                      /* http headers to send with request */

    /* url to test site */
    char *url = "172.18.1.101:3001/RouteServer/getRoute";

    /* init curl handle */
    if ((ch = curl_easy_init()) == NULL) {
        /* log error */
        fprintf(stderr, "ERROR: Failed to create curl handle in fetch_session");
        /* return error */
        return NULL;
    }

    /* set content type */
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");

    /* create json object for post */
    json = json_object_new_object();

    /* build post data */
    json_object_object_add(json, "user_ip", json_object_new_string(ip.c_str()));
    json_object_object_add(json, "user_prefix", json_object_new_string(number.c_str()));

    /* set curl options */
    curl_easy_setopt(ch, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(ch, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(ch, CURLOPT_POSTFIELDS, json_object_to_json_string(json));

    /* fetch page and capture return code */
    rcode = curl_fetch_url(ch, url, cf);

    /* cleanup curl handle */
    curl_easy_cleanup(ch);

    /* free headers */
    curl_slist_free_all(headers);

    /* free json object */
    json_object_put(json);

    /* check return code */
    if (rcode != CURLE_OK || cf->size < 1) {
        /* log error */
        fprintf(stderr, "ERROR: Failed to fetch url (%s) - curl said: %s",
            url, curl_easy_strerror(rcode));
        /* return error */
        return NULL;
    }

    /* check payload */
    if (cf->payload != NULL) {
        /* print result */
        printf("CURL Returned: \n%s\n", cf->payload);
        /* parse return */
        json = json_tokener_parse_verbose(cf->payload, &jerr);
        /* free payload */
        free(cf->payload);
    } else {
        /* error */
        fprintf(stderr, "ERROR: Failed to populate payload");
        /* free payload */
        free(cf->payload);
        /* return */
        return NULL;
    }

    /* check error */
    if (jerr != json_tokener_success) {
        /* error */
        fprintf(stderr, "ERROR: Failed to parse json string");
        /* free json object */
        json_object_put(json);
        /* return */
        return NULL;
    }

    /* debugging */
    printf("Parsed JSON: %s\n", json_object_to_json_string(json));

    struct json_object * egressPortJson;
    json_object_object_get_ex(json, "egress_port", & egressPortJson);
    struct json_object * egressIpJson;
    json_object_object_get_ex(json, "egress_ip", & egressIpJson);
    struct json_object * terminationPortJson;
    json_object_object_get_ex(json, "termination_port", & terminationPortJson);
    struct json_object * terminationIpJson;
    json_object_object_get_ex(json, "termination_ip", & terminationIpJson);
    struct json_object * terminationSuffixJson;
    json_object_object_get_ex(json, "termination_suffix", & terminationSuffixJson);
    struct json_object * terminationPrefixJson;
    json_object_object_get_ex(json, "termination_prefix", & terminationPrefixJson);

    printf("0x10\n");
    atoi(json_object_get_string(egressPortJson));
    printf("0x11\n");
    json_object_get_string(egressIpJson);
    printf("0x12\n");
    atoi(json_object_get_string(terminationPortJson));
    printf("0x13\n");
    json_object_get_string(terminationIpJson);
    printf("0x14\n");
    json_object_get_string(terminationPrefixJson);
    printf("0x15\n");
    json_object_get_string(terminationSuffixJson);
    printf("0x16\n");

    RouteData * rd = new RouteData( atoi(json_object_get_string(egressPortJson)),
                                    json_object_get_string(egressIpJson),
                                    atoi(json_object_get_string(terminationPortJson)),
                                    json_object_get_string(terminationIpJson),
                                    json_object_get_string(terminationPrefixJson),
                                    json_object_get_string(terminationSuffixJson));

    json_object_put(json);
    return rd;

}

int SphereDB::allocateRoute(hdr_field * callid, char * ip, char * number) {
  RouteData * rd = routeDipp(ip,number);

  if(rd==NULL) {
    return 0;
  }

  char callIdStr[256];
  strncpy(callIdStr, callid->body.s, callid->body.len);
  callIdStr[callid->body.len] = 0;
  std::string key(callIdStr);
  mRoutes[key]=rd;

  return 1;
}



RouteData * SphereDB::getRouteDataForCall(hdr_field * callid) {
  char callIdStr[256];
  strncpy(callIdStr, callid->body.s, callid->body.len);
  callIdStr[callid->body.len] = 0;
  std::string key(callIdStr);
  return mRoutes[key];
}

char * SphereDB::getEgressIp(hdr_field * callid) {
   RouteData * rd = getRouteDataForCall(callid);

   if(rd==NULL) {
     return NULL;
   }

   return (char *)rd->getEgressIp().c_str();
}

char * SphereDB::getTerminationIp(hdr_field * callid) {
   RouteData * rd = getRouteDataForCall(callid);

   if(rd==NULL) {
     return NULL;
   }

   return (char *)rd->getTerminationIp().c_str();
}

char * SphereDB::getSuffix(hdr_field * callid) {
   RouteData * rd = getRouteDataForCall(callid);

   if(rd==NULL) {
     return NULL;
   }

   return (char *)rd->getSuffix().c_str();
}

char * SphereDB::getPrefix(hdr_field * callid) {
   RouteData * rd = getRouteDataForCall(callid);

   if(rd==NULL) {
     return NULL;
   }

   return (char *)rd->getPrefix().c_str();
}

char * SphereDB::getRoute(hdr_field * callid) {
   RouteData * rd = getRouteDataForCall(callid);

   if(rd==NULL) {
     return NULL;
   }

   return (char *)rd->getRouteString().c_str();
}

unsigned int SphereDB::getTerminationPort(hdr_field * callid) {
   RouteData * rd = getRouteDataForCall(callid);

   if(rd==NULL) {
     return -1;
   }

   return rd->getTerminationPort();
}

unsigned int SphereDB::getEgressPort(hdr_field * callid) {
   RouteData * rd = getRouteDataForCall(callid);

   if(rd==NULL) {
     return -1;
   }

   return rd->getEgressPort();
}

int SphereDB::releaseRoute(hdr_field * callid) {
  char callIdStr[256];
  strncpy(callIdStr, callid->body.s, callid->body.len);
  callIdStr[callid->body.len] = 0;
  delete mRoutes[callIdStr];
  mRoutes.erase(std::string(callIdStr));
  return 1;
}





extern "C" {
  int cpp_sphere_alloc_route(struct hdr_field * callid, char * ip, char * number) {
    return SphereDB::getInstance()->allocateRoute(callid, ip, number);
  }
  int cpp_sphere_release_route(struct hdr_field * callid) {
    return SphereDB::getInstance()->releaseRoute(callid);
  }

  unsigned int cpp_sphere_get_egress_port(struct hdr_field * callid) {
    return SphereDB::getInstance()->getEgressPort(callid);
  }

  char * cpp_sphere_get_egress_ip(struct hdr_field * callid) {
    return SphereDB::getInstance()->getEgressIp(callid);
  }

  unsigned int cpp_sphere_get_termination_port(struct hdr_field * callid) {
    return SphereDB::getInstance()->getEgressPort(callid);
  }

  char * cpp_sphere_get_termination_ip(struct hdr_field * callid) {
    return SphereDB::getInstance()->getEgressIp(callid);
  }

  char * cpp_sphere_get_suffix(struct hdr_field * callid) {
    return SphereDB::getInstance()->getSuffix(callid);
  }

  char * cpp_sphere_get_prefix(struct hdr_field * callid) {
    return SphereDB::getInstance()->getPrefix(callid);
  }

  char * cpp_sphere_get_route(struct hdr_field * callid) {
    return SphereDB::getInstance()->getRoute(callid);
  }


}
