#include <octave/oct.h>
#include <cstdio>
#include "libcurl_wrapper.cc"

DEFUN_DLD (__curl__, args, , "Some __curl__ demo.")
{
  if (args.length () != 1)
    print_usage ();

  auto a = libcurl_wrapper::create();

  std::string wikiapi = "http://wiki.octave.org/wiki/api.php";
  std::string url =  wikiapi +  "?action=query&meta=tokens&type=login&format=json";
  a.setURL (url);
  a.setWRITEFUNCTION ();
  a.perform ();

  return octave_value (a.getTOKEN ());
}