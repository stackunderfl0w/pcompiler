#pragma once
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "dynarray.h"
#include "hashmap.h"

//todo, combine vertical stack and horizontal to use same render code
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
typedef struct uielement uielement;

typedef struct{
    int type, w, h;
    void* window;
    void* renderer;
    dynarray* render_order;
    uielement* text_in_focus;
}rendertarget;

typedef struct{
    unsigned short n,s,e,w;
}padding;

struct uielement{
    int type;
    unsigned int background_color;
    int radius;
    int (*render)(rendertarget* target,uielement* root, int x, int y, int w, int h);
    void (*onclick)(uielement* e);
    void (*onrightclick)(uielement* e);
    void (*onscroll)(uielement* e, int x, int y);
    void (*ontextinput)(uielement* e, char* s);

    size_t flags;
    dynarray* children;
    size_t data;
    void* a_data;
    void* a_data_2;
    void* p_data;
    int w,h;
    padding pad;
    SDL_Rect last_bb;
};
SDL_Rect bounds_overlap(SDL_Rect f, SDL_Rect s){
    int x_overlap = MAX(0, MIN(f.x+f.w, s.x+s.w) - MAX(f.x, s.x));
    int y_overlap = MAX(0, MIN(f.y+f.h, s.y+s.h) - MAX(f.y, s.y));
    SDL_Rect result;
    result.x = MAX(f.x, s.x);
    result.y = MAX(f.y, s.y);
    result.w = x_overlap;
    result.h = y_overlap;
    return result;
}
#define int_to_color(c)(c>>24)&0xff, (c>>16)&0xff, (c>>8)&0xff, c&0xff

#define UI_TYPE_SOLID_COLOR 1
#define UI_TYPE_TEXT 2
#define UI_TYPE_VERTICAL 3
#define UI_TYPE_HORIZONTAL 4
#define UI_TYPE_STACK 5
#define UI_TYPE_TEXT_INPUT 6
#define UI_TYPE_SPACER 7

#define UI_HAS_CHILDREN 1<<0
#define UI_MEM_ALLOCED 1<<1
#define UI_MEM_ALLOCED_2 1<<2
#define UI_TEXT_CENTER_HORIZONTAL 1<<3
#define UI_TEXT_CENTER_VERTICAL 1<<4
#define UI_TEXT_CENTER (UI_TEXT_CENTER_HORIZONTAL|UI_TEXT_CENTER_VERTICAL)
#define UI_SCROLL_DISABLE 1<<5

bool debug=false;

hashmap* font_cache;

void RenderFillRoundedRect(rendertarget* target, SDL_Rect *rect, int rad){
    SDL_Rect R={rect->x+rad,rect->y,rect->w-rad-rad,rect->h};
    SDL_RenderFillRect(target->renderer,&R);
    //based on Jesko's Method of the Midpoint circle algorithm
    int t1 = rad / 16;
    int x = rad;
    int y = 0;

    while (x >= y){
        // Draw pixels in 8 symmetric positions
        int rx= rect->x + rad,ry= rect->y + rad;
        //left side
        SDL_RenderDrawLine(target->renderer, rx - y, ry - x, rx - y, ry + x + rect->h - 2*rad - 1);
        SDL_RenderDrawLine(target->renderer, rx - x, ry - y, rx - x, ry + y + rect->h - 2*rad - 1);

        rx= rect->x +rect->w- rad;
        //right side
        SDL_RenderDrawLine(target->renderer, rx + y, ry - x, rx + y, ry + x + rect->h - 2*rad - 1);
        SDL_RenderDrawLine(target->renderer, rx + x, ry - y, rx + x, ry + y + rect->h - 2*rad - 1);

        y = y + 1;
        t1 = t1 + y;
        int t2 = t1 - x;

        if (t2 >= 0) {
            t1 = t2;
            x = x - 1;
        }
    }
}

