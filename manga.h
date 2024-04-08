#pragma once
#include <fcntl.h>
#include <sys/stat.h>
#include "json.h"
#include "dynarray.h"
#include "curl_wrapper.h"
typedef struct{
    char* name;
    char* address;
    bool read;
}chapter;
typedef struct {
    char* title;
    char* address;
    char* root_address;
    bool loaded;
    dynarray* chapters;
}manga;
char* escape_quotes(char *str) {
    size_t i, j;
    size_t len = strlen(str);
    char *buffer = (char *) malloc(len * 2 + 1); // allocate twice the size of original string + 1 for null terminator
    if (buffer == NULL) { // check if allocation failed
        return NULL;
    }
    for (i = 0, j = 0; i < len; i++) {
        if (str[i] == '"') {
            buffer[j++] = '\\';
            buffer[j++] = '"';
        } else {
            buffer[j++] = str[i];
        }
    }
    buffer[j] = '\0'; // add null terminator to the end of the buffer
    return buffer;
}
char* strconcat(char* s1, char* s2){
    char* r=malloc(strnlen(s1,256)+strnlen(s2,256)+1);
    memcpy(r,s1,strnlen(s1,256));
    strncpy(r+strnlen(s1,256),s2,strnlen(s2,256));
    r[strnlen(s1,256)+strnlen(s2,256)]=0;
    return r;
}
struct json_object_element_s* json_get_child_with_name(struct json_object_element_s* start,char* name){
    do {
        if(strncmp(start->name->string,name,256)==0)
            return start;
    }while((start=start->next));
    return NULL;
}
void mangas_flush_to_file(dynarray* mangas,char* outfile){
    //flush to json file, easier than finding a new library
    FILE* f= fopen(outfile,"w");
    fprintf(f,"{\n");
    for (int i = 0; i < mangas->size; ++i){
        manga* m=mangas->data[i];
        char* s= escape_quotes(m->title);
        fprintf(f,"\t\"%s\": {\n",s);
        fprintf(f,"\t\t\"Address\": \"%s\",\n",m->address);
        fprintf(f,"\t\t\"Chapters\": [\n");
        for (int j = 0; j < m->chapters->size; ++j) {
            chapter* c=m->chapters->data[j];
            fprintf(f,"\t\t\t{\n");
            fprintf(f,"\t\t\t\t\"Name\": \"%s\",\n",c->name);
            fprintf(f,"\t\t\t\t\"Read\": %s\n",c->read?"true":"false");
            fprintf(f,"\t\t\t},\n");
        }
        fseek(f,-2,SEEK_CUR);
        fprintf(f,"\n");

        fprintf(f,"\t\t],\n");

        fprintf(f,"\t\t\"Root address\": \"%s\",\n",m->root_address);
        fprintf(f,"\t\t\"Title\": \"%s\"\n",s);
        fprintf(f,"\t},\n");
        free(s);
    }
    fseek(f,-2,SEEK_CUR);
    fprintf(f,"\n}");
}

