#ifndef VIDEO_H
#define VIDEO_H

/* =========================================================
   VIDEO.H — Mode 13h, backbuffer, curseur
   =========================================================
   Le mode vidéo 13h est le mode graphique VGA le plus
   simple : 320x200 pixels, 256 couleurs, 1 octet par pixel.
   La VRAM (Video RAM) commence au segment 0xA000, offset 0.
   Un pixel à la position (x, y) se trouve à l'adresse :
     0xA000:0000 + y * 320 + x

   Double buffering :
   On n'écrit jamais directement en VRAM pour éviter le
   scintillement (tearing). À la place on dessine dans un
   buffer en RAM (le backbuffer), puis on copie tout d'un
   coup en VRAM avec flip(), idéalement pendant le retrace
   vertical.
   ========================================================= */

/* ---------------------------------------------------------
   Dimensions de l'écran en mode 13h
   --------------------------------------------------------- */
#define SCREEN_WIDTH    320   /* largeur en pixels           */
#define SCREEN_HEIGHT   200   /* hauteur en pixels           */

/* Taille totale du buffer : 320 * 200 = 64000 octets.
   UL = Unsigned Long pour éviter un débordement 16 bits
   lors des calculs d'adresse (64000 > 32767). */
#define BACKBUFFER_SIZE 64000UL

/* ---------------------------------------------------------
   Adresse de la VRAM VGA
   --------------------------------------------------------- */

/* Segment de la VRAM en mode 13h (adresse physique 0xA0000).
   En mode réel 16 bits : adresse = segment * 16 + offset. */
#define VGA_SEG  0xA000

/* Offset de départ dans la VRAM (premier pixel en haut
   à gauche). Toujours 0 en mode 13h. */
#define VGA_OFF  0x0000

/* ---------------------------------------------------------
   Macro de calcul d'offset dans le backbuffer
   --------------------------------------------------------- */

/* Convertit des coordonnées (x, y) en offset linéaire dans
   le backbuffer (ou la VRAM).

   En mode 13h, les pixels sont stockés ligne par ligne :
     ligne 0 : pixels 0..319
     ligne 1 : pixels 320..639
     ...
   Donc le pixel (x, y) se trouve à l'offset : y * 320 + x

   Optimisation par décomposition de la multiplication :
     y * 320 = y * (256 + 64)    ← 256 + 64 = 320 exactement
             = y * 256 + y * 64
             = (y << 8) + (y << 6)
   Les décalages binaires remplacent une multiplication coûteuse
   par deux décalages bon marché sur 8086. */
#define OFFSET(x, y)  (((y) << 8) + ((y) << 6) + (x))

/* ---------------------------------------------------------
   Backbuffer global
   --------------------------------------------------------- */

/* Pointeur far vers le backbuffer alloué en mémoire étendue.
   far = pointeur 32 bits (segment:offset) nécessaire pour
   accéder à plus de 64 Ko en modèle mémoire large. */
extern unsigned char far *backbuffer;

/* ---------------------------------------------------------
   Fonctions — Backbuffer
   --------------------------------------------------------- */

/* Alloue le backbuffer (64000 octets) avec _fmalloc.
   Retourne 1 si succès, 0 si échec (mémoire insuffisante).
   À appeler AVANT setVideoMode(). */
int  initBackbuffer(void);

/* Libère la mémoire du backbuffer.
   Met le pointeur à NULL pour éviter un accès invalide. */
void freeBackbuffer(void);

/* Copie le backbuffer vers la VRAM (affichage à l'écran).
   Utilise movedata() qui gère les segments far correctement.
   À appeler après avoir fini de dessiner dans le backbuffer. */
void flip(void);

/* ---------------------------------------------------------
   Fonctions — Mode vidéo
   --------------------------------------------------------- */

/* Change le mode vidéo via l'interruption BIOS 10h.
   Modes courants : 0x03 = texte 80x25, 0x13 = graphique. */
void setVideoMode(unsigned char mode);

/* Cache le curseur texte, pour le mode 0x03 */
void cursorOff(void);

/* Réaffiche le curseur texte, pour le mode 0x03 */
void cursorOn(void);

/* Attend la synchronisation verticale du moniteur.
   Le port 0x3DA indique l'état du signal VGA :
   bit 3 = 1 pendant le retrace vertical.
   Attendre le retrace avant flip() évite le palette
   tearing (déchirure d'image visible lors des changements
   de palette). */
void waitVRetrace(void);

#endif /* VIDEO_H */
