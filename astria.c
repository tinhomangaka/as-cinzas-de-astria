
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define W 1280
#define H 720
#define TILE 48
#define MW 40
#define MH 25

typedef enum { TITLE, WORLD, MENU, DIALOG, BATTLE, SHOP, PARTY, QUESTS } State;
typedef enum { VILLAGE, FOREST, SHRINE } Area;

typedef struct {
    int hp,maxHp,mp,maxMp,level,xp,nextXp,gold,potions;
    int weapon,armor,quest,guardianDead;
    int area,x,y;
} Save;

typedef struct {
    int x,y;
    const char *name;
    const char *line;
} NPC;

static State state=TITLE;
static Area area=VILLAGE;
static Save s={140,140,36,36,1,0,100,150,4,0,0,0,0,0,VILLAGE,8,12};
static int map[3][MH][MW];
static int enemyType=0, enemyHp=0, enemyMax=0, turnFlash=0;
static int dialogPage=0, dialogCount=0;
static const char *dialogs[8];
static char toast[160]="";
static int toastTime=0;
static float anim=0;

static NPC village[]={
    {7,6,"Mira","A estrada do norte foi tomada pelas criaturas de cinzas."},
    {13,9,"Ferreiro","A Espada de Cinza custa caro, mas aguenta batalhas longas."},
    {20,7,"Ancia","O santuario guarda a origem daquilo que caiu do ceu."},
    {33,5,"Nilo","Se voce for a floresta, procure a luz roxa."}
};
static NPC forest[]={
    {8,8,"Lira","As arvores estao doentes. Algo esta drenando a energia daqui."},
    {24,17,"Explorador","O santuario fica no extremo norte."}
};

static void toastMsg(const char *m){snprintf(toast,sizeof(toast),"%s",m);toastTime=180;}
static int atk(){return 16+s.level*3+s.weapon*8;}
static int mag(){return 24+s.level*5;}
static int def(){return 5+s.level*2+s.armor*7;}
static int xpNext(int lv){return 100+(lv-1)*80;}

static void levelUp(){
    while(s.xp>=s.nextXp){
        s.xp-=s.nextXp;s.level++;s.nextXp=xpNext(s.level);
        s.maxHp+=20;s.maxMp+=7;s.hp=s.maxHp;s.mp=s.maxMp;
        char b[80];snprintf(b,sizeof(b),"Nivel %d! Seus atributos aumentaram.",s.level);toastMsg(b);
    }
}

static void buildMaps(){
    memset(map,0,sizeof(map));
    for(int a=0;a<3;a++){
        for(int x=0;x<MW;x++){map[a][0][x]=2;map[a][MH-1][x]=2;}
        for(int y=0;y<MH;y++){map[a][y][0]=2;map[a][y][MW-1]=2;}
    }
    // Village
    for(int x=2;x<38;x++) map[VILLAGE][12][x]=4;
    for(int y=3;y<23;y++) map[VILLAGE][y][18]=4;
    for(int x=5;x<9;x++)for(int y=4;y<8;y++)map[VILLAGE][y][x]=3;
    for(int x=11;x<16;x++)for(int y=7;y<11;y++)map[VILLAGE][y][x]=3;
    for(int x=25;x<31;x++)for(int y=12;y<17;y++)map[VILLAGE][y][x]=3;
    for(int x=17;x<23;x++)for(int y=3;y<7;y++)map[VILLAGE][y][x]=1;
    map[VILLAGE][12][38]=5;

    // Forest
    for(int x=2;x<38;x++) map[FOREST][12][x]=4;
    for(int y=3;y<23;y++) map[FOREST][y][20]=4;
    for(int y=3;y<11;y++) map[FOREST][y][6]=2;
    for(int y=15;y<23;y++) map[FOREST][y][30]=2;
    for(int x=23;x<36;x++) map[FOREST][5][x]=2;
    map[FOREST][12][1]=5; map[FOREST][4][34]=6;

    // Shrine
    for(int x=4;x<36;x++)for(int y=4;y<21;y++)map[SHRINE][y][x]=7;
    for(int x=4;x<36;x++){map[SHRINE][3][x]=2;map[SHRINE][21][x]=2;}
    for(int y=3;y<22;y++){map[SHRINE][y][4]=2;map[SHRINE][y][35]=2;}
    for(int x=15;x<25;x++)map[SHRINE][10][x]=6;
    map[SHRINE][20][19]=5;
}

