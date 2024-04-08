#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include "ui.h"
#include "manga.h"

uielement* ui_root;
dynarray* mangas;

void reload_onclick(uielement*e);
void series_onclick(uielement*e);
uielement* generate_title_page(dynarray* manga_list);
uielement* generate_series_page(manga* m);

void chapter_onclick(uielement*e){
    chapter* c=e->p_data;
    c->read=true;
    e->background_color=0x33333300;
    char command[512]={0};
    sprintf(command,"xdg-open %s",c->address);
    system(command);
}
void chapter_onrightclick(uielement*e){
    chapter* c=e->p_data;
    c->read=!c->read;
    e->background_color=c->read?0x33333300:0x66666600;
}
void back_onclick(uielement* e){
    (void)e;
    free_element_tree(ui_root);
    ui_root= generate_title_page(mangas);
}
void series_onclick(uielement* e){
    (void)e;
    manga* m=e->p_data;
    free_element_tree(ui_root);
    ui_root= generate_series_page(m);
}
uielement* to_update=NULL;

void reload_onclick(uielement* e){
    to_update=e;
//    update_series(e->p_data);
//    manga* m=e->p_data;
//    free_element_tree(ui_root);
//    ui_root= generate_series_page(m);
}
//sigh, do this in a different function so gdb doesn't freeze the entire system
void reload_onclick_2(){
    update_series(to_update->p_data);
    manga* m=to_update->p_data;
    free_element_tree(ui_root);
    ui_root= generate_series_page(m);

    to_update=NULL;
}

void add_series_dialog_cancel(uielement* e){
    (void)e;
    ui_element_free_child(ui_root,2);
    ui_element_free_child(ui_root,1);
}
char* add_series_name=NULL;
char* add_series_url=NULL;

void add_series_dialog_accept(uielement* e){
    (void)e;
    manga* tmp= calloc(sizeof(manga),1);
    tmp->chapters=dynarray_init();
    tmp->title= strndup(add_series_name,256);
    tmp->address=strndup(add_series_url,256);
    dynarray_insert(mangas,tmp,0);
    update_series(tmp);

    ui_root= generate_title_page(mangas);
    //printf("name:%s\nurl:%s\n",add_series_name,add_series_url);
    //add_series_dialog_cancel(e);
}

void add_button_onclick(uielement* e){
    (void)e;
    uielement* background=ui_create_spacer();
    background->onclick=add_series_dialog_cancel;
    ui_element_add_child(ui_root,background,-1);

    uielement* v=ui_create_vertical();
    v->pad=(padding){100,100,100,100};
    v->background_color=0x44444400;
    v->flags|=UI_SCROLL_DISABLE;
    ui_element_add_child(ui_root,v,-1);

    char* columns[]={"Title:","Url:"};
    for(int i = 0; i < 2; ++i){
        uielement* h=ui_create_horizontal();
        h->h=100;
        h->pad=(padding){10,10,50,50};
        h->flags|=UI_SCROLL_DISABLE;
        ui_element_add_child(v,h,-1);

        uielement* left_text=ui_create_text(columns[i],"SourceCodePro-Regular.ttf", 32);
        left_text->flags|=UI_TEXT_CENTER_VERTICAL;
        left_text->w=150;

        uielement* right_entry= ui_create_text_input("SourceCodePro-Regular.ttf", 32);
        right_entry->background_color=0x55555500;
        right_entry->flags|=UI_TEXT_CENTER_VERTICAL;

        ui_element_add_child(h,left_text,-1);
        ui_element_add_child(h,right_entry,-1);
        if(i==0)
            add_series_name=right_entry->a_data;
        else
            add_series_url=right_entry->a_data;
    }
    uielement* center_space=ui_create_spacer();
    ui_element_add_child(v,center_space,-1);

    uielement* h=ui_create_horizontal();
    h->h=100;
    h->pad=(padding){10,10,50,50};
    h->flags|=UI_SCROLL_DISABLE;
    ui_element_add_child(v,h,-1);

    uielement* cancel= ui_create_text("Cancel","SourceCodePro-Regular.ttf", 32);
    cancel->background_color=0x55555500;
    cancel->pad=(padding){10,10,10,10};
    cancel->flags|=UI_TEXT_CENTER;
    cancel->onclick=add_series_dialog_cancel;
    ui_element_add_child(h,cancel,-1);

    uielement* add= ui_create_text("Add","SourceCodePro-Regular.ttf", 32);
    add->background_color=0x55555500;
    add->pad=(padding){10,10,10,10};
    add->flags|=UI_TEXT_CENTER;
    add->onclick=add_series_dialog_accept;
    ui_element_add_child(h,add,-1);

}

