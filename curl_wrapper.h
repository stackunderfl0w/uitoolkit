#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "dynarray.h"

typedef struct{
    size_t size;
    char* data;
}curlstorage;

static char *strrstr(const char *haystack, const char *needle){
    if (*needle == '\0')
        return (char *) haystack;

    char *result = NULL;
    for (;;) {
        char *p = strstr(haystack, needle);
        if (p == NULL)
            break;
        result = p;
        haystack = p + 1;
    }
    return result;
}

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {

    curlstorage* dest=(curlstorage*)userp;

    dest->data = realloc(dest->data,dest->size + size * nmemb + 1);
    if(dest == NULL) {
        printf("error: not enough memory\n");
        return 0;
    }
    memcpy(dest->data+dest->size, contents, size*nmemb);
    dest->size+=size*nmemb;
    return size*nmemb;
}

regex_t regex={0};
int ret=-1;

dynarray* get_chapter_list(const char* address, char** redirection){
    CURL *curl;
    CURLcode res;
    curlstorage readBuffer={0};

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, address);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        // Enable automatic redirect following
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        // Set maximum number of redirects
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
        res = curl_easy_perform(curl);
        printf("curl res code:%d\n",res);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        else {
            // Get the final URL
            char *url = NULL;
            curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
            if(url){
                if(strcmp(address,url)){
                    printf("Redirect to: %s\n", url);
                    *redirection=strdup(url);
                }
            }
        }

        curl_easy_cleanup(curl);
    }
    if(ret!=0){
        ret= regcomp(&regex,
                     "https?:\\/\\/(www\\.)?"\
                      "[-a-zA-Z0-9@:%._\\+~#=]{1,256}"\
                      "\\.[a-zA-Z0-9()]{1,6}\\b([-a-z"\
                      "A-Z0-9()@:%_\\+.~#?&//=]*)",REG_EXTENDED);
        //ret= regcomp(&regex,"https:\\/\\/[\\w\\.\\/-]+chapter-\\d+(\\.\\d+)?$",REG_EXTENDED);

        if (ret != 0) {
            printf("Error compiling regex\n");
            return NULL;
        }
    }


    // link counter
    dynarray* links=dynarray_init();

    char* curr=readBuffer.data;
    if(!curr){
        printf("No data returned from \"%s\"",address);
        return NULL;
    }
    regmatch_t match={0};
    int start=0;
    while(regexec(&regex, curr+start, 1, &match, 0)==0){
        char buf[256]={0};
        //make sure I don't corrupt the damn stack again
        memcpy(buf,curr+start+match.rm_so,MIN(match.rm_eo - match.rm_so,255));
        if(strstr(buf,"chapter")){
            //printf("Match found at position %d: %s\n", start + match.rm_so, buf);
            dynarray_insert(links,strndup(buf,255),-1);
        }
        start += match.rm_eo;
    }
    /*for (int i = 0; i < links->size; ++i) {
        printf("%s\n",(char*)links->data[i]);
    }*/
    free(readBuffer.data);
    return links;
}