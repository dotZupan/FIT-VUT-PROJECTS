/*
 * Binární vyhledávací strom — rekurzivní varianta
 *
 * S využitím datových typů ze souboru btree.h a připravených koster funkcí
 * implementujte binární vyhledávací strom pomocí rekurze.
 */

#include "../btree.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * Inicializace stromu.
 *
 * Uživatel musí zajistit, že inicializace se nebude opakovaně volat nad
 * inicializovaným stromem. V opačném případě může dojít k úniku paměti (memory
 * leak). Protože neinicializovaný ukazatel má nedefinovanou hodnotu, není
 * možné toto detekovat ve funkci.
 */
void bst_init(bst_node_t **tree){
  *tree = NULL;
}

/*
 * Vyhledání uzlu v stromu.
 *
 * V případě úspěchu vrátí funkce hodnotu true a do proměnné value zapíše
 * ukazatel na obsah daného uzlu. V opačném případě funkce vrátí hodnotu false a proměnná
 * value zůstává nezměněná.
 *
 * Funkci implementujte rekurzivně bez použité vlastních pomocných funkcí.
 */
bool bst_search(bst_node_t *tree, char key, bst_node_content_t **value)
{
  if(tree == NULL){ // Skončí, ak je strom prázdny.
    return false;
  }
  char curr_key = tree->key;
  if(curr_key == key){ // Kľúč sa zhoduje, nastaví ukazateľ na hodnotu uzla.
    *value = &(tree->content);
    return true;
  }

  // Rekurzívne prehľadáva ľavý alebo pravý podstrom podľa hodnoty kľúča.
  else if(curr_key > key){
    return bst_search(tree->left, key, value);
  }
  else{
    return bst_search(tree->right, key, value);
  }
}

/*
 * Vložení uzlu do stromu.
 *
 * Pokud uzel se zadaným klíče už ve stromu existuje, nahraďte jeho hodnotu.
 * Jinak vložte nový listový uzel.
 *
 * Výsledný strom musí splňovat podmínku vyhledávacího stromu — levý podstrom
 * uzlu obsahuje jenom menší klíče, pravý větší.
 *
 * Funkci implementujte rekurzivně bez použití vlastních pomocných funkcí.
 */
void bst_insert(bst_node_t **tree, char key, bst_node_content_t value){
  if(*tree == NULL){ // Ak je strom prázdny, vytvorí nový uzol.
    bst_node_t *new_node = (bst_node_t *)malloc(sizeof(bst_node_t));
    if(new_node == NULL){
      return;
    }
    new_node->key = key;
    new_node->content = value;
    new_node->left = NULL;
    new_node->right = NULL;
    *tree = new_node;
    return;
  }
  
  char curr_key = (*tree)->key;
  if(curr_key == key){  // Pokiaľ uzol existuje, aktualizuje jeho hodnotu.
    free((*tree)->content.value);
    (*tree)->content = value;
    return;
  }
  // Inak rekurzívne hľadá správnu pozíciu v ľavom alebo pravom podstrome
  else if(curr_key > key){
    bst_insert(&(*tree)->left, key, value);
  }
  else if(curr_key < key){
    bst_insert(&(*tree)->right, key, value);
  }
}

/*
 * Pomocná funkce která nahradí uzel nejpravějším potomkem.
 *
 * Klíč a hodnota uzlu target budou nahrazeny klíčem a hodnotou nejpravějšího
 * uzlu podstromu tree. Nejpravější potomek bude odstraněný. Funkce korektně
 * uvolní všechny alokované zdroje odstraněného uzlu.
 *
 * Funkce předpokládá, že hodnota tree není NULL.
 *
 * Tato pomocná funkce bude využitá při implementaci funkce bst_delete.
 *
 * Funkci implementujte rekurzivně bez použití vlastních pomocných funkcí.
 */
void bst_replace_by_rightmost(bst_node_t *target, bst_node_t **tree){
  if((*tree)->right == NULL){ // Ak uzol nemá pravého potomka, nahradí cieľový uzol.
    target->key = (*tree)->key;
    free(target->content.value);
    target->content = (*tree)->content;
    bst_node_t *buff = *tree;
    *tree = (*tree)->left;
    free(buff);
    return;
  }
  // Rekurzívne hľadá najpravší uzol
  bst_replace_by_rightmost(target, &(*tree)->right);
}

