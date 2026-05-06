/* =========================================================
   FONT.C — Moteur de rendu texte bitmap
   =========================================================
   API unifiée : drawChar, drawText, drawTextCentered
   acceptent un Font* qui encapsule indifféremment la police
   BIOS ROM ou une FontBank custom.

   Les données brutes des glyphes sont dans fontdata.c.
   ========================================================= */

#include <stdlib.h>    /* exit    */
#include <string.h>    /* memset  */
#include <malloc.h>    /* _fmalloc, _ffree */
#include <dos.h>       /* MK_FP  */
#include "video.h"     /* SCREEN_WIDTH */
#include "graphics.h"  /* putPixel */
#include "font.h"
#include "fontdata.h"  /* _initFont8D, _initFont16D */

/* =========================================================
   VARIABLES GLOBALES
   ========================================================= */

/* Pointeur vers la table de glyphes 8x8 en ROM BIOS.
   Adresse fixe F000:FA6E sur tous les PC IBM compatibles.
   far = pointeur 32 bits (segment:offset). */
static unsigned char far *biosFont = NULL;

/* FontBank sous-jacentes, allouées en far heap par initMyFont*(). */
FontBank far *myFont8  = NULL;
FontBank far *myFont16 = NULL;

/* Structures Font globales prêtes à l'emploi.
   Initialisées par initBiosFont() et initMyFont*().
   FONT_BIOS.bank = NULL car la police vient de la ROM. */
Font FONT_BIOS = { FONT_TYPE_BIOS, NULL, 8  };
Font FONT_8    = { FONT_TYPE_BANK, NULL, 8  };
Font FONT_16   = { FONT_TYPE_BANK, NULL, 16 };

/* =========================================================
   INITIALISATION INTERNE
   ========================================================= */

/* Prépare une FontBank vide pour la taille donnée.
   Met tous les slots LUT à -1 (aucun glyphe défini)
   et efface le tableau data[]. */
static void _initFontBank(FontBank far *fb, FontSize size)
{
    int i, bpg;

    switch (size)
    {
        case FONT_SIZE_8:  bpg = FONT8_GLYPH_BYTES;  break;
        case FONT_SIZE_16: bpg = FONT16_GLYPH_BYTES; break;
        default:           bpg = FONT8_GLYPH_BYTES;  break;
    }

    fb->size            = size;
    fb->count           = 0;
    fb->capacity        = FONT_BANK_CAPACITY;
    fb->bytes_per_glyph = bpg;

    for (i = 0; i < 256; i++) fb->lut[i] = -1;
    _fmemset(fb->data, 0, sizeof(fb->data));
}

/* =========================================================
   DÉFINITION DE GLYPHES
   ========================================================= */

/* Ajoute ou remplace un glyphe 8x8.
   Alloue un nouveau slot si le caractère n'existe pas encore
   dans la FontBank, sinon écrase l'existant. */
void defineChar8(FontBank far *fb, unsigned char c,
                 unsigned char b0, unsigned char b1,
                 unsigned char b2, unsigned char b3,
                 unsigned char b4, unsigned char b5,
                 unsigned char b6, unsigned char b7)
{
    int slot;
    unsigned char far *g;

    if (fb->lut[c] == -1)
    {
        if (fb->count >= fb->capacity) return;
        fb->lut[c] = fb->count++;
    }
    slot = fb->lut[c];
    g    = fb->data + slot * fb->bytes_per_glyph;
    g[0]=b0; g[1]=b1; g[2]=b2; g[3]=b3;
    g[4]=b4; g[5]=b5; g[6]=b6; g[7]=b7;
}

/* Ajoute ou remplace un glyphe 16x16.
   Stockage big-endian : octet haut en premier pour que
   le rendu lise les bits de gauche à droite. */
void defineChar16(FontBank far *fb, unsigned char c,
                  unsigned int rows[16])
{
    int slot, i;
    unsigned char far *g;

    if (fb->lut[c] == -1)
    {
        if (fb->count >= fb->capacity) return;
        fb->lut[c] = fb->count++;
    }
    slot = fb->lut[c];
    g    = fb->data + slot * fb->bytes_per_glyph;
    for (i = 0; i < 16; i++)
    {
        g[i*2]   = (unsigned char)(rows[i] >> 8);
        g[i*2+1] = (unsigned char)(rows[i] & 0xFF);
    }
}

/* =========================================================
   RENDU INTERNE
   ========================================================= */

/* Rend un glyphe 8x8.
   Pour chaque ligne, lit l'octet de bits et teste chaque
   bit avec un masque décalé de 0x80 (gauche) à 0x01 (droite).
   far pointer : nécessaire pour la ROM BIOS et par cohérence
   avec les FontBank en mémoire far. */
