#ifndef FONT_H
#define FONT_H

/* =========================================================
   FONT.H — Moteur de texte bitmap multi-tailles
   =========================================================
   API unifiée : toutes les fonctions de rendu acceptent
   un pointeur Font*, quelle que soit la source de la police
   (ROM BIOS ou FontBank personnelle).

   Utilisation :
     drawChar(x, y, 'A', 255, &FONT_BIOS);
     drawChar(x, y, 'A', 255, &FONT_8X8);
     drawChar(x, y, 'A', 255, &FONT_16X16);

   Pour ajouter une nouvelle police :
     1. Déclarer une FontBank dans fontdata.c.
     2. La charger avec _initFontBank() + defineChar*().
     3. Créer une Font pointant vers cette FontBank.

   Fonctionnement d'un glyphe :
   Chaque ligne est encodée en binaire : bit 1 = pixel allumé,
   bit 0 = transparent. Le MSB correspond au pixel le plus
   à gauche de la ligne.
   ========================================================= */

/* ---------------------------------------------------------
   Constantes
   --------------------------------------------------------- */

/* Octets par glyphe selon la taille :
   8x8   : 8  lignes × 1 octet  =   8 octets
   16x16 : 16 lignes × 2 octets =  32 octets */
#define FONT_8X8_GLYPH_BYTES   8
#define FONT_16X16_GLYPH_BYTES  32

/* ---------------------------------------------------------
   Macros CP850 — accents et caractères spéciaux français
   --------------------------------------------------------- */

/* Usage : drawText(x, y, "caract" eGRAVE "re", 255, &FONT_8X8);
   La concaténation de littéraux adjacents est résolue à la
   compilation, sans coût à l'exécution.

   Attention avec \xNN : si le caractère suivant est un chiffre
   hexadécimal (0-9, a-f, A-F), le compilateur l'incorpore dans
   l'échappement. Dans ce cas, couper la chaîne :
     "\x82" "e"  et non  "\x82e"  (lu comme un seul code 0x82E) */

/* Minuscules accentuées */
#define eAIGU   "\x82"   /* é  CP850 0x82 */
#define eGRAVE  "\x8A"   /* è  CP850 0x8A */
#define eCIRC   "\x88"   /* ê  CP850 0x88 */
#define eTREMA  "\x89"   /* ë  CP850 0x89 */
#define aGRAVE  "\x85"   /* à  CP850 0x85 */
#define aCIRC   "\x83"   /* â  CP850 0x83 */
#define aTREMA  "\x84"   /* ä  CP850 0x84 */
#define uGRAVE  "\x97"   /* ù  CP850 0x97 */
#define uCIRC   "\x96"   /* û  CP850 0x96 */
#define uTREMA  "\x81"   /* ü  CP850 0x81 */
#define iCIRC   "\x8C"   /* î  CP850 0x8C */
#define iTREMA  "\x8B"   /* ï  CP850 0x8B */
#define oCIRC   "\x93"   /* ô  CP850 0x93 */
#define oTREMA  "\x94"   /* ö  CP850 0x94 */
#define cCED    "\x87"   /* ç  CP850 0x87 */

/* Majuscules accentuées */
#define EAIGU   "\x90"   /* É  CP850 0x90 */
#define ATREMA  "\x8E"   /* Ä  CP850 0x8E */
#define OTREMA  "\x99"   /* Ö  CP850 0x99 */
#define UTREMA  "\x9A"   /* Ü  CP850 0x9A */
#define CCED    "\x80"   /* Ç  CP850 0x80 */

/* Nombre maximum de glyphes par FontBank.
   256 = ensemble complet des caractères (0 à 255). */
#define FONT_BANK_CAPACITY  256

/* ---------------------------------------------------------
   FontSize — tailles de glyphes supportées
   --------------------------------------------------------- */

/* La valeur numérique = taille en pixels d'un côté du
   glyphe (glyphe carré). Utilisée comme espacement entre
   deux caractères dans drawText(). */
typedef enum {
    FONT_SIZE_8X8  =  8,
    FONT_SIZE_16X16 = 16
} FontSize;

/* ---------------------------------------------------------
   FontBank — stockage des glyphes personnels
   --------------------------------------------------------- */

