/*
 * Tabulka s rozptýlenými položkami
 *
 * S využitím datových typů ze souboru hashtable.h a připravených koster
 * funkcí implementujte tabulku s rozptýlenými položkami s explicitně
 * zretězenými synonymy.
 *
 * Při implementaci uvažujte velikost tabulky HT_SIZE.
 */

#include "hashtable.h"
#include <stdlib.h>
#include <string.h>

int HT_SIZE = MAX_HT_SIZE;

/*
 * Rozptylovací funkce která přidělí zadanému klíči index z intervalu
 * <0,HT_SIZE-1>. Ideální rozptylovací funkce by měla rozprostírat klíče
 * rovnoměrně po všech indexech. Zamyslete sa nad kvalitou zvolené funkce.
 */
int get_hash(char *key) {
  int result = 1;
  int length = strlen(key);
  for (int i = 0; i < length; i++) {
    result += key[i];
  }
  return (result % HT_SIZE);
}

/*
 * Inicializace tabulky — zavolá sa před prvním použitím tabulky.
 */
void ht_init(ht_table_t *table) {
  for(int i = 0; i <= HT_SIZE; i++){
    (*table)[i] = NULL;
  }
}

/*
 * Vyhledání prvku v tabulce.
 *
 * V případě úspěchu vrací ukazatel na nalezený prvek; v opačném případě vrací
 * hodnotu NULL.
 */
ht_item_t *ht_search(ht_table_t *table, char *key) {
  ht_item_t *curr_item = (*table)[get_hash(key)];
  while(curr_item){
    if(strcmp(curr_item->key, key) == 0){ //porovnaj aktualny kluc, s hľadaným
      return curr_item;
    }
    curr_item = curr_item->next;
  }
  return NULL; //v prípade že hľadaný kľúč sa v tabuľke nenachádza
}

/*
 * Vložení nového prvku do tabulky.
 *
 * Pokud prvek s daným klíčem už v tabulce existuje, nahraďte jeho hodnotu.
 *
 * Při implementaci využijte funkci ht_search. Pri vkládání prvku do seznamu
 * synonym zvolte nejefektivnější možnost a vložte prvek na začátek seznamu.
 */
void ht_insert(ht_table_t *table, char *key, float value) {
  ht_item_t *item = ht_search(table, key); 

  if (item != NULL) { //pokiaľ sa daný kľúč nachádza v tabuľke tak prepíš value
    item->value = value;
  } else { //keď nie tak vytvor nový prvok
    ht_item_t *new_item = malloc(sizeof(struct ht_item));
    if (new_item == NULL) {
      return; // Alokácia zlyhala
    }
    
    new_item->key = key;
    new_item->value = value;
    new_item->next = (*table)[get_hash(key)];
    (*table)[get_hash(key)] = new_item;
  }
}


/*
 * Získání hodnoty z tabulky.
 *
 * V případě úspěchu vrací funkce ukazatel na hodnotu prvku, v opačném
 * případě hodnotu NULL.
 *
 * Při implementaci využijte funkci ht_search.
 */
float *ht_get(ht_table_t *table, char *key) {
  ht_item_t *item = ht_search(table, key);
  if(item == NULL){
    return NULL;
  }
  else{
    return &(item->value);
  }
}

/*
 * Smazání prvku z tabulky.
 *
 * Funkce korektně uvolní všechny alokované zdroje přiřazené k danému prvku.
 * Pokud prvek neexistuje, funkce nedělá nic.
 *
 * Při implementaci NEPOUŽÍVEJTE funkci ht_search.
 */
void ht_delete(ht_table_t *table, char *key) {
  int hash = get_hash(key);
  ht_item_t *item = (* table)[hash];
  ht_item_t *p_item = NULL;
  while(item){   //iteruj kým item sa nerovná NULL
    if(strcmp(item->key, key) == 0){
      if(p_item){   //pokiaľ mazaný prvok má predchádzajúci prvok, zebpeč správne zreťazenie
        p_item->next = item->next;
      }
      else{   //keď nie tak urč nasledujúci prvok ako prvý
        (* table)[hash] = item->next;
      }
      free(item);
      return;
    }
    p_item = item;
    item = item->next;
  }
}

/*
 * Smazání všech prvků z tabulky.
 *
 * Funkce korektně uvolní všechny alokované zdroje a uvede tabulku do stavu po 
 * inicializaci.
 */
void ht_delete_all(ht_table_t *table) {
  for(int i = 0; i < HT_SIZE; i++){
    ht_item_t *curr_item = (* table)[i];
    while (curr_item){
      ht_item_t *tmp = curr_item;
      curr_item = curr_item->next;
      free(tmp);
    }
    (* table)[i] = NULL;
  }
}
