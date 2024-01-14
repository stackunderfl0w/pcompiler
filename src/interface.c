#include "inc/ui.h"
#include <unistd.h>
#include "scope.h"
#include "ast.h"

uielement* ui_root;

void reload_button_onclick(uielement* e){

}


uielement* generate_main_ui(){
    uielement* v=ui_create_vertical();
    v->background_color=0x44444400;
    v->flags|=UI_SCROLL_DISABLE;
    uielement* title_bar=ui_create_horizontal();
    title_bar->flags|=UI_SCROLL_DISABLE;
    title_bar->h=100;
    ui_element_add_child(v, title_bar, -1);

    uielement* title= ui_create_text("Jitlang test", "SourceCodePro-Regular.ttf", 32);
    title->flags|= UI_TEXT_CENTER;
    title->pad=(padding){10, 10, 10, 10};
    title->background_color=0x66666600;
    ui_element_add_child(title_bar, title, -1);

    uielement* reload_button= ui_create_text("Recompile", "SourceCodePro-Regular.ttf", 32);
    reload_button->flags|= UI_TEXT_CENTER;
    reload_button->pad=(padding){10, 10, 10, 10};
    reload_button->background_color=0x66666600;
    reload_button->w=240;
    reload_button->onclick=reload_button_onclick;
    ui_element_add_child(title_bar, reload_button, -1);

    uielement* text_input= ui_create_text_input("SourceCodePro-Regular.ttf", 20);
    text_input->pad=(padding){10, 10, 10, 10};
    text_input->background_color=0x66666600;

    ui_element_add_child(v,text_input,-1);



    return v;
}

extern ast_node* target_program;
extern int parse_error;

scope * parse_symbols;


int main(){
    rendertarget* sdl_target= init_sdl2_ui(800,600);

    ui_root= generate_main_ui();

    bool running=true;
    bool need_update=true;

    while(running){

        need_update= ui_handle_input(sdl_target, &running);

        if(need_update){
            need_update=0;
            ui_render(sdl_target,ui_root);
            static int frames=0;
            //printf("frames:%d\n",++frames);
        }
        usleep(10000);
    }
    free_element_tree(ui_root);
    deinit_sdl2_ui(sdl_target);
    return 0;
}