//middleman to apply common checks
int ui_dispatch_render(rendertarget* target, uielement* e, int x, int y, int w, int h, SDL_Rect* bb){
    SDL_Rect req= (SDL_Rect){x+e->pad.w,y+e->pad.n,w-e->pad.w-e->pad.e,h-e->pad.n-e->pad.s};

    e->last_bb=req;
    //check render bounds
    if(bb){
        //are we completely within the bounds?
        if(bb->x>req.x||bb->y>req.y||bb->x+bb->w<req.x+req.w||bb->y+bb->h<req.y+req.h){
            *bb= bounds_overlap(*bb,req);
            if(bb->h==0||bb->w==0)
                return 0;
        }else
            bb=NULL;
    }
    //save render order for mouse events
    dynarray_insert(target->render_order,e,-1);
    if(e->render==NULL)
        return 0;


    //handle clipping by setting viewport and offsetting rendering location
    int rx=x,ry=y;
    if(bb){
        SDL_RenderSetViewport(target->renderer,bb);
        rx-=bb->x;ry-=bb->y;
        req.x-=bb->x;req.y-=bb->y;
    }
    //render background
    if(e->background_color){
        SDL_SetRenderDrawColor(target->renderer,int_to_color(e->background_color));
        if(!e->radius)
            SDL_RenderFillRect(target->renderer,&req);
        else
            RenderFillRoundedRect(target,&req,e->radius);
    }
    e->render(target,e,rx+e->pad.w,ry+e->pad.n,w-e->pad.w-e->pad.e,h-e->pad.n-e->pad.s);
    SDL_RenderSetViewport(target->renderer,NULL);

    if(debug){
        SDL_SetRenderDrawColor(target->renderer, 0xff, 0, 0, 0);
        SDL_Rect rect={x,y,w,h};
        SDL_RenderDrawRect(target->renderer,&rect);
        if(e->pad.n|e->pad.s|e->pad.e|e->pad.w){
            SDL_SetRenderDrawColor(target->renderer, 0xff, 0, 0xff, 0);
            rect=(SDL_Rect){x+e->pad.w,y+e->pad.n,w-e->pad.w-e->pad.e,h-e->pad.n-e->pad.s};
            SDL_RenderDrawRect(target->renderer,&rect);
        }
        if(bb){
            SDL_SetRenderDrawColor(target->renderer, 0, 0xff, 0xff, 0);
            SDL_RenderDrawRect(target->renderer,bb);
        }
    }
}
int ui_render(rendertarget* target, uielement* root){
    dynarray_clear(target->render_order);
    SDL_SetRenderDrawColor(target->renderer, 0, 0, 0, 0);
    SDL_SetRenderDrawBlendMode(target->renderer, SDL_BLENDMODE_NONE);

    SDL_RenderClear(target->renderer);
    int res=ui_dispatch_render(target,root,0,0,target->w,target->h,NULL);
    SDL_RenderPresent(target->renderer);
    return res;
}

void ui_vertical_on_scroll(uielement* e, int x, int y){
    ssize_t* s=(ssize_t*)&e->data;
    if(e->type==UI_TYPE_VERTICAL)
        *s=MAX(0,*s-y*15);
    else
        *s=MAX(0,*s+x*15);
}

void ui_text_input_ontext(uielement* e, char* s){
    if(s[0]!='\b')
        strncat(e->a_data,s,1023);
    else if(strlen(e->a_data)>0)
        ((char*)e->a_data)[strlen(e->a_data)-1] = '\0';
}

void ui_element_add_child(uielement* v, uielement* e, int index){
    dynarray_insert(v->children,e,index);
}

uielement* ui_element_pop_child(uielement* v, int index){
    uielement* e=v->children->data[index];
    dynarray_remove_index(v->children,index);
    return e;
}
void free_element_tree(uielement* e);
uielement* ui_element_free_child(uielement* v, int index){
    free_element_tree(ui_element_pop_child(v,index));
}