/*
 * Odstranění uzlu ze stromu.
 *
 * Pokud uzel se zadaným klíčem neexistuje, funkce nic nedělá.
 * Pokud má odstraněný uzel jeden podstrom, zdědí ho rodič odstraněného uzlu.
 * Pokud má odstraněný uzel oba podstromy, je nahrazený nejpravějším uzlem
 * levého podstromu. Nejpravější uzel nemusí být listem.
 *
 * Funkce korektně uvolní všechny alokované zdroje odstraněného uzlu.
 *
 * Funkci implementujte rekurzivně pomocí bst_replace_by_rightmost a bez
 * použití vlastních pomocných funkcí.
 */
void bst_delete(bst_node_t **tree, char key){
  if(*tree == NULL){  //Ak je strom prázdny, nič sa nevykoná
    return;
  }
  if(key < (*tree)->key){
    bst_delete(&(*tree)->left, key);
  }
  else if(key > (*tree)->key){
    bst_delete(&(*tree)->right, key);
  }
  else{
    if((*tree)->left == NULL && (*tree)->right == NULL){  //AK uzol nemá deti, tak sa iba odstráni
      free((*tree)->content.value);
      free(*tree);
      *tree = NULL;
      return;
    }
    else if((*tree)->left != NULL && (*tree)->right == NULL){ //Pokiaľ má uzol ľavý podstrom, uzol sa nahradí ľavým podstromom
      bst_node_t *tmp = *tree;
      free((*tree)->content.value);
      *tree = (*tree)->left;
      free(tmp);
      return;
    }
    else if((*tree)->left == NULL && (*tree)->right != NULL){ //Pokiaľ má uzol pravý podstrom, uzol sa nahradí pravým podstromom
      bst_node_t *tmp = *tree;
      free((*tree)->content.value);
      *tree = (*tree)->right;
      free(tmp);
      return;
    }
    else{ // Ak má uzol oboch potomkov, nahradí ho najpravším uzlom ľavého podstromu.
      bst_replace_by_rightmost(*tree, &(*tree)->left);
      return;
    }
  }
}

/*
 * Zrušení celého stromu.
 *
 * Po zrušení se celý strom bude nacházet ve stejném stavu jako po
 * inicializaci. Funkce korektně uvolní všechny alokované zdroje rušených
 * uzlů.
 *
 * Funkci implementujte rekurzivně bez použití vlastních pomocných funkcí.
 */
void bst_dispose(bst_node_t **tree){
  if(*tree == NULL){
    return;
  }
  bst_dispose(&(*tree)->left);
  bst_dispose(&(*tree)->right);
  free((*tree)->content.value);
  free(*tree);
  *tree = NULL;
}

/*
 * Preorder průchod stromem.
 *
 * Pro aktuálně zpracovávaný uzel zavolejte funkci bst_add_node_to_items.
 *
 * Funkci implementujte rekurzivně bez použití vlastních pomocných funkcí.
 */
void bst_preorder(bst_node_t *tree, bst_items_t *items){
  if(tree == NULL){
      return;
    }

    bst_add_node_to_items(tree, items);
    bst_preorder(tree->left, items);
    bst_preorder(tree->right, items);
}

/*
 * Inorder průchod stromem.
 *
 * Pro aktuálně zpracovávaný uzel zavolejte funkci bst_add_node_to_items.
 *
 * Funkci implementujte rekurzivně bez použití vlastních pomocných funkcí.
 */
void bst_inorder(bst_node_t *tree, bst_items_t *items){
  if(tree == NULL){
      return;
    }

  bst_inorder(tree->left, items);
  bst_add_node_to_items(tree, items);
  bst_inorder(tree->right, items);
}

/*
 * Postorder průchod stromem.
 *
 * Pro aktuálně zpracovávaný uzel zavolejte funkci bst_add_node_to_items.
 *
 * Funkci implementujte rekurzivně bez použití vlastních pomocných funkcí.
 */
void bst_postorder(bst_node_t *tree, bst_items_t *items){
  if(tree == NULL){
      return;
    }
  bst_postorder(tree->left, items);
  bst_postorder(tree->right, items);
  bst_add_node_to_items(tree, items);

}
