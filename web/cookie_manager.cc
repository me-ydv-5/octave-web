#include <octave/oct.h>
#include <cstdio>
#include "libcurl_wrapper.cc"

DEFUN_DLD (cookie_manager, args, , "Some __cookie manager__ demo.")
{
  if (args.length () != 1)
    print_usage ();

  auto a = libcurl_wrapper::create();

  // octave_value_list retval = mkstemp(s,"delete");
  FILE* fid = fopen("temp.txt","w+");
  // std::string oct_ver = OCTAVE_VERSION;

  curl_version_info_data * data = curl_version_info(CURLVERSION_NOW);
  const char *lib_ver = data->version;

  a.setJAR("temp.txt");
  a.setAGENT(lib_ver);
  std::string wikiapi = "http://wiki.octave.org/wiki/api.php";
  std::string url =  wikiapi +  "?action=query&meta=tokens&type=login&format=json";
  a.setURL (url);
  
  a.perform ();
  fclose(fid);
  return octave_value (a.getEFFECTIVE_URL ());
}
