/* =========================================================
   SCENE3.C — Scène : Démonstration des polices
   =========================================================
   Affiche successivement tous les caractères de chaque
   police disponible.

   Sous-phases :
   1. FONT_BIOS : 128 caractères ASCII/IBM (ROM BIOS)
   2. FONT_8    : 256 caractères custom 8x8
   3. FONT_16   : 256 caractères custom 16x16, sur 2 pages

   Navigation :
   - Phases 1 et 2 : n'importe quelle touche → phase suivante
   - Phase 3 (FONT_16) :
       Tab / Shift+Tab → page suivante / précédente (sans fade)
       Toute autre touche → fade-out vers SCENE_4

   Transitions entre phases 1→2 et 2→3 :
   - Fade-out 0.5 s, puis fade-in 0.5 s.

   Grilles :
     FONT_BIOS / FONT_8  : 16 col × 16 lignes, pas 10 px
     FONT_16 page 1      : car. 0-127,  16 col × 8 lignes, pas 18 px
     FONT_16 page 2      : car. 128-255, 16 col × 8 lignes, pas 18 px

   Optimisation : le backbuffer est construit une seule fois
   par (sous-phase, page), puis seule la palette évolue
   pendant les fades.
   ========================================================= */

#include <stdlib.h>   /* exit                              */
#include <conio.h>    /* kbhit, getch                      */
#include "timer.h"    /* readTimer, elapsedTimeMs          */
#include "video.h"    /* clearScreen, flip, SCREEN_*       */
#include "palette.h"  /* pinkPalette, fadePalette,
                         setPalette, generateBlackPalette  */
#include "graphics.h" /* clearScreen, drawLine             */
#include "font.h"     /* Font, FONT_BIOS, FONT_8/16,
                         drawChar, drawText                */
#include "scene.h"    /* setScene, SCENE_4                 */

/* Prototype de shutdown() défini dans main.c. */
void shutdown(void);

/* =========================================================
   CONSTANTES
   ========================================================= */

#define FADE_MS   500UL   /* durée du fade-in et fade-out  */
#define NB_PHASES 3       /* nombre de sous-phases         */
#define KEY_TAB   0x09    /* code ASCII de la touche Tab   */

/* =========================================================
   ÉTATS INTERNES
   ========================================================= */

typedef enum {
    STATE_FADEIN,    /* fondu entrant au début de la sous-phase */
    STATE_DISPLAY,   /* affichage en attente d'une touche       */
    STATE_FADEOUT    /* fondu sortant après appui touche        */
} PhaseState;

/* =========================================================
   DESSIN DES SOUS-PHASES
   ========================================================= */

/* Sous-phase 1 : FONT_BIOS 8x8
   Affiche les 128 caractères en grille 16x8.
   Chaque cellule = 10 px (8 px glyphe + 2 px marge).
   La ROM BIOS ne contient ici que 128 glyphes (0–127) : 
   on n'affiche pas au-delà. */
static void drawPhase1(void)
{
    int c, col, row;
    int startX = (SCREEN_WIDTH  - 16 * 10) / 2;
    int startY = (SCREEN_HEIGHT -  8 * 10) / 2 + 9;

    drawTextCentered(4, "biosFont (8x8) - 0..127", 255, &FONT_BIOS);
    drawLine(4, 15, 315, 15, 100);

    for (c = 0; c < 128; c++)
    {
        col = c % 16;
        row = c / 16;
        drawChar(startX + col * 10,
                 startY + row * 10,
                 (unsigned char)c, 255, &FONT_BIOS);
    }
}

/* Sous-phase 2 : FONT_8 (myFont8 8x8 custom)
   Grille 16×16, espacement 10 px.
   Les cases sans glyphe défini restent vides (trous voulus). */
static void drawPhase2(void)
{
    int c, col, row;
    int startX = (SCREEN_WIDTH  - 16 * 10) / 2;
    int startY = (SCREEN_HEIGHT - 16 * 10) / 2 + 9;

    drawTextCentered(4, "myFont8 (8x8) - 0..255", 255, &FONT_BIOS);
    drawLine(4, 15, 315, 15, 100);

    for (c = 0; c < 256; c++)
    {
        col = c % 16;
        row = c / 16;
        drawChar(startX + col * 10,
                 startY + row * 10,
                 (unsigned char)c, 255, &FONT_8);
    }
}

/* Sous-phase 3 : FONT_16 (myFont16 16x16 custom), 2 pages.
   Chaque page affiche 128 caractères en grille 16×8.
   Espacement 18 px (16 px glyphe + 2 px marge).
   Hauteur grille : 8 × 18 = 144 px — tient dans 200 px.
   page = 0 : car. 0-127 / page = 1 : car. 128-255.

   Le titre indique la page courante et les touches actives. */
