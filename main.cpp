#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <expat.h>
#include <curl/curl.h>

using namespace std;

struct MemoryStruct {
  char *memory;
  size_t size;
};

char *replace_str(char *str, char *orig, char *rep)
{
  static char buffer[4096];
  char *p;

  if(!(p = strstr(str, orig)))  // Is 'orig' even in 'str'?
    return str;

  strncpy(buffer, str, p-str); // Copy characters from 'str' start to 'orig' st$
  buffer[p-str] = '\0';

  sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));

  return buffer;
}


static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = (char*) realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    // out of memory! 
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

///////////////////////////////////////////////////

/* track the current level in the xml tree */
static int      depth = 0;

static char    *last_content;

/* first when start element is encountered */
void
start_element(void *data, const char *element, const char **attribute)
{
    depth++;
}

/* decrement the current level of the tree */
void
end_element(void *data, const char *el)
{
    if(depth == 6)
    {
    	printf("%s: \"%s\"\n", el, last_content);
    }
    depth--;
}

void
handle_data(void *data, const char *content, int length)
{
    char           *tmp = (char*) malloc(length);
    strncpy(tmp, content, length);
    tmp[length] = '\0';
    data = (void *) tmp;
    last_content = tmp;
}

int
parse_xml(char *buff, size_t buff_size)
{
    XML_Parser      parser = XML_ParserCreate(NULL);
    XML_SetElementHandler(parser, start_element, end_element);
    XML_SetCharacterDataHandler(parser, handle_data);

    /* parse the xml */
    if (XML_Parse(parser, buff, strlen(buff), XML_TRUE) == XML_STATUS_ERROR) {
        printf("Error: %s\n", XML_ErrorString(XML_GetErrorCode(parser)));
    }

    XML_ParserFree(parser);

    return 0;
}
 
int main(void)
{
  CURL *curl;
  CURLcode res;

  struct MemoryStruct mem;
  mem.memory = (char*) malloc(1);  //will be grown as needed by the realloc above
  mem.size = 0;    // no data at this point

  char post[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?><soap12:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap12=\"http://www.w3.org/2003/05/soap-envelope\"><soap12:Body><GetWeather xmlns=\"http://www.webserviceX.NET\"><CityName>Eindhoven</CityName><CountryName>Netherlands</CountryName></GetWeather></soap12:Body></soap12:Envelope>";

  // In windows, this will init the winsock stuff
  curl_global_init(CURL_GLOBAL_ALL);
 
  // get a curl handle
  curl = curl_easy_init();
  if(curl) {

	struct curl_slist *chunk = NULL;
  	chunk = curl_slist_append(chunk, "Content-type: application/soap+xml; charset=utf-8");
  	chunk = curl_slist_append(chunk, "Accept: application/soap+xml");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    curl_easy_setopt(curl, CURLOPT_URL, "http://www.webservicex.com/globalweather.asmx");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem);
 
    // Perform the request, res will get the return code
    res = curl_easy_perform(curl);
    // Check for errors 
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
 
    // always cleanup 
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();

  // Process stuffjes

  char replace_lt[] = "&lt;";
  char replace_gt[] = "&gt;";
  char replace_utf_spam[] = "&lt;?xml version=\"1.0\" encoding=\"utf-16\"?&gt;";
  char empty[] = "";
  char lt[] = "<";
  char gt[] = ">";

  char * replaced_str = mem.memory;

  replaced_str = replace_str(replaced_str, replace_utf_spam, empty);
  for(int i=0; i < 42; i++) {
	  replaced_str = replace_str(replaced_str, replace_lt, lt);
	  replaced_str = replace_str(replaced_str, replace_gt, gt);
  }

  parse_xml(replaced_str, 100000);

  return 0;
}