SDL_Texture* ui_cache_font(rendertarget* target, char* fontname, int size){
    TTF_Font* font = TTF_OpenFont("SourceCodePro-Regular.ttf", size);
    char* s= calloc(129,1);
    for (int i = 0; i < 128; ++i)
        s[i]= isprint(i)?i:' ';
    SDL_Surface* g= TTF_RenderText_Blended(font,s,(SDL_Color){0xff,0xff,0xff,0});
    TTF_CloseFont(font);
    free(s);
    SDL_Texture* text_atlas= SDL_CreateTextureFromSurface(target->renderer,g);
    SDL_FreeSurface(g);
    hashmap_insert(font_cache, fontname, size, text_atlas);
    return text_atlas;
}

int ui_render_color(rendertarget* target, uielement* e, int x, int y, int w, int h){
    SDL_SetRenderDrawColor(target->renderer,int_to_color(e->data));
    SDL_Rect rect={x,y,w,h};
    SDL_RenderFillRect(target->renderer,&rect);
}
//todo handle undersized lists of elements
int ui_render_vertical(rendertarget* target, uielement* e, int x, int y, int w, int h) {
    int requested_height = 0, indif = e->children->size;
    for (int i = 0; i < e->children->size; ++i) {
        if(((uielement *) (e->children->data[i]))->h){
            requested_height += ((uielement *) (e->children->data[i]))->h;
            indif--;
        }
    }
    int available=MAX(h-requested_height,0);
    int current_y=y-(int)*(ssize_t*)&e->data;
    for (int i = 0; i < e->children->size&&current_y<y+h; ++i){
        uielement* c = e->children->data[i];
        if(current_y+c->h>=y){
            SDL_Rect bb={x,y,w,h};
            ui_dispatch_render(target, c, x,current_y, c->w?c->w:w, c->h ? c->h : available / indif, &bb);
        }
        current_y+=c->h?c->h:available/indif;
    }
}
int ui_render_horizontal(rendertarget* target, uielement* e, int x, int y, int w, int h) {
    int requested_width = 0, indif = e->children->size;
    for (int i = 0; i < e->children->size; ++i) {
        if(((uielement *) (e->children->data[i]))->w){
            requested_width += ((uielement *) (e->children->data[i]))->w;
            indif--;
        }
    }
    int available=MAX(w-requested_width,0);
    int current_x=x-(int)*(ssize_t*)&e->data;
    for (int i = 0; i < e->children->size&&current_x<x+w; ++i){
        uielement* c = e->children->data[i];
        if(current_x+c->w>=x){
            SDL_Rect bb={x,y,w,h};
            ui_dispatch_render(target, c, current_x,y, c->w?c->w:available / indif, c->h ? c->h : h, &bb);
        }
        current_x+=c->w?c->w:available/indif;
    }
}
int ui_render_stack(rendertarget* target, uielement* e, int x, int y, int w, int h){
    for (int i = 0; i < e->children->size; ++i) {
        uielement* c = e->children->data[i];
        ui_dispatch_render(target,c,x,y,w,h,NULL);
    }
}
int ui_render_text(rendertarget* target, uielement* e, int x, int y, int w, int h){
    SDL_Texture* text_atlas;
    if(!(text_atlas= hashmap_get(font_cache, e->a_data_2, (int)e->data)))
        text_atlas=ui_cache_font(target,e->a_data_2,(int)e->data);

    int char_w,char_h;
    SDL_QueryTexture(text_atlas,NULL,NULL,&char_w,&char_h);
    char_w/=128;
    SDL_Rect dstrect={x,y,char_w,char_h};
    if(w<char_w)
        return 0;
    char* s=e->a_data;
    //calculate text height
    if(e->flags & UI_TEXT_CENTER_VERTICAL){
        int text_height=0,c_x=0;
        char*s_c=s;
        while(text_height+char_h<h&&*s_c) {
            while(c_x+char_w<w&&*s_c){
                c_x+=char_w;
                s_c++;
            }
            c_x=0;
            text_height+=char_h;
        }
        dstrect.y+=(h-text_height)/2;
    }
    while(dstrect.y+char_h<y+h&&*s){
        //calculate line width
        if(e->flags & UI_TEXT_CENTER_HORIZONTAL){
            int line_width=0;
            char*s_c=s;
            while(line_width+char_w<w&&*s_c){line_width+=char_w;s_c++;}
            dstrect.x=x+((w-line_width)/2);
        }
        while(dstrect.x+char_w<x+w&&*s){
            if(*s=='\n'){
                s++;
                break;
            }

            SDL_Rect srcrect={*s*dstrect.w,0,char_w,char_h};
            SDL_RenderCopy(target->renderer,text_atlas,&srcrect,&dstrect);
            dstrect.x+=char_w;
            s++;
        }
        dstrect.x=x;
        dstrect.y+=char_h;
    }
}