static void drawPhase3(int page)
{
    int c, col, row;
    int base   = page * 128;           /* premier code de la page   */
    int startX = (SCREEN_WIDTH  - 16 * 18) / 2;
    int startY = (SCREEN_HEIGHT -  8 * 18) / 2 + 9;

    if (page == 0)
        drawTextCentered(4, "myFont16 (16x16) - 0..127  [Tab>]", 255, &FONT_BIOS);
    else
        drawTextCentered(4, "myFont16 (16x16) - 128..255 [<Tab]", 255, &FONT_BIOS);
    drawLine(4, 15, 315, 15, 100);

    for (c = 0; c < 128; c++)
    {
        col = c % 16;
        row = c / 16;
        drawChar(startX + col * 18,
                 startY + row * 18,
                 (unsigned char)(base + c), 255, &FONT_16);
    }
}

/* =========================================================
   RENDU D'UNE SOUS-PHASE (dispatch)
   ========================================================= */

/* Construit le backbuffer selon la phase et (pour phase 3)
   la page courante, puis appelle flip(). */
static void buildFrame(int phase, int page16)
{
    clearScreen(0);
    switch (phase)
    {
        case 0: drawPhase1();       break;
        case 1: drawPhase2();       break;
        case 2: drawPhase3(page16); break;
    }
    flip();
}

/* =========================================================
   SCÈNE PRINCIPALE
   ========================================================= */

void scene3(void)
{
    /* Variables statiques : persistent entre les appels.
       initialized = 0 force un rebuild du backbuffer au
       prochain appel (après setScene ou changement de page). */
    static int           phase       = 0;
    static int           page16      = 0;   /* page active en phase 3 */
    static int           initialized = 0;
    static PhaseState    state       = STATE_FADEIN;
    static unsigned long fadeStart   = 0;

    unsigned long now = readTimer();
    unsigned long elapsed;
    float         t;
    int           key;

    /* -------------------------------------------------------
       Initialisation : construction du backbuffer et fade-in.
       ------------------------------------------------------- */
    if (!initialized)
    {
        initialized = 1;
        state       = STATE_FADEIN;
        fadeStart   = now;

        generateBlackPalette(workingPalette);
        setPalette(workingPalette);
        buildFrame(phase, page16);
    }

    /* -------------------------------------------------------
       Machine à états commune aux phases 1 et 2.
       La phase 3 a sa propre gestion du clavier en DISPLAY.
       ------------------------------------------------------- */
    switch (state)
    {
        /* Fondu entrant : luminosité 0 → 1 sur FADE_MS ms. */
        case STATE_FADEIN:
            elapsed = elapsedTimeMs(fadeStart, now);
            t = (float)elapsed / (float)FADE_MS;
            if (t > 1.0f) t = 1.0f;
            fadePalette(pinkPalette, t);
            if (elapsed >= FADE_MS)
                state = STATE_DISPLAY;
            break;

        /* Attente d'une touche. */
        case STATE_DISPLAY:
            setPalette(pinkPalette);
            if (!kbhit()) break;

            key = getch();

            if (phase < 2)
            {
                /* Phases 1 et 2 : toute touche → phase suivante. */
                state     = STATE_FADEOUT;
                fadeStart = now;
            }
            else
            {
                /* Phase 3 (FONT_16) : Tab bascule la page,
                   toute autre touche passe à SCENE_4. */
                if (key == KEY_TAB)
                {
                    /* Basculer la page sans fade : on éteint
                       la palette, redessine, puis rallume. */
                    page16 ^= 1;
                    generateBlackPalette(workingPalette);
                    setPalette(workingPalette);
                    buildFrame(phase, page16);
                    /* Relancer un fade-in pour la nouvelle page. */
                    state     = STATE_FADEIN;
                    fadeStart = readTimer();
                }
                else
                {
                    /* Toute autre touche → fade-out vers SCENE_4. */
                    state     = STATE_FADEOUT;
                    fadeStart = now;
                }
            }
            break;

        /* Fondu sortant : luminosité 1 → 0 sur FADE_MS ms. */
        case STATE_FADEOUT:
            elapsed = elapsedTimeMs(fadeStart, now);
            t = 1.0f - (float)elapsed / (float)FADE_MS;
            if (t < 0.0f) t = 0.0f;
            fadePalette(pinkPalette, t);
            if (elapsed >= FADE_MS)
            {
                initialized = 0;
                phase++;
                if (phase >= NB_PHASES)
                {
                    /* Fin de la scène 3 : réinitialiser l'état
                       statique pour une éventuelle relecture,
                       vider le buffer clavier, puis passer à
                       SCENE_4. */
                    phase  = 0;
                    page16 = 0;
                    while (kbhit()) getch();
                    setScene(SCENE_4);
                }
            }
            break;
    }
}
