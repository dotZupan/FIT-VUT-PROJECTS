/*
 * Binární vyhledávací strom — iterativní varianta
 *
 * S využitím datových typů ze souboru btree.h, zásobníku ze souboru stack.h
 * a připravených koster funkcí implementujte binární vyhledávací
 * strom bez použití rekurze.
 */

#include "../btree.h"
#include "stack.h"
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
 * Funkci implementujte iterativně bez použité vlastních pomocných funkcí.
 */
bool bst_search(bst_node_t *tree, char key, bst_node_content_t **value){
  while(tree){
    if(tree->key == key){ //Našli sme hľadaný kľúč, nastavíme hodnotu ukazateľa
      *value = &(tree->content);
      return true;
    }
    // Pokiaľ sme nenašli pokračujeme v pokračujeme do ľavého alebo pravého podstromu
    else if(tree->key < key){
      tree = tree->right;
    }
    else{
      tree = tree->left;
    }
  }
  return false; // Kľúč sa v podstrome nenašiel
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
 * Funkci implementujte iterativně bez použití vlastních pomocných funkcí.
 */
void bst_insert(bst_node_t **tree, char key, bst_node_content_t value){
  bst_node_t *new_node = (bst_node_t *)malloc(sizeof(bst_node_t));
  if(new_node == NULL){
    return;
  }
  new_node->key = key;
  new_node->content = value;
  new_node->left = NULL;
  new_node->right = NULL;


  if(*tree == NULL){
    *tree = new_node; //vložíme nový uzol ako koreň ak je strom prázdny
    return;
  }
  bst_node_t *curr = *tree;

  while(1){

    if(curr->key == key){ // Kľúč existuje nahradíme jeho obsah
      free(curr->content.value);
      curr->content = value;
      free(new_node);
      return;
    }

    else if(curr->key > key){
      if(curr->left != NULL){
        curr = curr->left;  // Pokračujeme na ľavého potomka
      }
      else{
        curr->left = new_node;  // Vložíme uzol na ľavého potomka
        return;
      }
    }

    else{
      if(curr->right != NULL){
        curr = curr->right; // Pokračujeme na pravého potomka
      }
      else{
        curr->right = new_node; // Vložíme uzol na pravého potomka
        return;
      }
    }
  }
}

/*
 * Pomocná funkce která nahradí uzel nejpravějším potomkem.
 *
 * Klíč a hodnota uzlu target budou nahrazené klíčem a hodnotou nejpravějšího
 * uzlu podstromu tree. Nejpravější potomek bude odstraněný. Funkce korektně
 * uvolní všechny alokované zdroje odstraněného uzlu.
 *
 * Funkce předpokládá, že hodnota tree není NULL.
 *
 * Tato pomocná funkce bude využita při implementaci funkce bst_delete.
 *
 * Funkci implementujte iterativně bez použití vlastních pomocných funkcí.
 */
void bst_replace_by_rightmost(bst_node_t *target, bst_node_t **tree){
  if(*tree == NULL){  //  Strom je prázdny
    return; 
  }

  bst_node_t *curr = *tree;
  bst_node_t *pnode = NULL;

  while(1){
    if(curr->right != NULL){
      pnode = curr;
      curr = curr->right; // Pokračujeme k najpravejšiemu uzlu
    }
    else{
      target->key = curr->key;  // Nastavíme hodnoty z najpravejšieho uzla na cieľový uzol
      free(target->content.value);
      target->content = curr->content;
      if(pnode == NULL){
        *tree = curr->left; // Ak najpravejší uzol nemá rodiča, nahradíme ho ľavým podstromom
      }
      else{
        pnode->right = curr->left;  // Pripojíme ľavého potomka k rodičovi
      }
      
      free(curr); // Odstarníme uzol
      return;
    }
  }
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
 * Funkci implementujte iterativně pomocí bst_replace_by_rightmost a bez
 * použití vlastních pomocných funkcí.
 */
void bst_delete(bst_node_t **tree, char key){
  if(*tree == NULL){
    return;
  }

  bst_node_t *curr = *tree;
  bst_node_t *pnode = NULL;

  while(1){
    if(curr == NULL){ // Kľúč nebol nájdený
      return;
    }

    if(key == curr->key){
      if(curr->left == NULL && curr->right == NULL){
        
        if(pnode->left->key == key){
          pnode->left = NULL; // Uzol je ľavý potomok bez detí
        }
        else if(pnode->right->key == key){
          pnode->right = NULL;  // Uzol je pravý potomok bez detí
        }
        else{
          *tree = NULL; // Uzol je koreň bez detí
        }
        free(curr->content.value);
        free(curr);
      }

      else if(curr->left != NULL && curr->right == NULL){
        bst_node_t *tmp = curr->left;
        
        if(pnode->left->key == key){
          pnode->left = tmp;    // Uzol má iba ľavého potomka, spojíme ho s rodičom
          
        }
        else if(pnode->right->key == key){
          pnode->right = tmp; // Uzol má iba pravého potomka, spojíme ho s rodičom
        }
        else{
          *tree = NULL;  // Ak je uzol koreňom, nahradíme ho ľavým potomkom
        }
        free(curr->content.value);
        free(curr);
      }

      else if(curr->left == NULL && curr->right != NULL){
        bst_node_t *tmp = curr->right;
        
        if(pnode->left->key == key){
          pnode->left = tmp;
          free(curr->content.value);
          free(curr);
        }
        else if(pnode->right->key == key){
          pnode->right = tmp;
          free(curr->content.value);
          free(curr);
        }
        else{
          free(curr->content.value);
          free(curr);
          *tree = NULL;
        }
      }
      else{ // Uzol má oboch potomkov, nahradíme ho najpravším uzlom ľavého podstromu
        bst_replace_by_rightmost(curr, &(curr)->left);
      }

    break;
    }

    else if(key < curr->key){
      pnode = curr;
      curr = curr->left;  // Pokračujeme doľava
    }

    else{
      pnode = curr;
      curr = curr->right; // Pokračujeme doprava
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
 * Funkci implementujte iterativně s pomocí zásobníku a bez použití
 * vlastních pomocných funkcí.
 */
void bst_dispose(bst_node_t **tree){
  if(*tree == NULL){
    return;
  }
  stack_bst_t *stack = (stack_bst_t *)malloc(sizeof(stack_bst_t));
  if(stack == NULL){
    return;
  }
  stack_bst_init(stack);

  stack_bst_push(stack, *tree);
  while(!stack_bst_empty(stack)){
    bst_node_t *curr = stack_bst_pop(stack);
    if(curr == NULL){
      continue;
    }

    if(curr->left != NULL){
      stack_bst_push(stack, curr->left);  // Pridáme ľavého potomka na zásobník
    }
    if(curr->right != NULL){
      stack_bst_push(stack, curr->right); // Pridáme pravého potomka na zásobník
    }
    free(curr->content.value);
    free(curr); // Uvoľníme aktúalny uzol
  }
  *tree = NULL; 
  free(stack);
}

/*
 * Pomocná funkce pro iterativní preorder.
 *
 * Prochází po levé větvi k nejlevějšímu uzlu podstromu.
 * Nad zpracovanými uzly zavolá bst_add_node_to_items a uloží je do zásobníku uzlů.
 *
 * Funkci implementujte iterativně s pomocí zásobníku a bez použití
 * vlastních pomocných funkcí.
 */
void bst_leftmost_preorder(bst_node_t *tree, stack_bst_t *to_visit, bst_items_t *items){
  bst_node_t *curr = tree;
  while(curr){
    bst_add_node_to_items(curr, items); // Spracujeme aktuálny uzol
    stack_bst_push(to_visit, curr);     // Pridáme uzol na zásobník
    curr = curr->left;   // Pokračujeme na ľavého potomka
  }
}

/*
 * Preorder průchod stromem.
 *
 * Pro aktuálně zpracovávaný uzel zavolejte funkci bst_add_node_to_items.
 *
 * Funkci implementujte iterativně pomocí funkce bst_leftmost_preorder a
 * zásobníku uzlů a bez použití vlastních pomocných funkcí.
 */
void bst_preorder(bst_node_t *tree, bst_items_t *items){
  if(tree == NULL){
    return;
  }

  stack_bst_t *stack = (stack_bst_t *)malloc(sizeof(stack_bst_t));
  if(stack == NULL){
    return;
  }
  stack_bst_init(stack);
  bst_node_t *curr = tree;
  bst_leftmost_preorder(curr, stack, items);
  
  while(!stack_bst_empty(stack)){
    curr = stack_bst_pop(stack);
    if(curr->right != NULL){
      bst_leftmost_preorder(curr->right, stack, items); // Spracujeme pravého potomka
    }
  }
  free(stack);
}

/*
 * Pomocná funkce pro iterativní inorder.
 *
 * Prochází po levé větvi k nejlevějšímu uzlu podstromu a ukládá uzly do
 * zásobníku uzlů.
 *
 * Funkci implementujte iterativně s pomocí zásobníku a bez použití
 * vlastních pomocných funkcí.
 */
void bst_leftmost_inorder(bst_node_t *tree, stack_bst_t *to_visit){
  bst_node_t *curr = tree;
  while(curr){
    stack_bst_push(to_visit, curr); // Pridáme uzol na zásobník
    curr = curr->left;  // Pokračujeme na ľavého potomka
  }
}

/*
 * Inorder průchod stromem.
 *
 * Pro aktuálně zpracovávaný uzel zavolejte funkci bst_add_node_to_items.
 *
 * Funkci implementujte iterativně pomocí funkce bst_leftmost_inorder a
 * zásobníku uzlů a bez použití vlastních pomocných funkcí.
 */
void bst_inorder(bst_node_t *tree, bst_items_t *items){
  if(tree == NULL){
    return;
  }

  stack_bst_t *stack = (stack_bst_t *)malloc(sizeof(stack_bst_t));
  if(stack == NULL){
    return;
  }
  stack_bst_init(stack);
  bst_node_t *curr = tree;
  bst_leftmost_inorder(curr, stack);
  
  while(!stack_bst_empty(stack)){
    curr = stack_bst_pop(stack);
    bst_add_node_to_items(curr, items); // Spracujeme aktuálny uzol
    if(curr->right != NULL){
      bst_leftmost_inorder(curr->right, stack); // Prejdeme na pravého potomka
    }
  }
  free(stack);
}

/*
 * Pomocná funkce pro iterativní postorder.
 *
 * Prochází po levé větvi k nejlevějšímu uzlu podstromu a ukládá uzly do
 * zásobníku uzlů. Do zásobníku bool hodnot ukládá informaci, že uzel
 * byl navštíven poprvé.
 *
 * Funkci implementujte iterativně pomocí zásobníku uzlů a bool hodnot a bez použití
 * vlastních pomocných funkcí.
 */
void bst_leftmost_postorder(bst_node_t *tree, stack_bst_t *to_visit, stack_bool_t *first_visit){
  bst_node_t *curr = tree;
  while(curr){
    stack_bst_push(to_visit, curr); // Pridáme uzol na zásobník
    stack_bool_push(first_visit, true); // Zaznamenáme prvú návštevu uzla
    curr = curr->left;  // Pokračujeme na ľavého potomka
  }
}

/*
 * Postorder průchod stromem.
 *
 * Pro aktuálně zpracovávaný uzel zavolejte funkci bst_add_node_to_items.
 *
 * Funkci implementujte iterativně pomocí funkce bst_leftmost_postorder a
 * zásobníku uzlů a bool hodnot a bez použití vlastních pomocných funkcí.
 */
void bst_postorder(bst_node_t *tree, bst_items_t *items){
  if(tree == NULL){
    return;
  }

  stack_bst_t *stack = (stack_bst_t *)malloc(sizeof(stack_bst_t));
  if(stack == NULL){
    return;
  }
  stack_bst_init(stack);

  stack_bool_t *bool_stack = (stack_bool_t *)malloc(sizeof(stack_bool_t));
  if(bool_stack == NULL){
    return;
  }
  stack_bool_init(bool_stack);

  bst_node_t *curr = tree;
  bst_leftmost_postorder(curr, stack, bool_stack);
  while(!stack_bst_empty(stack)){
    curr = stack_bst_pop(stack);
    bool first = stack_bool_pop(bool_stack);
    if(first){
      stack_bst_push(stack, curr);
      stack_bool_push(bool_stack, false);  // Označíme uzol ako druhýkrát navštívený
      if(curr->right != NULL){
        bst_leftmost_postorder(curr->right, stack, bool_stack);
      }
    }
    else{
      bst_add_node_to_items(curr, items);   // Spracujeme uzol po oboch podstromoch
    }
  }
  free(stack);
  free(bool_stack);
}