void free_mangas(dynarray* mangas){
    for (int i = 0; i < mangas->size; ++i) {
        manga* m=mangas->data[i];
        for (int j = 0; j < m->chapters->size; ++j) {
            chapter* c= m->chapters->data[j];
            free(c->name);
            free(c->address);
            free(c);
        }
        free(m->title);
        free(m->root_address);
        free(m->address);
        dynarray_free(m->chapters);
        free(m);
    }
    dynarray_free(mangas);
}
dynarray* populatemangas_from_json(char* infile){
    dynarray* m=dynarray_init();
    char* json_string;
    {
        int f=open(infile,O_RDONLY);
        struct stat s;
        fstat(f,&s);
        json_string= malloc(s.st_size+1);
        json_string[s.st_size]=0;
        read(f,json_string,s.st_size);
        close(f);
    }

    struct json_value_s* jroot= json_parse(json_string, strlen(json_string));
    struct json_object_s* object = json_value_as_object(jroot);

    for (struct json_object_element_s* s=object->start; s; s=s->next) {
        struct json_object_s* series=s->value->payload;
        manga* tmp= calloc(sizeof(manga),1);
        tmp->chapters=dynarray_init();

        struct json_object_element_s* current=json_get_child_with_name(series->start,"Title");
        tmp->title=strndup(json_value_as_string(current->value)->string,256);

        current=json_get_child_with_name(series->start,"Address");
        tmp->address=strndup(json_value_as_string(current->value)->string,256);


        current=json_get_child_with_name(series->start,"Root address");
        tmp->root_address=strndup(json_value_as_string(current->value)->string, 256);

        current=json_get_child_with_name(series->start,"Chapters");
        struct json_array_s* chap=json_value_as_array(current->value);
        for (struct json_array_element_s* c=chap->start; c; c=c->next) {
            struct json_object_element_s* chap_object=json_value_as_object(c->value)->start;
            chapter* final_chapter= malloc(sizeof(chapter));
            final_chapter->name=strndup(json_value_as_string(chap_object->value)->string,256);
            final_chapter->address= strconcat(tmp->root_address,final_chapter->name);
            final_chapter->read=json_value_is_true(chap_object->next->value);
            dynarray_insert(tmp->chapters,final_chapter,-1);
        }
        dynarray_insert(m,tmp,-1);
    }
    free(json_string);
    free(jroot);
    return m;
}
float chapter_num_from_str(char* str){
    float last_num = 0.0f;
    for(long i=0;str[i];i++){
        if (isdigit(str[i]) || str[i] == '.') {
            char* end;
            float num = strtof(&str[i], &end);
            if (end > &str[i]) {
                last_num = num;
            }
            i = end - str - 1;
        }
    }
    return last_num;
}
void update_series(manga* m){
    char* redir=NULL;
    dynarray* links=get_chapter_list(m->address,&redir);
    if(redir){
        free(m->address);
        m->address=redir;
    }
    if(!links)
        return;
    //look for the last chapter that doesn't have asset in the name
    char* root=NULL;
    for(int i = links->size - 1; i >= 0; i--){
        if(strstr(links->data[i], "chapter") && !strstr(links->data[i], "assets")){
            char* end=strrstr(links->data[i], "chapter");
            root= calloc(end - (char*)links->data[i] + 1, 1);
            if(m->root_address)
                free(m->root_address);
            m->root_address=root;
            memcpy(root, links->data[i], end - (char*)links->data[i]);
            break;
        }
    }
    if(!root){
        printf("No chapters found");
    }
    for (int i = 0; i < links->size; ++i){
        char* str=links->data[i];
        if(str[strlen(str)-1]=='/'){
            str[strlen(str)-1]=0;
        }
        char* substr=strrstr(str, "chapter");
        //filter out bad links and other series
        if(str[strlen(str)-1]=='-'||strlen(substr)>40||strstr(str,root)==NULL){
            free(str);
            dynarray_remove_index(links, i);
            i--;
        }
    }
    if (m->chapters->size==0){
        for (int i = 0; i < links->size; ++i) {
            chapter* tmp= calloc(sizeof (chapter),1);
            tmp->name=strdup(strrstr(links->data[i],"chapter"));
            tmp->address=strdup(links->data[i]);
            dynarray_insert(m->chapters,tmp,-1);
        }
    }
    else{
        int index=0;
        for (int i = 0; i < links->size; ++i){
            char* str=links->data[i];
            char* name=strrstr(str,"chapter");
            bool found=false;
            for (int j = 0; j < m->chapters->size; ++j) {
                if(!strcmp(((chapter*)m->chapters->data[j])->name, name)){
                    found=true;
                    break;
                }
            }
            if (!found){
                chapter* temp = calloc(sizeof (chapter),1);
                temp->name=strdup(strrstr(str,"chapter"));
                temp->address=strdup(str);
                if (chapter_num_from_str(((chapter*)m->chapters->data[0])->name) > 1){
                    dynarray_insert(m->chapters,temp,index);
                    printf("insert at %d\n",index);
                }
                else{
                    dynarray_insert(m->chapters,temp,-1);
                    printf("append at %d\n",m->chapters->size);
                }
                index++;
            }
        }
    }
    for (int i = 0; i < links->size; ++i) {
        free(links->data[i]);
    }
    dynarray_free(links);
}
