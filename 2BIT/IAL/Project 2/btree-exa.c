/*
 * Použití binárních vyhledávacích stromů.
 *
 * S využitím Vámi implementovaného binárního vyhledávacího stromu (soubory ../iter/btree.c a ../rec/btree.c)
 * implementujte triviální funkci letter_count. Všimněte si, že výstupní strom může být značně degradovaný 
 * (až na úroveň lineárního seznamu). Jako typ hodnoty v uzlu stromu využijte 'INTEGER'.
 * 
 */

#include "../btree.h"
#include <stdio.h>
#include <stdlib.h>


/**
 * Vypočítání frekvence výskytů znaků ve vstupním řetězci.
 * 
 * Funkce inicilializuje strom a následně zjistí počet výskytů znaků a-z (case insensitive), znaku 
 * mezery ' ', a ostatních znaků (ve stromu reprezentováno znakem podtržítka '_'). Výstup je 
 * uložen ve stromu (klíč vždy lowercase).
 * 
 * Například pro vstupní řetězec: "abBccc_ 123 *" bude strom po běhu funkce obsahovat:
 * 
 * key | value
 * 'a'     1
 * 'b'     2
 * 'c'     3
 * ' '     2
 * '_'     5
 * 
 * Pro implementaci si můžete v tomto souboru nadefinovat vlastní pomocné funkce.
*/
void letter_count(bst_node_t **tree, char *input) {
    bst_init(tree); // Inicializácia stromu
    char curr_key;
    while(*input != '\0'){  // Iterácia cez každý prvok v reťazci
        if((*input >= 'a' && *input <= 'z') || *input == ' '){  // Pokiaľ je aktuaĺny znak malé písmeno alebo medzera, priradíme ho priamo
            curr_key = *input;
        }
        else if(*input >= 'A' && *input <= 'Z'){    // Ak je aktuálny znak veľké písmeno prevedieme ho na malé písmeno
            curr_key = *input + ('a' - 'A');
        }
        else{   // všetky ostatné znaky nahradíme '_'
            curr_key = '_';
        }

        bst_node_content_t *content;
        if(bst_search(*tree, curr_key, &content)){  // Ak uzol so znakom existuje zvýšime jeho hodnotu o 1
            int *count = (int *)(content->value);
            (*count)++;

        }
        else{   // vytvoríme nový uzol pre aktuálny znak
            bst_node_content_t new_cont;
            new_cont.type = INTEGER;
            new_cont.value = malloc(sizeof(int));
            if(new_cont.value == NULL){
                return;
            }
            
            *(int  *)new_cont.value = 1;
            bst_insert(tree, curr_key, new_cont);   // Vložíme uzol do stromu
            
            
        }
        input++;    // Prejdeme na ďalší znak
    }
}