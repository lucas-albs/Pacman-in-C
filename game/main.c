#include <allegro.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "teclado.h"


///Variaveis globais
const int TILESIZE = 15;
float SPEED = 3.0*15.0/60.0;

int matriz_espelho[31][28];

volatile int exit_game;
volatile int ticks, cronometro;


void fun_fps(void)
{
    int fps = 0;

    while(fps< 5999999){
        fps++;
    }
}

enum
{
  AGUA = 0,
  PONTINHOS = 1,
  PONTAO = 2,
  PAREDE = 3,
  PORTAO = 4
};

int** Carregar_Mapa(const char* nome_arquivo, int* linhas, int* colunas)
{
   FILE* f = fopen(nome_arquivo, "r");
   int** matriz;

   if(f != NULL)
   {
     int i, j;

     fscanf(f, "%d %d", linhas, colunas);

     //ALOCA O MAPA
     matriz = (int**) malloc ( (*linhas) * sizeof(int*));
     for(i = 0; i < *linhas; i++)
      matriz[i] = (int*) malloc( (*colunas) * sizeof(int));

     //CARREGA TILES
     for(i = 0; i < *linhas; i++)
     {
        for(j = 0; j < *colunas; j++)
        {
          fscanf(f, "%d", &matriz[i][j]);
        }
     }

     fclose(f);
   }

   return matriz;
}


void Desenhar_Mapa(BITMAP* buffer, int** mapa, int linhas, int colunas)
{

    BITMAP* pedra = load_bitmap("files/pedra.bmp", NULL);
    int i, j;

    for(i = 0; i < linhas; i++)
    {
        for(j = 0; j < colunas; j++)
        {
            if(mapa[i][j] == AGUA || matriz_espelho[i][j] == AGUA)
                rectfill(buffer, (j*TILESIZE)+(2*TILESIZE), i*TILESIZE+50, (j*TILESIZE)+(2*TILESIZE) + TILESIZE, ( i*TILESIZE+50) + TILESIZE, makecol(46,155,167));

            else if(mapa[i][j] == PONTINHOS && matriz_espelho[i][j] != AGUA){
                rectfill(buffer, (j*TILESIZE)+(2*TILESIZE), i*TILESIZE+50, (j*TILESIZE)+(2*TILESIZE) + TILESIZE, ( i*TILESIZE+50) + TILESIZE, makecol(100,230,0));
                matriz_espelho[i][j] = PONTINHOS;
           }

            else if(mapa[i][j] == PONTAO && matriz_espelho[i][j] != AGUA){
                rectfill(buffer, (j*TILESIZE)+(2*TILESIZE), i*TILESIZE+50, (j*TILESIZE)+(2*TILESIZE) + TILESIZE, ( i*TILESIZE+50) + TILESIZE, makecol(213,21,234));
                matriz_espelho[i][j] = PONTAO;
            }

            else if(mapa[i][j] == PAREDE) {
                rectfill(buffer, (j*TILESIZE)+(2*TILESIZE), i*TILESIZE+50, (j*TILESIZE)+(2*TILESIZE) + TILESIZE, ( i*TILESIZE+50) + TILESIZE, makecol(45,64,234));
                matriz_espelho[i][j] = PAREDE;
            }

            else if(mapa[i][j] == PORTAO){
                //draw_sprite_ex(buffer, j * TILESIZE, i * TILESIZE, (j * TILESIZE) + TILESIZE, (i * TILESIZE) + TILESIZE, makecol(133,133,133));
                draw_sprite_ex(buffer, pedra, (j*TILESIZE)+(2*TILESIZE),  i*TILESIZE+50, DRAW_SPRITE_NORMAL, DRAW_SPRITE_NO_FLIP);
                matriz_espelho[i][j] = PAREDE;
           }
        }
     }

    destroy_bitmap(pedra);
}

void Libera_Mapa(int** mapa, int linhas)
{
   int i;
   for(i = 0; i < linhas; i++)
    free(mapa[i]);

   free(mapa);
}

