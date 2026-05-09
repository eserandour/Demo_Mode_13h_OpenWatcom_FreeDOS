#ifndef IMAGE_H
#define IMAGE_H

/* =========================================================
   IMAGE.H — Chargement d'images RAW/PAL en mode 13h
   =========================================================
   Environnement : Open Watcom 1.9, FreeDOS 1.4
   Mode video    : 13h (320x200, 256 couleurs, 1 octet/pixel)

   FORMATS DE FICHIERS
   -------------------
   .raw  Pixels bruts, un octet par pixel = index palette.
         Stockage ligne par ligne, gauche->droite, haut->bas.
         Dimensions libres (pas forcément 320x200).
         Produit par img2vga.py.

   .pal  768 octets = 256 entrees x 3 composantes R, G, B.
         Chaque composante sur 6 bits (0-63), format natif
         du DAC VGA. Produit par img2vga.py.

   PRINCIPE GENERAL
   ----------------
   Toutes les fonctions de pixels ecrivent dans le backbuffer
   global (defini dans video.h). Rien n'est visible a l'ecran
   tant que flip() n'est pas appele.

   TRANSPARENCE (colorKey)
   -----------------------
   Les fonctions suffixees Key acceptent un parametre colorKey :
     colorKey <  0  : copie opaque, tous les pixels ecrits.
     colorKey >= 0  : les pixels dont l'index == colorKey
                      ne sont pas ecrits (transparence par
                      cle de couleur, comme le chroma-key).

   FLUX TYPIQUES
   -------------
   Image plein ecran 320x200 avec sa palette :
       drawScreen("img.pal", "img.raw");
       flip();

   Sprite isole, opaque, a une position precise :
       loadImagePal("sprites.pal");
       loadImageRaw("sprite.raw", 16, 16, 100, 80);
       flip();

   Sprite extrait d'une feuille, avec transparence :
       loadImagePal("sheet.pal");
       loadImageZoneRawKey("sheet.raw", 256,
                           32, 0, 16, 16,
                           100, 80, 0);
       flip();
   ========================================================= */


/* =========================================================
   CODES DE RETOUR
   ========================================================= */

#define IMG_OK       0   /* succes                          */
#define IMG_ERR_PAL  1   /* impossible d'ouvrir le .pal     */
#define IMG_ERR_RAW  2   /* impossible d'ouvrir le .raw     */
#define IMG_ERR_READ 3   /* lecture incomplete (fichier trop
                            court ou erreur disque)         */


/* =========================================================
   PALETTE
   ========================================================= */

/* Charge le fichier .pal dans workingPalette et envoie
   immediatement la palette au DAC VGA.
   Ne touche pas au backbuffer.
   Retourne IMG_OK ou IMG_ERR_PAL / IMG_ERR_READ. */
int loadImagePal(const char *palFile);


/* =========================================================
   PIXELS -> BACKBUFFER  (image entiere)
   ========================================================= */

/* Lit le .raw de dimensions srcW x srcH et le copie dans
   le backbuffer, coin superieur gauche en (dstX, dstY).
   Le reste du backbuffer n'est pas modifie.
   Contraintes : dstX + srcW <= 320, dstY + srcH <= 200.
   Retourne IMG_OK ou IMG_ERR_RAW / IMG_ERR_READ. */
int loadImageRaw(const char *rawFile,
                 int srcW, int srcH,
                 int dstX, int dstY);

/* Identique a loadImageRaw avec gestion de la transparence.
   colorKey <  0 : copie opaque (equivalent a loadImageRaw).
   colorKey >= 0 : pixels d'index colorKey non ecrits.
   Retourne IMG_OK ou IMG_ERR_RAW / IMG_ERR_READ. */
int loadImageRawKey(const char *rawFile,
                    int srcW, int srcH,
                    int dstX, int dstY,
                    int colorKey);

/* Raccourci : charge un .raw 320x200 en (0,0).
   Equivalent a loadImageRaw(rawFile, 320, 200, 0, 0).
   Retourne IMG_OK ou IMG_ERR_RAW / IMG_ERR_READ. */
int loadScreenRaw(const char *rawFile);


/* =========================================================
   PIXELS -> BACKBUFFER  (zone d'une feuille de sprites)
   ========================================================= */

/* Extrait le rectangle (srcX, srcY, zoneW, zoneH) depuis
   un .raw de largeur imgW et le copie dans le backbuffer
   en (dstX, dstY).

   Parametres :
     rawFile      chemin du fichier .raw source
     imgW         largeur totale du .raw en pixels
                  (stride source, en octets par ligne)
     srcX, srcY   coin superieur gauche de la zone a lire
     zoneW, zoneH dimensions de la zone a extraire
     dstX, dstY   coin superieur gauche dans le backbuffer

   Contraintes :
     srcX + zoneW <= imgW
     dstX + zoneW <= 320
     dstY + zoneH <= 200

   Retourne IMG_OK ou IMG_ERR_RAW / IMG_ERR_READ. */
int loadImageZoneRaw(const char *rawFile,
                     int imgW,
                     int srcX, int srcY,
                     int zoneW, int zoneH,
                     int dstX, int dstY);

/* Identique a loadImageZoneRaw avec gestion de la
   transparence.
   colorKey <  0 : copie opaque (equivalent a
                   loadImageZoneRaw).
   colorKey >= 0 : pixels d'index colorKey non ecrits.
   Retourne IMG_OK ou IMG_ERR_RAW / IMG_ERR_READ. */
int loadImageZoneRawKey(const char *rawFile,
                        int imgW,
                        int srcX, int srcY,
                        int zoneW, int zoneH,
                        int dstX, int dstY,
                        int colorKey);


/* =========================================================
   TOUT-EN-UN  (palette + pixels en un seul appel)
   ========================================================= */

/* Charge la palette puis copie le .raw srcW x srcH
   en (dstX, dstY) dans le backbuffer.
   Retourne IMG_OK ou un code IMG_ERR_*. */
int drawImage(const char *palFile, const char *rawFile,
              int srcW, int srcH,
              int dstX, int dstY);

/* Charge la palette puis copie le .raw 320x200 en (0,0).
   Raccourci pour afficher une image plein ecran.
   Retourne IMG_OK ou un code IMG_ERR_*. */
int drawScreen(const char *palFile, const char *rawFile);

#endif /* IMAGE_H */