uielement* ui_create_color(unsigned int color){
    uielement* e= calloc(sizeof (uielement),1);
    *e= (uielement) {.type=UI_TYPE_SOLID_COLOR,.render=ui_render_color,.data=color};
    return e;
}
uielement* ui_create_text(char* s, char* f, int size){
    uielement* e= calloc(sizeof(uielement),1);
    *e=(uielement){.type=UI_TYPE_TEXT,.render=ui_render_text,.data=size,.a_data=strdup(s),.a_data_2=strdup(f),.flags=UI_MEM_ALLOCED|UI_MEM_ALLOCED_2};
    return e;
}
uielement* ui_create_text_input(char* f, int size){
    uielement* e= calloc(sizeof(uielement),1);
    *e=(uielement){.type=UI_TYPE_TEXT_INPUT,.render=ui_render_text,.ontextinput=ui_text_input_ontext,.data=size,.a_data=calloc(1024,1),.a_data_2=strdup(f),.flags=UI_MEM_ALLOCED|UI_MEM_ALLOCED_2};
    return e;
}
uielement* ui_create_vertical(){
    uielement* e= calloc(sizeof(uielement),1);
    *e=(uielement){.type=UI_TYPE_VERTICAL,.render=ui_render_vertical,.onscroll=ui_vertical_on_scroll,.children=dynarray_init(),.flags=UI_HAS_CHILDREN};
    return e;
}
uielement* ui_create_horizontal(){
    uielement* e= calloc(sizeof(uielement),1);
    *e=(uielement){.type=UI_TYPE_HORIZONTAL,.render=ui_render_horizontal,.onscroll=ui_vertical_on_scroll,.children=dynarray_init(),.flags=UI_HAS_CHILDREN};
    return e;
}
uielement* ui_create_stack(){
    uielement* e= calloc(sizeof(uielement),1);
    *e=(uielement){.type=UI_TYPE_STACK,.render=ui_render_stack,.children=dynarray_init(),.flags=UI_HAS_CHILDREN};
    return e;
}
uielement* ui_create_spacer(){
    uielement* e= calloc(sizeof(uielement),1);
    *e=(uielement){.type=UI_TYPE_SPACER};
    return e;
}