void definicao_sentido(int* sent, int* prox_sent, int player_ou_n)
{           //player_ou_n serve p identificar se a funcao esta sendo chamada para controlar um fantasma ou nao, e agir de acordo
    if (player_ou_n == 1) {
        keyboard_input();
        if(!apertou(KEY_RIGHT)) *prox_sent= 1;
        else if(!apertou(KEY_LEFT)) *prox_sent= 2;
        else if(!apertou(KEY_UP)) *prox_sent= 3;
        else if(!apertou(KEY_DOWN)) *prox_sent= 4;
    }

    if (*prox_sent== 1 && *sent==2) *sent=*prox_sent; //permite inverter o sentido
    if (*prox_sent== 2 && *sent==1) *sent=*prox_sent;
    if (*prox_sent == 3 && *sent==4) *sent=*prox_sent;
    if (*prox_sent == 4 && *sent==3) *sent=*prox_sent;

    if(*sent==0) *sent=*prox_sent;    //se estiver parado, incia o movimento
}

int colisoes(double* eixo_x, double* eixo_y, int* sent, int* prox_sent, int* medo, int player_ou_n)    //player_ou_n -> funcao sendo chamada para controlar um fantasma ou nao, e agir de acordo
{
    int y_cima, y_baix, x_esqu, x_dire;     //usando as pos_x e pos_y, sao transformadas de double para int e tratadas p refletir o tile do mapa como na funcao Desenhar_Mapa
                                            //cada uma ira refletir em um dos 4 cantos dos blocos, e irao reger o movimento do bloco pelos corredores
    int y_meio, x_meio;                     //em complemento, irao existir coordenadas no meio para calculo preciso da pontuacao




    y_cima = ((int)*eixo_y-50)/TILESIZE;
    y_baix = ((int)*eixo_y+(TILESIZE-1)-50)/TILESIZE;
    x_esqu = ((int)*eixo_x-(2*TILESIZE))/TILESIZE;
    x_dire = ((int)*eixo_x+(TILESIZE-1)-(2*TILESIZE))/TILESIZE;

    x_meio = ((int)*eixo_x+TILESIZE/2-(2*TILESIZE))/TILESIZE;
    y_meio = ((int)*eixo_y-50+TILESIZE/2)/TILESIZE;

    if(x_dire == 0 && *sent == 2){      //teleporta de um lado para o outro do mapa naquele corredor
        *eixo_x = 29.0*TILESIZE;
        return 0;
    }
    else if(x_esqu == 27 && *sent ==1){
        *eixo_x = 2.0*TILESIZE;
        return 0;
    }
    if (x_meio >= 13 && x_meio <= 14 && y_meio >= 13 && y_meio <=14) {
        *eixo_y = 11*TILESIZE+50;
        *eixo_x = 15*TILESIZE;
    }
    switch (*prox_sent) {               //definicao do sentido e do prox_sentido baseado na posicao das paredes
        case 1:{
            if (*sent ==3 || *sent ==4)
                if(matriz_espelho[y_cima][x_esqu+1] !=3 && matriz_espelho[y_baix][x_esqu+1] !=3) {
                    *sent=*prox_sent;
                    *prox_sent=0;
                }
            break;
        }
        case 2:{
            if (*sent ==3 || *sent ==4)
                if(matriz_espelho[y_cima][x_dire-1] !=3 && matriz_espelho[y_baix][x_dire-1] !=3 ) {
                    *sent=*prox_sent;
                    *prox_sent=0;
                }
            break;
        }
        case 3:{
            if (*sent ==1 || *sent==2)
                if(matriz_espelho[y_cima-1][x_esqu] !=3 && matriz_espelho[y_cima-1][x_dire] !=3) {
                    *sent=*prox_sent;
                    *prox_sent=0;
                }
            break;
        }
        case 4:{
            if (*sent ==1 || *sent==2)
                if(matriz_espelho[y_baix+1][x_esqu] !=3 && matriz_espelho[y_baix+1][x_dire] !=3) {
                    *sent=*prox_sent;
                    *prox_sent=0;
                }
            break;
        }
    }

    switch (*sent) {
        case 1:{
            if(matriz_espelho[y_cima][x_esqu+1] ==3 && matriz_espelho[y_baix][x_esqu+1] ==3) {
                if(*prox_sent!=1) *sent= *prox_sent;
                else *sent=0;
                *prox_sent=0;
            }
            break;
        }
        case 2:{
            if(matriz_espelho[y_cima][x_dire-1] ==3 && matriz_espelho[y_baix][x_dire-1] ==3) {
                if(*prox_sent!=2) *sent= *prox_sent;
                else *sent=0;
                *prox_sent=0;
            }
            break;
        }
        case 3:{
            if(matriz_espelho[y_baix-1][x_esqu] ==3 && matriz_espelho[y_baix-1][x_dire] ==3) {
                if(*prox_sent!=3) *sent= *prox_sent;
                else *sent=0;
                *prox_sent=0;
            }
            break;
        }
        case 4:{
            if(matriz_espelho[y_cima+1][x_esqu] ==3 && matriz_espelho[y_cima+1][x_dire] ==3 ) {
                if(*prox_sent!=4) *sent= *prox_sent;
                else *sent=0;
                *prox_sent=0;
            }
            break;
        }
    }

    if(player_ou_n ==1) {   //player aqui come os pontinhos
        if(matriz_espelho[y_meio][x_meio] ==PONTINHOS) {
            matriz_espelho[y_meio][x_meio] = AGUA;
            return 1;
            }
        else if(matriz_espelho[y_meio][x_meio] ==PONTAO) {
            matriz_espelho[y_meio][x_meio] = AGUA;
            *medo = 1;  //controla se os fantasmas irao ficar com medo ou nao
            return 1;
            }
        else return 0;
    }
}

