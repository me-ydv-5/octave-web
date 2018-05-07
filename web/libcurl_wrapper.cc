/*

Copyright (C) 2017 Kai T. Ohlhus <k.ohlhus@gmail.com>

This file is part of Octave.

Octave is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Octave is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Octave; see the file COPYING.  If not, see
<http://www.gnu.org/licenses/>.

*/

//#if defined (HAVE_CURL)
#include <curl/curl.h>
#include <curl/easy.h>
//#endif

// struct for storing the response of curl_easy_perform.
// the struct is dynamic, i.e, the space grows automatically.
struct MemoryStruct {
  MemoryStruct (char* mem, size_t sz): memory(mem), size(sz) { }
  ~MemoryStruct () { }
  char *memory;
  size_t size;
};

// taken from https://curl.haxx.se/libcurl/c/getinmemory.html
// Callback function to write the output in a buffer, which 
// dynamically allcates the space.
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */ 
    error("Not enough memory (realloc returned NULL)\n");
  }
 
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}

//! Wrapper class for libcurl's easy interface, for the API specification see
//! https://curl.haxx.se/libcurl/c/libcurl-easy.html.

class libcurl_wrapper {
private:
  CURL* curl;  //! curl instance
  FILE* fid;
  struct MemoryStruct chunk;
  char errbuf[CURL_ERROR_SIZE]; //! curl error buffer

  //! Throw an Octave error and print message and code from libcurl

  void curl_error (CURLcode c) {
    if (c != CURLE_OK) {
      error ("libcurl (code = %d): %s\n", c, curl_easy_strerror (c));
    }
  }

protected:

  //! Ctor.

  libcurl_wrapper (std::string s) : curl(curl_easy_init ()), fid(fopen(s.c_str(),"w+")), \
            chunk((char*)malloc(1),0) { }

public:

  //! Static factory method

  static libcurl_wrapper create (std::string s) {
    libcurl_wrapper obj(s);
    // default error buffer is that of the object itself
    obj.setERRORBUFFER (obj.errbuf);
    obj.setVERBOSE (false);
    obj.setAGENT();
    obj.setTIMEOUT ();
    obj.setWRITEFUNCTION ();
    


    //Set the cookie jar 
    obj.setCOOKIEJAR(s);
    obj.setCOOKIEREAD(s);
    
    return obj;
  }

  //! Dtor.

  ~libcurl_wrapper() {
    if (curl) {
      curl_easy_cleanup (curl);
      free(chunk.memory);
      fclose(fid);
    }
  }

  //! Wrapper for curl_easy_perform
  void perform () {
    curl_error (curl_easy_perform (curl));
  }

  //! BEHAVIOR OPTIONS
  //! set verbose mode on (true) /off (false)
  void setVERBOSE (bool b) {
    curl_error (curl_easy_setopt (curl, CURLOPT_VERBOSE, (b ? 1L : 0L)));
  }

  //! ERROR OPTIONS
  //! set error buffer for error messages
  void setERRORBUFFER (char* buf) {
    curl_error (curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, buf));
  }

  //! NETWORK OPTIONS
  //! provide the URL to use in the request
  void setURL (std::string url) {
    curl_error (curl_easy_setopt (curl, CURLOPT_URL, url.c_str ()));
  }

  //! COOKIES OPTIONS
  //! sets the cookies to be written in the file specified
  void setCOOKIEJAR (std::string filename) {
    curl_error (curl_easy_setopt (curl, CURLOPT_COOKIEJAR, filename.c_str()));
  }

  //! COOKIES OPTIONS
  //! sets the cookies to be read from the file specified
  void setCOOKIEREAD (std::string filename) {
    curl_error (curl_easy_setopt (curl, CURLOPT_COOKIEFILE, filename.c_str()));
  }

  //! TIMEOUT OPTIONS
  //! sets the timeout of 20 seconds for a transfer, default is infinite
  void setTIMEOUT () {
    curl_error ( curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L));
  }

  //! CALLBACK OPTIONS
  //! We don't want the user to see all the details, but only the necessary information
  //! Redirect the output to the buffer in the memory instead of stdout
  void setWRITEFUNCTION () {
    curl_error ( curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback));
    /* we pass our 'chunk' struct to the callback function */ 
    curl_error ( curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk));
  }
  
  //! COOKIES OPTIONS
  //! get all the stored cookies
  void getCOOKIELIST () {
    struct curl_slist *cookies = NULL;
    curl_error (curl_easy_getinfo (curl, CURLINFO_COOKIELIST, &cookies));
    if(cookies) {
       // a linked list of cookies in cookie file format 
      struct curl_slist *each = cookies;
      while(each) {
        printf("%s\n\n", each->data);
        each = each->next;
    }
    curl_slist_free_all(cookies);
  }
  }

  //! USERAGENT OPTIONS
  //! sets the user agent for the software to be used when communicating with wiki
  void setAGENT () {

    //Set the user agent
    curl_version_info_data * data = curl_version_info(CURLVERSION_NOW);
    const char *lib_ver = data->version;
    std::string agent = "GNU Octave/" + std::string(OCTAVE_VERSION)         \
    + " (https://www.gnu.org/software/octave/ ; help@octave.org) libcurl/"
    + std::string(lib_ver); 
    curl_error (curl_easy_setopt (curl, CURLOPT_USERAGENT, agent.c_str()));
  }

  //! GETINFO
  //! get the last used URL
  std::string getEFFECTIVE_URL () {
    char* urlp;
    curl_error (curl_easy_getinfo (curl, CURLINFO_EFFECTIVE_URL, &urlp));
    if (urlp) {
      return std::string (urlp);
    }
    return "";
  }

  //! GETTOKEN
  //! get the login token, it acutally sends out the complete
  //! output received, which will then be fed to regexp
  char* getTOKEN () {
    if (chunk.memory)
    {
      return chunk.memory;
    }
    error("__curl__: Nothing to output\n");
  }

};