static bool blocked(int x,int y){
    if(x<1||y<1||x>=MW-1||y>=MH-1)return true;
    int t=map[area][y][x];
    return t==1||t==2||t==3||t==6||t==7;
}

static void tile(int x,int y,int t){
    Rectangle r={(float)x*TILE,(float)y*TILE,TILE,TILE};
    Color c=(Color){54,78,61,255};
    if(t==0)c=(Color){54,78,61,255};
    if(t==1)c=(Color){43,92,126,255};
    if(t==2)c=(Color){29,49,36,255};
    if(t==3)c=(Color){121,77,58,255};
    if(t==4)c=(Color){126,106,81,255};
    if(t==5)c=(Color){83,55,103,255};
    if(t==6)c=(Color){121,67,153,255};
    if(t==7)c=(Color){71,64,82,255};
    DrawRectangleRec(r,c);

    // pixel-art style details
    if(t==0){DrawRectangle(x*TILE+8,y*TILE+9,3,3,(Color){77,104,72,255});}
    if(t==1){DrawLine(x*TILE+8,y*TILE+25,x*TILE+35,y*TILE+25,(Color){80,150,184,255});}
    if(t==2){
        DrawRectangle(x*TILE+20,y*TILE+28,8,20,(Color){88,61,43,255});
        DrawRectangle(x*TILE+10,y*TILE+10,28,22,(Color){35,95,53,255});
        DrawRectangle(x*TILE+15,y*TILE+6,18,8,(Color){45,113,61,255});
    }
    if(t==3){
        DrawRectangle(x*TILE+5,y*TILE+4,38,24,(Color){169,104,73,255});
        DrawRectangle(x*TILE+17,y*TILE+29,14,19,(Color){58,45,43,255});
    }
    if(t==6){DrawCircle(x*TILE+24,y*TILE+24,15,(Color){177,100,213,255});}
}

static void playerSprite(float px,float py,int frame){
    float x=px*TILE+24,y=py*TILE+24+((frame%2)?1:0);
    DrawRectangle((int)x-12,(int)y-1,24,25,(Color){91,66,145,255});
    DrawRectangle((int)x-9,(int)y-18,18,17,(Color){225,186,149,255});
    DrawRectangle((int)x-14,(int)y-23,28,7,(Color){48,40,63,255});
    DrawRectangle((int)x-14,(int)y+24,9,14,(Color){43,42,53,255});
    DrawRectangle((int)x+5,(int)y+24,9,14,(Color){43,42,53,255});
    DrawRectangle((int)x+12,(int)y+5,18,5,(Color){209,194,172,255});
}

static void npcSprite(NPC *n){
    float x=n->x*TILE+24,y=n->y*TILE+24;
    DrawRectangle((int)x-10,(int)y,20,23,(Color){177,82,103,255});
    DrawCircle(x,y-9,10,(Color){224,184,148,255});
    DrawText(n->name,(int)x-MeasureText(n->name,13)/2,(int)y-37,13,RAYWHITE);
}

static void header(){
    const char *name=area==VILLAGE?"VILA AURORA":area==FOREST?"FLORESTA CINZENTA":"SANTUARIO";
    DrawRectangle(0,0,W,70,(Color){12,16,25,235});
    DrawText(name,22,15,27,RAYWHITE);
    DrawText(TextFormat("Nv %d  HP %d/%d  MP %d/%d  Ouro %d",s.level,s.hp,s.maxHp,s.mp,s.maxMp,s.gold),22,45,16,LIGHTGRAY);
}

static void world(){
    for(int y=0;y<MH;y++)for(int x=0;x<MW;x++)tile(x,y,map[area][y][x]);
    NPC *n=area==VILLAGE?village:forest;
    int count=area==VILLAGE?4:2;
    if(area!=SHRINE)for(int i=0;i<count;i++)npcSprite(&n[i]);
    playerSprite((float)s.x,(float)s.y,(int)(anim*8));
    header();

    DrawRectangle(W-400,12,370,46,(Color){28,33,46,240});
    const char *q=s.quest==0?"Fale com a Ancia":
                 s.quest==1?"Va ate a Floresta":
                 s.quest==2?"Encontre o Santuario":
                 s.quest==3?"Derrote o Guardiao":"Missao concluida";
    DrawText(q,W-380,28,16,RAYWHITE);

    DrawCircle(105,H-100,66,(Color){20,25,35,190});
    DrawCircle(105,H-100,26,(Color){140,140,155,200});
    DrawCircle(W-100,H-110,40,(Color){102,65,151,235});
    DrawText("A",W-110,H-122,25,RAYWHITE);
    DrawCircle(W-195,H-60,30,(Color){57,91,126,235});
    DrawText("M",W-204,H-70,19,RAYWHITE);

    if(toastTime>0){
        DrawRectangle(250,H-104,780,58,(Color){10,13,20,235});
        DrawText(toast,270,H-85,18,RAYWHITE);toastTime--;
    }
}