int fantasmas (double* x, double* y, int* sent, int* prox_sent, double player_x, int player_y, int medo) {

    int sentido = *sent, prox_sentido = *prox_sent;
    double pos_x = *x, pos_y = *y; //mandar um ponteiro como ponteiro para uma funcao iria complicar o programa,
                                // atribuir o valor dele numa variavel e depois atribuir a variavel de volta no ponteiro facilita
    static int randhelp=0;
    randhelp++;    //usado em conjunto com srand para valores randoms
    if (cronometro ==0 || cronometro ==30 || sentido==0) {    //definira para onde o fantasma ira andar
        if(medo ==1){
            if(rand()%2 > 0) {
                if (player_x > pos_x) prox_sentido = 2;
                else if (player_x < pos_x) prox_sentido = 1;
            }
            else {
                if (player_y > pos_y) prox_sentido = 3;
                else if (player_y < pos_y) prox_sentido = 4;
            }
        }
        else {
            if(rand()%3 > 0){    //4/5 das vezes os fantasmas vao seguir o jogador ao inves de ir randomicamente
                if(rand()%2 > 0) {
                    if (player_x > pos_x) prox_sentido = 1;
                    else if (player_x < pos_x) prox_sentido = 2;
                }
                else {
                    if (player_y > pos_y) prox_sentido = 4;
                    else if (player_y < pos_y) prox_sentido = 3;
                }
            }
            else {
                prox_sentido = (rand()+randhelp)%4+1;
                if (sentido == 0)
                    sentido = prox_sentido;
            }
        }
    }

    colisoes(&pos_x, &pos_y, &sentido, &prox_sentido, 0, 0);
    if (medo ==0) {
        if(sentido ==1) pos_x += SPEED;
        else if(sentido ==2) pos_x -= SPEED;
        else if(sentido ==3) pos_y -= SPEED;
        else if(sentido ==4) pos_y += SPEED;
    }
    else
        if(sentido ==1) pos_x += SPEED/2;
        else if(sentido ==2) pos_x -= SPEED/2;
        else if(sentido ==3) pos_y -= SPEED/2;
        else if(sentido ==4) pos_y += SPEED/2;

    *x = pos_x; //atribui a variavel de volta no ponteiro
    *y = pos_y;
    *sent = sentido;
    *prox_sent = prox_sentido;

    if (pos_x >= player_x-2.0 && pos_x <= player_x+TILESIZE+2.0 && pos_y >= player_y-2 && pos_y <= player_y+TILESIZE+2){
            if(medo == 0)
                return 1;
            else {
                *x = 15*TILESIZE;
                *y = 15*TILESIZE+50.0;
                return 0;
            }
    }
    else return 0;

   }


