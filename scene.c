/* =========================================================
   SCENE.C — Gestionnaire de scènes
   ========================================================= */

#include "timer.h"   /* readTimer                           */
#include "scene.h"

/* ---------------------------------------------------------
   Prototypes des fonctions de scène
   --------------------------------------------------------- */

/* Ces fonctions sont définies dans scenes/*.c mais on
   déclare leurs prototypes ici pour que le tableau scenes[]
   puisse y faire référence. On ne les met pas dans scene.h
   car elles ne font pas partie de l'API publique : seul
   le gestionnaire de scènes les appelle. */
void scene1(void);
void scene2(void);
void scene3(void);
void scene4(void);

/* =========================================================
   ÉTAT GLOBAL
   ========================================================= */

/* Scène courante : commence à SCENE_1. */
Scene currentScene = SCENE_1;

/* Timestamp de démarrage de la scène courante.
   Mis à jour à chaque appel de setScene(). */
unsigned long sceneStart = 0;

/* =========================================================
   TABLEAU DE DISPATCH
   =========================================================
   Un tableau de pointeurs de fonctions : chaque entrée
   correspond à une valeur de l'enum Scene.
   runCurrentScene() n'a qu'à appeler scenes[currentScene]()
   sans aucun switch/if.

   Avantages par rapport à un switch/case :
   - Ajouter une scène = une ligne ici + une valeur dans l'enum.
   - Pas de risque d'oublier un "break" ou un "case".
   - Le compilateur génère un accès indirect par index,
     plus rapide qu'une chaîne de comparaisons branchées.

   Contrainte : l'ordre des entrées doit correspondre
   exactement à l'ordre des valeurs dans l'enum Scene.
   ========================================================= */

typedef void (*SceneFunc)(void);   /* type pointeur de fonction */

static SceneFunc scenes[] = {
    scene1,  /* SCENE_1 = 0 */
    scene2,  /* SCENE_2 = 1 */
    scene3,  /* SCENE_3 = 2 */
    scene4,  /* SCENE_4 = 3 */
};

/* =========================================================
   API PUBLIQUE
   ========================================================= */

/* Change la scène active et enregistre l'instant de
   démarrage. sceneStart sera utilisé par les scènes pour
   calculer leur temps écoulé avec elapsedTimeMs(). */
void setScene(Scene s)
{
    currentScene = s;
    sceneStart   = readTimer();
}

/* Appelle la fonction de la scène courante.
   L'indexation du tableau par currentScene remplace un
   switch/case et reste valide sans modification si on
   ajoute des scènes en respectant l'ordre de l'enum. */
void runCurrentScene(void)
{
    scenes[currentScene]();
}