static void title(){
    ClearBackground((Color){9,8,17,255});
    DrawCircle(940,280,190,(Color){88,45,120,80});
    for(int i=0;i<22;i++){
        float x=780+(i*73)%340,y=130+(i*47)%330;
        DrawRectangle((int)x,(int)y,4,4,(Color){181,123,213,180});
    }
    DrawText("AS CINZAS",90,190,74,RAYWHITE);
    DrawText("DE ASTRIA",90,270,74,(Color){204,155,232,255});
    DrawText("v0.4 — Jornada das Cinzas",95,370,25,LIGHTGRAY);
    DrawText("TOQUE PARA INICIAR",95,455,25,RAYWHITE);
    DrawText("JRPG original • exploração • missões • equipamentos • batalhas",95,505,18,GRAY);
}

static void beginDialog(const char **d,int n){for(int i=0;i<n;i++)dialogs[i]=d[i];dialogCount=n;dialogPage=0;state=DIALOG;}
static void dialog(){
    ClearBackground((Color){17,20,29,255});
    DrawRectangle(65,450,1150,205,(Color){9,12,19,248});
    DrawRectangleLines(65,450,1150,205,(Color){157,112,197,255});
    DrawText(dialogs[dialogPage],100,505,23,RAYWHITE);
    DrawText("TOQUE / ENTER",1000,615,15,GRAY);
}

static void saveGame(){
    FILE *f=fopen("astria_v04.sav","wb");
    if(f){fwrite(&s,sizeof(s),1,f);fclose(f);toastMsg("Jogo salvo.");}
    else toastMsg("Falha ao salvar.");
}
static void loadGame(){
    FILE *f=fopen("astria_v04.sav","rb");
    if(f){fread(&s,sizeof(s),1,f);fclose(f);area=(Area)s.area;toastMsg("Jogo carregado.");}
    else toastMsg("Nenhum save encontrado.");
}

static void menu(){
    ClearBackground((Color){12,15,24,255});
    DrawText("MENU",75,55,46,RAYWHITE);
    DrawText(TextFormat("Nv %d  XP %d/%d",s.level,s.xp,s.nextXp),75,120,22,LIGHTGRAY);
    DrawText(TextFormat("HP %d/%d  MP %d/%d",s.hp,s.maxHp,s.mp,s.maxMp),75,155,22,LIGHTGRAY);
    DrawText(TextFormat("Pocoes %d  Ouro %d",s.potions,s.gold),75,190,22,LIGHTGRAY);
    const char *it[]={"CONTINUAR","SALVAR","CARREGAR","PARTY","MISSOES","LOJA"};
    for(int i=0;i<6;i++){
        Rectangle r={680,80+i*82,420,62};
        DrawRectangleRec(r,(Color){44,49,67,255});
        DrawText(it[i],r.x+25,r.y+18,22,RAYWHITE);
    }
}

static void party(){
    ClearBackground((Color){14,17,27,255});
    DrawText("GRUPO",75,60,44,RAYWHITE);
    DrawText("AREN",90,150,29,RAYWHITE);
    DrawText("Espadachim das Cinzas",90,183,18,LIGHTGRAY);
    DrawText(TextFormat("HP %d/%d   MP %d/%d",s.hp,s.maxHp,s.mp,s.maxMp),90,220,20,RAYWHITE);
    DrawText(TextFormat("ATK %d   DEF %d   MAG %d",atk(),def(),mag()),90,255,20,RAYWHITE);
    DrawText(s.weapon?"Arma: Espada de Cinza":"Arma: Lamina simples",90,305,20,LIGHTGRAY);
    DrawText(s.armor?"Armadura: Couraca Astral":"Armadura: Tecido",90,338,20,LIGHTGRAY);
    DrawText("M / ESC para voltar",90,600,18,GRAY);
}