static void _renderGlyph8(int x, int y, unsigned char color,
                          unsigned char far *glyph)
{
    int row, col;
    unsigned char bits;

    for (row = 0; row < 8; row++)
    {
        bits = glyph[row];
        for (col = 0; col < 8; col++)
            if (bits & (0x80 >> col))
                putPixel(x+col, y+row, color);
    }
}

/* Rend un glyphe 16x16.
   Reconstitue chaque ligne en unsigned int depuis 2 octets
   big-endian, puis teste les 16 bits avec masque 0x8000→0x0001. */
static void _renderGlyph16(int x, int y, unsigned char color,
                           unsigned char far *glyph)
{
    int row, col;
    unsigned int bits;

    for (row = 0; row < 16; row++)
    {
        bits = ((unsigned int)glyph[row*2] << 8) | glyph[row*2+1];
        for (col = 0; col < 16; col++)
            if (bits & (0x8000 >> col))
                putPixel(x+col, y+row, color);
    }
}

/* =========================================================
   RENDU PUBLIC — API UNIFIÉE
   ========================================================= */

/* Dessine un caractère CP437 avec la police f.
   c est un code CP437 direct (0x00–0xFF).
   Pour FONT_TYPE_BIOS, le code est passé tel quel à la ROM.
   Pour FONT_TYPE_BANK, le code indexe directement la LUT. */
void drawChar(int x, int y, unsigned char c,
              unsigned char color, Font *f)
{
    unsigned char far *glyph;
    unsigned char far *g;

    if (f->type == FONT_TYPE_BIOS)
    {
        /* Police BIOS : table ROM à F000:FA6E, 128 glyphes (0–127).
           Les codes >= 128 sont hors table : on les remplace par '?'. */
        if (c >= 128) c = '?';
        glyph = biosFont + ((unsigned int)c * 8);
        _renderGlyph8(x, y, color, glyph);
    }
    else
    {
        /* Police custom : chercher le glyphe CP437 dans la FontBank. */
        int slot = f->bank->lut[(unsigned int)c];
        if (slot < 0) return;   /* caractère non défini */

        g = f->bank->data + slot * f->bank->bytes_per_glyph;

        switch (f->bank->size)
        {
            case FONT_SIZE_8:
                _renderGlyph8(x, y, color, g);
                break;
            case FONT_SIZE_16:
                _renderGlyph16(x, y, color, g);
                break;
        }
    }
}

/* Dessine une chaîne CP437 avec la police f.
   Chaque octet de str est un code CP437 direct.
   L'espacement entre caractères = f->size pixels. */
void drawText(int x, int y, const char *str,
              unsigned char color, Font *f)
{
    int i    = 0;
    int step = f->size;

    while (str[i] != '\0')
    {
        drawChar(x + i * step, y, (unsigned char)str[i], color, f);
        i++;
    }
}

/* Dessine une chaîne CP437 centrée horizontalement.
   Largeur totale = longueur en octets * f->size pixels. */
void drawTextCentered(int y, const char *str,
                      unsigned char color, Font *f)
{
    int len = 0, x;
    while (str[len] != '\0') len++;
    x = (SCREEN_WIDTH - len * f->size) / 2;
    if (x < 0) x = 0;
    drawText(x, y, str, color, f);
}

/* =========================================================
   INITIALISATION
   ========================================================= */

/* Charge biosFont depuis la ROM BIOS.
   MK_FP(segment, offset) construit un far pointer 32 bits.
   F000:FA6E est l'adresse fixe de la table de polices 8x8
   dans le BIOS de tous les PC IBM compatibles. */
void initBiosFont(void)
{
    biosFont = (unsigned char far *)MK_FP(0xF000, 0xFA6E);
    /* FONT_BIOS est déjà initialisée statiquement,
       pas besoin de modifier ses champs ici. */
}

/* Alloue myFont8 en far heap, initialise la FontBank,
   charge ses glyphes depuis fontdata.c,
   et met à jour FONT_8 pour qu'elle pointe dessus. */
void initMyFont8(void)
{
    myFont8 = (FontBank far *)_fmalloc(sizeof(FontBank));
    if (!myFont8) { setVideoMode(0x03); exit(1); }
    _initFontBank(myFont8, FONT_SIZE_8);
    _initFont8D();
    FONT_8.bank = myFont8;
}

/* Alloue myFont16 en far heap et met à jour FONT_16. */
void initMyFont16(void)
{
    myFont16 = (FontBank far *)_fmalloc(sizeof(FontBank));
    if (!myFont16) { setVideoMode(0x03); exit(1); }
    _initFontBank(myFont16, FONT_SIZE_16);
    _initFont16D();
    FONT_16.bank = myFont16;
}
