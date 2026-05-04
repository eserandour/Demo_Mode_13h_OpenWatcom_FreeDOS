#ifndef SCENE_H
#define SCENE_H

/* =========================================================
   SCENE.H — Gestionnaire de scènes
   =========================================================
   Une scène est une fonction autonome appelée en boucle
   par main(). Elle gère son propre état interne via des
   variables statiques et peut décider de passer à la
   scène suivante via setScene().

   Chaque scène suit le même patron :
   1. Initialisation (une seule fois, via un flag static).
   2. Mise à jour de l'état selon le temps écoulé.
   3. Dessin dans le backbuffer + flip().
   4. Transition vers la scène suivante si nécessaire.

   Pour ajouter une scène :
   1. Ajouter une valeur dans l'enum Scene.
   2. Écrire la fonction dans scenes/<nom>.c.
   3. L'ajouter dans le tableau scenes[] de scene.c.
   ========================================================= */

/* ---------------------------------------------------------
   Énumération des scènes
   --------------------------------------------------------- */

typedef enum {
    SCENE_1 = 0,   /* pixels aléatoires (LCG)   */
    SCENE_2 = 1,   /* démonstration palette VGA */
    SCENE_3 = 2    /* démonstration des polices */
} Scene;

/* ---------------------------------------------------------
   État global du gestionnaire
   --------------------------------------------------------- */

/* Scène actuellement active. Utilisée par scene.c pour
   appeler la bonne fonction dans le tableau scenes[]. */
extern Scene currentScene;

/* Valeur de timer_ticks au moment du dernier setScene().
   Permet à chaque scène de mesurer son temps écoulé
   via elapsedTimeMs(sceneStart, now). */
extern unsigned long sceneStart;

/* ---------------------------------------------------------
   Fonctions
   --------------------------------------------------------- */

/* Change la scène active et enregistre le timestamp de
   démarrage dans sceneStart. À appeler depuis une scène
   quand elle veut passer à la suivante. */
void setScene(Scene s);

/* Appelle la fonction correspondant à currentScene.
   C'est le dispatcher : main() n'a qu'à appeler cette
   fonction en boucle sans connaître les scènes. */
void runCurrentScene(void);

#endif /* SCENE_H */