static void quests(){
    ClearBackground((Color){13,16,25,255});
    DrawText("DIARIO DE MISSOES",70,60,42,RAYWHITE);
    const char *a[]={
        "1. Falar com a Ancia",
        "2. Explorar a Floresta Cinzenta",
        "3. Encontrar o Santuario",
        "4. Derrotar o Guardiao de Cinzas"
    };
    for(int i=0;i<4;i++){
        Color c=i<s.quest? (Color){122,183,135,255} : i==s.quest ? (Color){222,188,91,255} : LIGHTGRAY;
        DrawText(a[i],95,145+i*65,23,c);
    }
    DrawText("M / ESC para voltar",95,600,18,GRAY);
}

static void shop(){
    ClearBackground((Color){18,16,22,255});
    DrawText("LOJA DO MERCADOR",70,60,42,RAYWHITE);
    DrawText(TextFormat("Ouro: %d",s.gold),70,112,23,GOLD);
    const char *a[]={"Pocao - 25 ouro","Espada de Cinza - 120 ouro","Couraca Astral - 100 ouro"};
    for(int i=0;i<3;i++){
        DrawRectangle(80,175+i*100,610,70,(Color){48,44,59,255});
        DrawText(a[i],110,198+i*100,22,RAYWHITE);
    }
    DrawText("M / ESC para sair",80,590,18,GRAY);
}

static void startBattle(int type){
    enemyType=type;
    enemyMaxHp=type==1?70+s.level*12:type==2?120+s.level*18:260+s.level*25;
    enemyHp=enemyMaxHp;state=BATTLE;
}

static const char *enemyName(){
    return enemyType==1?"Lobo de Cinzas":enemyType==2?"Sentinela Corrompida":"Guardiao de Cinzas";
}

static void battleSprite(){
    float bob=sinf(anim*5)*3;
    float x=950,y=285+bob;
    if(enemyType==1){
        DrawRectangle(x-58,y-18,116,58,(Color){105,73,130,255});
        DrawCircle(x-42,y-42,25,(Color){124,87,149,255});
        DrawCircle(x+42,y-42,25,(Color){124,87,149,255});
        DrawCircle(x-23,y-18,6,RED);DrawCircle(x+23,y-18,6,RED);
    }else if(enemyType==2){
        DrawRectangle(x-75,y-70,150,145,(Color){61,76,97,255});
        DrawRectangle(x-50,y-110,100,45,(Color){84,99,123,255});
        DrawCircle(x-25,y-92,7,RED);DrawCircle(x+25,y-92,7,RED);
    }else{
        DrawCircle(x,y-5,92,(Color){65,47,79,255});
        DrawRectangle(x-85,y+48,170,38,(Color){48,35,59,255});
        DrawCircle(x-32,y-25,13,(Color){229,99,137,255});
        DrawCircle(x+32,y-25,13,(Color){229,99,137,255});
        DrawRectangle(x-110,y+82,220,10,(Color){136,79,171,255});
    }
}

static void heroBattleSprite(){
    float bob=sinf(anim*5)*2;
    float x=270,y=365+bob;
    DrawRectangle(x-28,y-20,56,82,(Color){91,66,145,255});
    DrawCircle(x,y-50,28,(Color){225,185,148,255});
    DrawRectangle(x-34,y-65,68,9,(Color){48,40,63,255});
    DrawRectangle(x-40,y+62,28,48,(Color){42,41,52,255});
    DrawRectangle(x+12,y+62,28,48,(Color){42,41,52,255});
    DrawRectangle(x+30,y-12,115,8,(Color){216,201,178,255});
}

static void battle(){
    ClearBackground((Color){18,21,30,255});
    DrawText("BATALHA",42,35,38,RAYWHITE);
    DrawText(enemyName(),800,90,25,RAYWHITE);
    DrawRectangle(770,130,350,18,(Color){57,40,46,255});
    DrawRectangle(770,130,(int)(350*(enemyHp/(float)enemyMaxHp)),18,(Color){207,73,87,255});
    heroBattleSprite();battleSprite();
    DrawText(TextFormat("HP %d/%d  MP %d/%d",s.hp,s.maxHp,s.mp,s.maxMp),50,505,22,RAYWHITE);
    DrawRectangle(30,550,1220,145,(Color){9,12,19,248});
    const char *b[]={"1 ATK","2 MAG","3 ITEM","4 FUGIR"};
    for(int i=0;i<4;i++){
        DrawRectangle(60+i*295,590,245,70,(Color){45,50,68,255});
        DrawText(b[i],130+i*295,612,24,RAYWHITE);
    }
}