uielement* generate_title_page(dynarray* manga_list){
    uielement* root_stack=ui_create_stack();
    uielement* v=ui_create_vertical();
    v->background_color=0x44444400;
    v->flags|=UI_SCROLL_DISABLE;
    ui_element_add_child(root_stack, v, -1);

    uielement* title_bar=ui_create_horizontal();
    title_bar->flags|=UI_SCROLL_DISABLE;
    title_bar->h=100;
    ui_element_add_child(v, title_bar, -1);

    uielement* left_space=ui_create_spacer();
    left_space->w=120;
    ui_element_add_child(title_bar, left_space, -1);

    uielement* title= ui_create_text("ComixWatch", "SourceCodePro-Regular.ttf", 32);
    title->flags|= UI_TEXT_CENTER;
    title->pad=(padding){10, 10, 10, 10};
    title->background_color=0x66666600;
    ui_element_add_child(title_bar, title, -1);

    uielement* add_button= ui_create_text("+", "SourceCodePro-Regular.ttf", 60);
    add_button->flags|= UI_TEXT_CENTER;
    add_button->pad=(padding){10, 10, 10, 10};
    add_button->background_color=0x66666600;
    add_button->w=120;
    add_button->onclick=add_button_onclick;
    ui_element_add_child(title_bar, add_button, -1);


    uielement* series_list=ui_create_vertical();
    ui_element_add_child(v,series_list,-1);

    for(int i = 0; i < manga_list->size; ++i){
        manga* m=manga_list->data[i];
        uielement* t= ui_create_text(m->title,"SourceCodePro-Regular.ttf", 18);
        t->h=80;
        t->onclick=series_onclick;
        t->p_data=m;
        t->pad=(padding){10,10,10,10};
        t->background_color=0x55555500;
        t->flags|=UI_TEXT_CENTER;
        t->radius=10;
        ui_element_add_child(series_list,t,-1);
    }
    return root_stack;
}
uielement* generate_series_page(manga* m){
    //printf("Series:%s clicked\n",m->title);
    uielement* v = ui_create_vertical();
    v->background_color=0x44444400;
    uielement* h=ui_create_horizontal();
    h->h=100;
    ui_element_add_child(v,h,-1);

    uielement *back= ui_create_text("Back","SourceCodePro-Regular.ttf", 24);
    back->onclick=back_onclick;
    back->p_data=m;
    back->w=120;
    back->pad=(padding){10,10,10,10};
    back->flags|=UI_TEXT_CENTER;
    back->background_color=0x66666600;
    ui_element_add_child(h,back,-1);


    uielement* title_text= ui_create_text(m->title,"SourceCodePro-Regular.ttf", 28);
    title_text->background_color=0x66666600;
    title_text->flags|=UI_TEXT_CENTER;
    title_text->pad=(padding){10,10,10,10};

    ui_element_add_child(h,title_text,-1);

    uielement *reload= ui_create_text("Reload","SourceCodePro-Regular.ttf", 24);
    reload->onclick=reload_onclick;
    reload->p_data=m;
    reload->w=120;
    reload->pad=(padding){10, 10, 10, 10};
    reload->flags|=UI_TEXT_CENTER;
    reload->background_color=0x66666600;
    ui_element_add_child(h, reload, -1);


    uielement* v2=ui_create_vertical();
    ui_element_add_child(v,v2,-1);

    for(int i = 0; i < m->chapters->size; ++i){
        chapter* c=m->chapters->data[i];
        uielement* t= ui_create_text(c->name,"SourceCodePro-Regular.ttf", 18);
        t->background_color=c->read?0x33333300:0x66666600;
        t->pad=(padding){10,10,10,10};
        t->flags|=UI_TEXT_CENTER;
        t->h=80;
        t->onclick=chapter_onclick;
        t->onrightclick=chapter_onrightclick;
        t->p_data=c;
        t->radius=10;
        ui_element_add_child(v2,t,-1);
    }
    return v;
}

int main(void){
    mangas=populatemangas_from_json("sav.json");
    rendertarget* sdl_target= init_sdl2_ui(800,600);

    ui_root= generate_title_page(mangas);

    bool running=true;
    bool need_update=true;

    while(running){

        need_update= ui_handle_input(sdl_target, &running);
        //wait so that gdb doesn't block all mouse input
        //static int wait=0;
        if(to_update){
            //if(wait++>100){
            //    wait=0;
                reload_onclick_2();
            //}
        }

        if(need_update){
            need_update=0;
            ui_render(sdl_target,ui_root);
            //static int frames=0;
            //printf("frames:%d\n",++frames);
        }
        usleep(10000);
    }
    free_element_tree(ui_root);
    deinit_sdl2_ui(sdl_target);
    mangas_flush_to_file(mangas, "sav.json");
    free_mangas(mangas);

    return 0;
}
