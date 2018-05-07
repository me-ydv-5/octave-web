function [handle,cookie_jar] = cookie_manager (url)
  if (nargin < 1)
    print_usage ();
  endif

  if ((ischar (url) && isvector (url)) == 0)
    error("cookie_manager: url must be a string");
  else 
    cookie_jar = [tempname, ".txt"];
    handle = __curl__(url,cookie_jar);
  endif
endfunction