static void enemyTurn(){
    int dmg=7+enemyType*4-def()/4;
    if(dmg<1)dmg=1;s.hp-=dmg;
    if(s.hp<=0){s.hp=s.maxHp/2;toastMsg("Derrota. Voce recuou para a vila.");area=VILLAGE;s.area=VILLAGE;s.x=8;s.y=12;state=WORLD;}
}

static void victory(){
    int xp=enemyType==1?55:enemyType==2?90:240;
    int gold=enemyType==1?20:enemyType==2?45:180;
    s.xp+=xp;s.gold+=gold;
    if(enemyType==3){s.guardianDead=1;s.quest=4;}
    levelUp();
    char b[100];snprintf(b,sizeof(b),"Vitoria! +%d XP  +%d ouro.",xp,gold);toastMsg(b);
    state=WORLD;
}

static void action(int a){
    if(a==0){
        enemyHp-=atk()+GetRandomValue(-4,7);
        if(enemyHp<=0){victory();return;} enemyTurn();
    }else if(a==1){
        if(s.mp<8){toastMsg("MP insuficiente.");return;}
        s.mp-=8;enemyHp-=mag()+GetRandomValue(-5,9);
        if(enemyHp<=0){victory();return;}enemyTurn();
    }else if(a==2){
        if(s.potions<=0){toastMsg("Sem pocoes.");return;}
        s.potions--;s.hp+=50;if(s.hp>s.maxHp)s.hp=s.maxHp;enemyTurn();
    }else{
        if(enemyType==3){toastMsg("O Guardiao impede a fuga.");return;}
        if(GetRandomValue(0,99)<65){toastMsg("Voce escapou.");state=WORLD;}else enemyTurn();
    }
}

static void interact(){
    NPC *n=area==VILLAGE?village:forest;int count=area==VILLAGE?4:2;
    for(int i=0;i<count;i++)if(abs(n[i].x-s.x)<=1&&abs(n[i].y-s.y)<=1){
        if(strcmp(n[i].name,"Ancia")==0 && s.quest==0)s.quest=1;
        const char *d[3]={n[i].line,"As cinzas escondem mais do que parecem.","Continue a jornada."};
        beginDialog(d,3);return;
    }
    if(area==VILLAGE && s.x>=25&&s.x<=31&&s.y>=12&&s.y<=17){state=SHOP;return;}
    if(area==VILLAGE && s.x>=37&&s.y==12){
        area=FOREST;s.area=FOREST;s.x=3;s.y=12;if(s.quest==1)s.quest=2;toastMsg("Floresta Cinzenta.");return;
    }
    if(area==FOREST && s.x<=2&&s.y==12){area=VILLAGE;s.area=VILLAGE;s.x=35;s.y=12;return;}
    if(area==FOREST && s.x>=33&&s.y<=5){
        area=SHRINE;s.area=SHRINE;s.x=19;s.y=19;s.quest=3;toastMsg("O Santuario desperta.");return;
    }
    if(area==SHRINE && s.x==19&&s.y==20&&!s.guardianDead){startBattle(3);return;}
    if(area==SHRINE && s.x==19&&s.y==20&&s.guardianDead){area=FOREST;s.area=FOREST;s.x=33;s.y=5;}
}

static void movePlayer(int dx,int dy){
    int nx=s.x+dx,ny=s.y+dy;
    if(!blocked(nx,ny)){s.x=nx;s.y=ny;}
}

static void randomEncounter(){
    if(area==FOREST&&GetRandomValue(0,999)<5)startBattle(GetRandomValue(1,2));
}

static void touchWorld(Vector2 p){
    if(CheckCollisionPointCircle(p,(Vector2){105,H-100},90)){
        float dx=p.x-105,dy=p.y-(H-100);
        if(fabsf(dx)>fabsf(dy))movePlayer(dx>0?1:-1,0);else movePlayer(0,dy>0?1:-1);
        randomEncounter();return;
    }
    if(CheckCollisionPointCircle(p,(Vector2){W-195,H-60},45)){state=MENU;return;}
    if(CheckCollisionPointCircle(p,(Vector2){W-100,H-110},58)){interact();return;}
}

