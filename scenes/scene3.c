/* =========================================================
   SCENE3.C — Scène : Démonstration des polices
   =========================================================
   Affiche successivement tous les caractères de chaque
   police disponible. L'utilisateur passe à la police
   suivante en appuyant sur n'importe quelle touche.

   Sous-phases :
   1. FONT_BIOS : 128 caractères ASCII/IBM (ROM BIOS)
   2. FONT_8    : les caractères définis dans fontdata.c
   3. FONT_16   : les caractères définis dans fontdata.c

   Sur appui touche :
   - Fade-out 0.5 s
   - Passage à la sous-phase suivante
   - Fade-in 0.5 s
   Après la 3e sous-phase : shutdown() + exit().

   Optimisation : le backbuffer de chaque sous-phase est
   construit une seule fois, puis seule la palette change
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
                         myFont8/16, drawChar, drawText */
#include "scene.h"    /* sceneStart                        */

/* Prototype de shutdown() défini dans main.c. */
void shutdown(void);

/* =========================================================
   CONSTANTES
   ========================================================= */

#define FADE_MS   500UL   /* durée du fade-in et fade-out  */
#define NB_PHASES 3       /* nombre de sous-phases         */

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
   Affiche les 128 caractères en grille 32x8.
   Chaque cellule = 9 px (8 px glyphe + 1 px espace).
   On accède directement à FONT_BIOS sans passer par la LUT
   car tous les 128 caractères sont toujours définis en ROM. */
static void drawPhase1(void)
{
    int c, col, row;
    int startX = (SCREEN_WIDTH  - 16 * 10) / 2;
    int startY = (SCREEN_HEIGHT - 8  * 10) / 2 + 9;

    drawTextCentered(4, "FONT BIOS 8x8 - 128 caracteres", 255, &FONT_BIOS);
    drawLine(4, 15, 315, 15, 100);

    for (c = 0; c < 128; c++)
    {
        col = c % 16;
        row = c / 16;
        drawChar(startX + col * 10,
                 startY + row * 10,
                 (unsigned char)c, 255, &FONT_BIOS);
    }
    drawTextCentered(187, "pressez une touche du clavier", 255, &FONT_BIOS);
}

/* Sous-phase 2 : FONT_8 (myFont8 8x8 custom)
   N'affiche que les caractères définis (lut[c] != -1).
   On accède à myFont8.lut pour tester chaque entrée.
   Grille de 16 colonnes, espacement 10 px. */
static void drawPhase2(void)
{
    int c, col = 0, row = 0;
    int startX = (SCREEN_WIDTH - 16 * 10) / 2;
    int startY = (SCREEN_HEIGHT - 8  * 10) / 2 + 9;

    drawTextCentered(4, "FONT 8x8 - caracteres personnalises", 255, &FONT_BIOS);
    drawLine(4, 15, 315, 15, 100);

    for (c = 0; c < 128; c++)
    {
        /* Ce filtre est intentionnellement désactivé.
           Sans lui, drawChar() est appelé pour tous les c de 0 à 127,
           y compris ceux absents de la FontBank. drawChar() les ignore
           silencieusement (slot < 0 → return), ce qui laisse des
           "trous" dans la grille aux positions non définies.
           C'est un choix de présentation pour la démo : la grille
           complète 16×8 est plus lisible qu'une grille partielle. */
        // if (myFont8.lut[c] == -1) continue;
        
        col = c % 16;
        row = c / 16;
        drawChar(startX + col * 10,
                 startY + row * 10,
                 (unsigned char)c, 255, &FONT_8);
    }
    drawTextCentered(187, "pressez une touche du clavier", 255, &FONT_BIOS);
}

/* Sous-phase 3 : FONT_16 (myFont16 16x16 custom)
   Grille de 16 colonnes, espacement 18 px. */
   
static void drawPhase3(void)
{
    int c, col = 0, row = 0;
    int startX = (SCREEN_WIDTH - 16 * 18) / 2;
    int startY = (SCREEN_HEIGHT - 8  * 18) / 2 + 9;

    drawTextCentered(4, "FONT 16x16 - caracteres personnalises", 255, &FONT_BIOS);
    drawLine(4, 15, 315, 15, 100);

    for (c = 0; c < 128; c++)
    {
        /* Même logique que drawPhase2 : filtre désactivé
           pour afficher la grille complète avec les trous
           aux positions non définies dans la FontBank. */
        // if (myFont16.lut[c] == -1) continue;

        drawChar(startX + col * 18,
                 startY + row * 18,
                 (unsigned char)c, 255, &FONT_16);
        col++;
        if (col >= 16) { col = 0; row++; }
    }
    drawTextCentered(187, "pressez une touche du clavier", 255, &FONT_BIOS);
}

/* =========================================================
   SCÈNE PRINCIPALE
   ========================================================= */

void scene3(void)
{
    static int           phase       = 0;
    static int           initialized = 0;
    static PhaseState    state       = STATE_FADEIN;
    static unsigned long fadeStart   = 0;

    unsigned long now     = readTimer();
    unsigned long elapsed;
    float t;

    /* -------------------------------------------------------
       Initialisation de la sous-phase courante
       ------------------------------------------------------- */
    if (!initialized)
    {
        initialized = 1;
        state       = STATE_FADEIN;
        fadeStart   = now;

        generateBlackPalette(workingPalette);
        setPalette(workingPalette);
        clearScreen(0);

        switch (phase)
        {
            case 0: drawPhase1(); break;
            case 1: drawPhase2(); break;
            case 2: drawPhase3(); break;
        }

        flip();
    }

    /* -------------------------------------------------------
       Machine à états : FADEIN → DISPLAY → FADEOUT
       ------------------------------------------------------- */
    switch (state)
    {
        /* Fondu entrant : luminosité 0 → 1 pendant FADE_MS. */
        case STATE_FADEIN:
            elapsed = elapsedTimeMs(fadeStart, now);
            t = (float)elapsed / (float)FADE_MS;
            if (t > 1.0f) t = 1.0f;
            fadePalette(pinkPalette, t);
            if (elapsed >= FADE_MS)
                state = STATE_DISPLAY;
            break;

        /* Attente d'une touche : palette stable. */
        case STATE_DISPLAY:
            setPalette(pinkPalette);
            if (kbhit())
            {
                getch();
                state     = STATE_FADEOUT;
                fadeStart = now;
            }
            break;

        /* Fondu sortant : luminosité 1 → 0 pendant FADE_MS. */
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
                    phase       = 0;
                    initialized = 0;
                    /* Vider le buffer clavier : la touche qui a déclenché
                       le fade-out a été consommée par getch(), mais une
                       éventuelle touche résiduelle déclencherait le
                       kbhit() de main et couperait la boucle avant que
                       scene4 ne soit jamais appelée. */
                    while (kbhit()) getch();
                    setScene(SCENE_4);
                }
            }
            break;
    }
}