void fecha_game() { exit_game = TRUE;}
END_OF_FUNCTION(fecha_game)

void tick_counter() {
    ticks++;
    cronometro++;}
END_OF_FUNCTION(tick_counter)



int intro()
{
    //allegro_init();
    //install_timer();
    //install_keyboard();
    //set_color_depth(32);
    set_gfx_mode(GFX_AUTODETECT_WINDOWED, 800, 600, 0, 0);
    set_window_title("SEA PAC");



    exit_game = FALSE;
    LOCK_FUNCTION(fecha_game);
    LOCK_VARIABLE(exit_game);
    set_close_button_callback(fecha_game);

    //int pro_game = FALSE;
    int time;
    int cont1 = 0;
    int cont2 = 460;
    int cont3 = 150;
    int cont4 = 230;
    int controle = 1;
    int entra = 0;

    //SONS
    SAMPLE* mus = load_sample("files/wii.wav");


    ///BITMAP
    BITMAP* buffer = create_bitmap(SCREEN_W, SCREEN_H);
    BITMAP* polva= load_bitmap("files/octos.bmp", NULL);
    BITMAP* polvo = load_bitmap("files/red_octopus.bmp", NULL);
    BITMAP* mar = load_bitmap("files/under.bmp", NULL);
    BITMAP* title = load_bitmap("files/text.bmp", NULL);
    BITMAP* nadador = load_bitmap("files/diver.bmp", NULL);
    BITMAP* bolha = load_bitmap("files/bubble.bmp", NULL);





while(!exit_game){

        if(key[KEY_ENTER]) {
            return 0;
            }
        if(key[KEY_ESC]){
            fecha_game();
            destroy_bitmap(polvo);
            destroy_bitmap(title);
            destroy_bitmap(polva);
            destroy_bitmap(nadador);
            destroy_bitmap(title);
            destroy_bitmap(mar);
            destroy_bitmap(buffer);
            destroy_sample(mus);

        }

       // while(!pro_game && entra==0)
        {

        //INPUT
            //if(key[KEY_ENTER]){
            //exit_game = TRUE;
            //entra=1;
            //play_sample(music, 255, 128, 1000, FALSE);
            }
        //UPDATE

        //DRAW

        draw_sprite(buffer,mar, 0, 0);
        draw_sprite(buffer,title, 110, 100);
        if(controle == 1){
            cont1++;
            cont2++;
            cont3++;
            cont4++;
            fun_fps();
            draw_sprite(buffer, polva, cont1, 320);
            draw_sprite(buffer, polvo, cont2, 200);
            draw_sprite(buffer, bolha, 390, cont3);
            draw_sprite(buffer, nadador, 280, cont4);

            if (cont1 > 70) controle = 0;
        }
        else{
            fun_fps();
            cont1--;
            cont2--;
            cont3--;
            cont4--;
            draw_sprite(buffer, polva, cont1, 320);
            draw_sprite(buffer, polvo, cont2, 200);
            draw_sprite(buffer, bolha, 390, cont3);
            draw_sprite(buffer, nadador, 280, cont4);
            textprintf_centre_ex(buffer, font, SCREEN_W/1.3, SCREEN_H/1.2, 0xffffff, -1, "PRESS ENTER");

            if (cont1 < 55) controle = 1;
        }

        draw_sprite(screen, buffer, 0, 0);
        clear(buffer);
        }

       /*/ if(entra==1){
        destroy_bitmap(polvo);
        destroy_bitmap(title);
        destroy_bitmap(polva);
        destroy_bitmap(nadador);
        destroy_bitmap(title);
        entra=2;
        }
*/

        //draw_sprite(buffer,mar, 0, 0);
        //draw_sprite(screen, buffer, 0, 0);
        clear(buffer);



    ///FINAL
    destroy_bitmap(polvo);
    destroy_bitmap(title);
    destroy_bitmap(polva);
    destroy_bitmap(nadador);
    destroy_bitmap(title);
    destroy_bitmap(mar);
    destroy_bitmap(buffer);
    //destroy_sample(mus);
//    destroy_sample(music);

    return 0;
}