static void keyboard(){
    if(state==TITLE){if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_SPACE))state=WORLD;return;}
    if(state==WORLD){
        if(IsKeyPressed(KEY_UP)||IsKeyPressed(KEY_W))movePlayer(0,-1);
        if(IsKeyPressed(KEY_DOWN)||IsKeyPressed(KEY_S))movePlayer(0,1);
        if(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_A))movePlayer(-1,0);
        if(IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_D))movePlayer(1,0);
        if(IsKeyPressed(KEY_E))interact();
        if(IsKeyPressed(KEY_TAB))state=MENU;
        if(IsKeyPressed(KEY_F5))saveGame();
        randomEncounter();
    }else if(state==MENU){
        if(IsKeyPressed(KEY_TAB)||IsKeyPressed(KEY_ESCAPE))state=WORLD;
    }else if(state==DIALOG){
        if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_SPACE)){if(++dialogPage>=dialogCount)state=WORLD;}
    }else if(state==BATTLE){
        if(IsKeyPressed(KEY_ONE))action(0);if(IsKeyPressed(KEY_TWO))action(1);
        if(IsKeyPressed(KEY_THREE))action(2);if(IsKeyPressed(KEY_FOUR))action(3);
    }else if(state==SHOP||state==PARTY||state==QUESTS){
        if(IsKeyPressed(KEY_ESCAPE)||IsKeyPressed(KEY_TAB))state=MENU;
    }
}

int main(){
    SetConfigFlags(FLAG_FULLSCREEN_MODE|FLAG_VSYNC_HINT);
    InitWindow(W,H,"As Cinzas de Astria v0.4");
    SetTargetFPS(60);SetRandomSeed((unsigned)time(NULL));buildMaps();
    s.nextXp=xpNext(s.level);

    while(!WindowShouldClose()){
        anim+=GetFrameTime();keyboard();

        if((IsMouseButtonPressed(MOUSE_LEFT_BUTTON)||GetTouchPointCount()>0)){
            Vector2 p=GetTouchPointCount()?GetTouchPosition(0):GetMousePosition();
            if(state==TITLE)state=WORLD;
            else if(state==WORLD)touchWorld(p);
            else if(state==DIALOG){if(++dialogPage>=dialogCount)state=WORLD;}
            else if(state==MENU){
                if(CheckCollisionPointRec(p,(Rectangle){680,80,420,62}))state=WORLD;
                else if(CheckCollisionPointRec(p,(Rectangle){680,162,420,62}))saveGame();
                else if(CheckCollisionPointRec(p,(Rectangle){680,244,420,62}))loadGame();
                else if(CheckCollisionPointRec(p,(Rectangle){680,326,420,62}))state=PARTY;
                else if(CheckCollisionPointRec(p,(Rectangle){680,408,420,62}))state=QUESTS;
                else if(CheckCollisionPointRec(p,(Rectangle){680,490,420,62}))state=SHOP;
            }else if(state==BATTLE){
                if(CheckCollisionPointRec(p,(Rectangle){60,590,245,70}))action(0);
                else if(CheckCollisionPointRec(p,(Rectangle){355,590,245,70}))action(1);
                else if(CheckCollisionPointRec(p,(Rectangle){650,590,245,70}))action(2);
                else if(CheckCollisionPointRec(p,(Rectangle){945,590,245,70}))action(3);
            }else if(state==SHOP){
                if(CheckCollisionPointRec(p,(Rectangle){80,175,610,70})&&s.gold>=25){s.gold-=25;s.potions++;toastMsg("Pocao comprada.");}
                else if(CheckCollisionPointRec(p,(Rectangle){80,275,610,70})&&s.gold>=120){s.gold-=120;s.weapon=1;toastMsg("Espada equipada.");}
                else if(CheckCollisionPointRec(p,(Rectangle){80,375,610,70})&&s.gold>=100){s.gold-=100;s.armor=1;toastMsg("Couraca equipada.");}
            }
        }

        BeginDrawing();
        if(state==TITLE)title();
        else if(state==WORLD)world();
        else if(state==MENU)menu();
        else if(state==DIALOG)dialog();
        else if(state==BATTLE)battle();
        else if(state==SHOP)shop();
        else if(state==PARTY)party();
        else if(state==QUESTS)quests();
        EndDrawing();
    }
    CloseWindow();return 0;
}