/* Contient tous les glyphes d'une police bitmap personnelle.
   - size            : taille des glyphes (8 ou 16)
   - count           : nombre de glyphes définis
   - capacity        : nombre max de glyphes (128)
   - bytes_per_glyph : taille en octets d'un glyphe
   - lut[256]        : lut[c] = index du glyphe pour le
                       caractère c, ou -1 si non défini
   - data[]          : tableau compact de tous les glyphes */
typedef struct {
    FontSize      size;
    int           count;
    int           capacity;
    int           bytes_per_glyph;
    int           lut[256];
    unsigned char data[FONT_BANK_CAPACITY * FONT_16X16_GLYPH_BYTES];
} FontBank;

/* ---------------------------------------------------------
   FontType — type de source de la police
   --------------------------------------------------------- */

typedef enum {
    FONT_TYPE_BIOS = 0,   /* police ROM BIOS 8x8            */
    FONT_TYPE_BANK = 1    /* police personnelle dans une FontBank */
} FontType;

/* ---------------------------------------------------------
   Font — structure unifiée (BIOS ou FontBank)
   --------------------------------------------------------- */

/* Structure passée à toutes les fonctions de rendu.
   type  : indique si la police est BIOS ou personnelle.
   bank  : pointeur far vers la FontBank (si FONT_TYPE_BANK).
           NULL si FONT_TYPE_BIOS.
   size  : taille en pixels d'un glyphe (espacement). */
typedef struct {
    FontType      type;
    FontBank far *bank;
    int           size;
} Font;

/* ---------------------------------------------------------
   Polices globales prêtes à l'emploi
   --------------------------------------------------------- */

/* Police BIOS ROM 8x8 (128 caractères IBM/ASCII). */
extern Font FONT_BIOS;

/* Polices peersonnelles initialisées par initMyFont*(). */
extern Font FONT_8X8;    /* myFont8x8  8x8  */
extern Font FONT_16X16;   /* myFont16x16 16x16 */

/* FontBank sous-jacentes allouées en far heap.
   Initialisées par initMyFont8x8/16x16(), NULL avant. */
extern FontBank far *myFont8x8;
extern FontBank far *myFont16x16;

/* ---------------------------------------------------------
   Initialisation
   --------------------------------------------------------- */

/* Charge biosFont depuis la ROM BIOS (F000:FA6E).
   À appeler avant tout drawChar avec FONT_BIOS. */
void initBiosFont(void);

/* Initialise myFont8x8 et charge ses glyphes depuis fontdata.c.
   Met à jour FONT_8X8 pour qu'elle pointe sur myFont8x8. */
void initMyFont8x8(void);

/* Initialise myFont16x16 et charge ses glyphes. */
void initMyFont16x16(void);

/* ---------------------------------------------------------
   Définition de glyphes
   --------------------------------------------------------- */

/* Ajoute ou remplace un glyphe 8x8 dans une FontBank.
   b0 = ligne du haut, b7 = ligne du bas.
   Bit 7 de chaque octet = pixel le plus à gauche. */
void defineChar8x8(FontBank far *fb, unsigned char c,
                 unsigned char b0, unsigned char b1,
                 unsigned char b2, unsigned char b3,
                 unsigned char b4, unsigned char b5,
                 unsigned char b6, unsigned char b7);

/* Ajoute ou remplace un glyphe 16x16.
   rows[16] : 16 unsigned int, bit 15 = pixel gauche. */
void defineChar16x16(FontBank far *fb, unsigned char c,
                  unsigned int rows[16]);

/* ---------------------------------------------------------
   Rendu — API unifiée
   --------------------------------------------------------- */

/* Dessine un caractère avec la police f.
   Fonctionne avec FONT_BIOS, FONT_8X8, FONT_16X16
   ou toute autre Font correctement initialisée. */
void drawChar(int x, int y, unsigned char c,
              unsigned char color, Font *f);

/* Dessine une chaîne de caractères avec la police f.
   L'espacement entre caractères = f->size pixels. */
void drawText(int x, int y, const char *str,
              unsigned char color, Font *f);

/* Dessine une chaîne centrée horizontalement avec la
   police f. */
void drawTextCentered(int y, const char *str,
                      unsigned char color, Font *f);

#endif /* FONT_H */