void ui_handle_click(rendertarget* target, int x, int y){
    target->text_in_focus=NULL;
    for (int i = target->render_order->size-1; i>=0; --i){
        uielement* e =target->render_order->data[i];
        if(e->last_bb.x<x&&x<e->last_bb.x+e->last_bb.w&&e->last_bb.y<y&&y<e->last_bb.y+e->last_bb.h){
            if(e->type==UI_TYPE_TEXT_INPUT)
                target->text_in_focus=e;
            if(e->onclick)
                e->onclick(e);
            return;
        }
    }
}
void ui_handle_right_click(rendertarget* target, int x, int y){
    for (int i = target->render_order->size-1; i>=0; --i){
        uielement* e =target->render_order->data[i];
        if(e->last_bb.x<x&&x<e->last_bb.x+e->last_bb.w&&e->last_bb.y<y&&y<e->last_bb.y+e->last_bb.h){
            if(e->onrightclick)
                e->onrightclick(e);
            return;
        }
    }
}
void ui_handle_scroll(rendertarget* target, int x, int y, int wheel_x, int wheel_y){
    for (int i = target->render_order->size-1; i>=0; --i){
        uielement* e =target->render_order->data[i];
        if(e->onscroll){
            if(e->flags&UI_SCROLL_DISABLE)
                return;
            if(e->last_bb.x<x&&x<e->last_bb.x+e->last_bb.w&&e->last_bb.y<y&&y<e->last_bb.y+e->last_bb.h)
                return e->onscroll(e,wheel_x,wheel_y);
        }
    }
}
void ui_handle_text_input(rendertarget* target, char* input){
    if(target->text_in_focus)
        target->text_in_focus->ontextinput(target->text_in_focus,input);
}

rendertarget* init_sdl2_ui(int w, int h){
    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();
    SDL_Window *window = SDL_CreateWindow("test ui", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    font_cache=hashmap_init();

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    SDL_StartTextInput();

    rendertarget* sdl_target= calloc(sizeof (rendertarget),1);
    *sdl_target=(rendertarget){.w=w,.h=h,.window=window,.renderer=renderer,.render_order=dynarray_init()};
    return sdl_target;
}
int deinit_sdl2_ui(rendertarget* sdl_target){
    SDL_DestroyRenderer(sdl_target->renderer);
    SDL_DestroyWindow(sdl_target->window);
    free(sdl_target);
    hashmap_free(font_cache);
    TTF_Quit();
    SDL_Quit();
    return 0;
}

void free_element(uielement* e){
    if(e->flags&UI_HAS_CHILDREN)
        dynarray_free(e->children);
    if(e->flags&UI_MEM_ALLOCED)
        free(e->a_data);
    if(e->flags&UI_MEM_ALLOCED_2)
        free(e->a_data_2);
    //if(e==text_in_focus)
    //    text_in_focus=NULL;
    free(e);
}
void free_element_tree(uielement* e){
    if(e->flags&UI_HAS_CHILDREN){
        for (int i = 0; i < e->children->size; ++i) {
            free_element_tree(e->children->data[i]);
        }
    }
    free_element(e);
}

SDL_Event event;
int ui_handle_input(rendertarget* target, bool* running){
    bool need_update;
    while(SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_QUIT:
                *running = false; break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_TAB){
                    debug = !debug;
                    need_update = true;
                }
                if(event.key.keysym.sym == SDLK_v && SDL_GetModState() & KMOD_CTRL){
                    ui_handle_text_input(target, SDL_GetClipboardText());
                }
                if(event.key.keysym.sym==SDLK_BACKSPACE){
                    ui_handle_text_input(target,"\b");
                }
                if(event.key.keysym.sym==SDLK_RETURN){
                    ui_handle_text_input(target,"\n");
                }
                need_update=true;
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED){
                    target->w = event.window.data1;
                    target->h = event.window.data2;
                    need_update = true;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT)
                    ui_handle_click(target, event.button.x, event.button.y);
                else if (event.button.button == SDL_BUTTON_RIGHT)
                    ui_handle_right_click(target,event.button.x, event.button.y);
                need_update = true;
                break;
            case SDL_MOUSEWHEEL:
                ui_handle_scroll(target,event.wheel.mouseX, event.wheel.mouseY, event.wheel.x, event.wheel.y);
                need_update = true;
                break;
            case SDL_TEXTINPUT:
                ui_handle_text_input(target,event.text.text);
                need_update=true;
                break;
        }
    }
    return need_update;
}