int main()
    {

    allegro_init();
    install_timer();
    install_keyboard();
    set_color_depth(32);
    install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL);

    SAMPLE* mus = load_sample("files/wii.wav");

    play_sample(mus, 255, 128, 1000, TRUE);
    intro();

    set_gfx_mode(GFX_AUTODETECT_WINDOWED, 32*TILESIZE, 31*TILESIZE+100, 0, 0);
    set_window_title("Game");

    exit_game = FALSE;
    LOCK_FUNCTION(fecha_game);
    LOCK_VARIABLE(exit_game);
    set_close_button_callback(fecha_game);

    ticks = 0;
    cronometro = 0;
    LOCK_FUNCTION(tick_counter);
    LOCK_VARIABLE(ticks);
    install_int_ex(tick_counter, BPS_TO_TIMER(60)); //serve p fazer o jogo rodar constante em qlquer pc


    int vidas = 4;
    while(!exit_game) {
        int esta_vida =1;
        vidas--;    //comeca com 3 vidas

        ///BITMAPS
        BITMAP* buffer = create_bitmap(SCREEN_W, SCREEN_H);
        FONT* verdana = load_font("files/verdana.pcx", NULL, NULL);
        ///MAPA
        int linhas, colunas;
        int** mapa = Carregar_Mapa("files/mapa.txt", &linhas, &colunas);

        ///Variaveis
        int sentido=0, prox_sentido=0;
        int a, b;
        for (a=0; a<31; a++){
            for (b=0; b<28; b++){
                matriz_espelho[a][b] = -1;  //inicializa toda a matriz em -1.
            }
        }

        double pos_x = 15.0*TILESIZE+6.0, pos_y = 23.0*TILESIZE+50.0; //posicao inicial do jogador
        int pontuacao = 0, tempo = 0, medo = 0, medo_tempo =0;

        double fantasma_pos_x[4], fantasma_pos_y[4];
        int fantasma_sentido[4], fantasma_prox_sentido[4];     //sao 4 fantasmas
        for (a=0; a<4; a++) {
            fantasma_sentido[a] = 0;
            fantasma_prox_sentido[a] = 0;
        }
        fantasma_pos_x[0] = 13.0*TILESIZE;
        fantasma_pos_y[0] = 13.0*TILESIZE+50.0;

        fantasma_pos_x[1] = 13.0*TILESIZE;
        fantasma_pos_y[1] = 15.0*TILESIZE+50.0;

        fantasma_pos_x[2] = 18.0*TILESIZE;
        fantasma_pos_y[2] = 13.0*TILESIZE+50.0;

        fantasma_pos_x[3] = 18.0*TILESIZE;
        fantasma_pos_y[3] = 15.0*TILESIZE+50.0;
        srand (time(NULL)); // randomiza seed

        ///Game loop
        while (!exit_game && esta_vida ==1)
        {
            while (ticks > 0 && !exit_game && esta_vida ==1)
            {
                ticks=0; //60 vezes por segundo


                ///INPUT
                if(key[KEY_ESC])
                    fecha_game();
                definicao_sentido(&sentido, &prox_sentido, 1);


                ///UPDATE
                if (pontuacao>=246){    //checa se ganhou
                    rectfill(buffer, SCREEN_W/2-4*TILESIZE, SCREEN_H/2-1.5*TILESIZE, SCREEN_W/2+4*TILESIZE, SCREEN_H/2+1.5*TILESIZE, makecol(0,255,0));
                    textprintf_ex(buffer, font, SCREEN_W/2-3*TILESIZE, SCREEN_H/2-0.25*TILESIZE, makecol(0,0,255), -1, "Voce ganhou!");
                    draw_sprite(screen, buffer, 0, 0);
                    continue;
                }
                if (vidas==0) {     //testa se perdeu
                  rectfill(buffer, SCREEN_W/2-4*TILESIZE, SCREEN_H/2-1.5*TILESIZE, SCREEN_W/2+4*TILESIZE, SCREEN_H/2+1.5*TILESIZE, makecol(0,255,0));
                    textprintf_ex(buffer, font, SCREEN_W/2-3.5*TILESIZE, SCREEN_H/2-0.25*TILESIZE, makecol(0,0,255), -1, "Voce perdeu :(");
                    draw_sprite(screen, buffer, 0, 0);
                    continue;
                }


                if (medo==1) {
                    medo_tempo++;   //controla o tempo que os fantasmas terao medo
                    if (medo_tempo >= 60*8) {
                        medo =0;
                        medo_tempo =0;
                    }
                }

                pontuacao += colisoes(&pos_x, &pos_y, &sentido, &prox_sentido, &medo, 1);      //checa colisoes


                if(sentido ==1) pos_x += SPEED;
                else if(sentido ==2) pos_x -= SPEED;
                else if(sentido ==3) pos_y -= SPEED;
                else if(sentido ==4) pos_y += SPEED;

                if (cronometro == 60) {
                    cronometro = 0;
                    tempo++;
                }
                for (a=0; a<4; a++) {   //chama a funcao fantasmas para todos os 4 fantasmas e retorna 0 se morrer
                    esta_vida -= fantasmas(&fantasma_pos_x[a], &fantasma_pos_y[a], &fantasma_sentido[a], &fantasma_prox_sentido[a], pos_x, pos_y, medo);
                }

                ///DRAW
                Desenhar_Mapa(buffer, mapa, linhas, colunas);

                /*for (a=0; a<4; a++) {
                rectfill(buffer, (int)fantasma_pos_x[a], (int)fantasma_pos_y[a],
                 (int)fantasma_pos_x[a]+(TILESIZE-1), (int)fantasma_pos_y[a]+(TILESIZE-1), makecol(70*a,0,30*a));
                }*/

                rectfill(buffer, (int)fantasma_pos_x[0], (int)fantasma_pos_y[0],
                (int)fantasma_pos_x[0]+(TILESIZE-1), (int)fantasma_pos_y[0]+(TILESIZE-1), makecol(225,165,74));

                rectfill(buffer, (int)fantasma_pos_x[1], (int)fantasma_pos_y[1],
                (int)fantasma_pos_x[1]+(TILESIZE-1), (int)fantasma_pos_y[1]+(TILESIZE-1), makecol(245,54,207));

                rectfill(buffer, (int)fantasma_pos_x[2], (int)fantasma_pos_y[2],
                (int)fantasma_pos_x[2]+(TILESIZE-1), (int)fantasma_pos_y[2]+(TILESIZE-1), makecol(255,9,15));

                rectfill(buffer, (int)fantasma_pos_x[3], (int)fantasma_pos_y[3],
                (int)fantasma_pos_x[3]+(TILESIZE-1), (int)fantasma_pos_y[3]+(TILESIZE-1), makecol(12,216,252));

                rectfill(buffer, (int)pos_x, (int)pos_y, (int)pos_x+(TILESIZE-1), (int)pos_y+(TILESIZE-1), makecol(255,242,0));
                textprintf_right_ex(buffer, font, 31*TILESIZE, 1.7*TILESIZE, makecol(0,0,255), -1, "Score: %d", pontuacao*10);
                textprintf_ex(buffer, font, 3*TILESIZE, 1.7*TILESIZE, makecol(0,0,255), -1, "Tempo: %d", tempo);


                draw_sprite(screen, buffer, 0, 0);
                clear(buffer);

            }
        }



        ///Finalizacao
        Libera_Mapa(mapa, linhas);
        destroy_font(verdana);
        destroy_bitmap(buffer);

    }
    destroy_sample(mus);
    return 0;
    }
END_OF_MAIN();
