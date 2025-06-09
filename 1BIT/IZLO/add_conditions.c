#include <stddef.h>
#include "cnf.h"

//
// LOGIN: <xfignam00>
//

// Tato funkce by mela do formule pridat klauzule predstavujici podminku 1)
// Křižovatky jsou reprezentovany cisly 0, 1, ..., num_of_crossroads-1
// Cislo num_of_streets predstavuje pocet ulic a proto i pocet kroku cesty
// Pole streets ma velikost num_of_streets a obsahuje vsechny existujuci ulice
//    - pro 0 <= i < num_of_streets predstavuje streets[i] jednu existujici
//      ulici od krizovatky streets[i].crossroad_from ke krizovatce streets[i].crossroad_to
void at_least_one_valid_street_for_each_step(CNF* formula, unsigned num_of_crossroads, unsigned num_of_streets, const Street* streets) {
    assert(formula != NULL);
    assert(num_of_crossroads > 0);
    assert(num_of_streets > 0);
    assert(streets != NULL);

    for(unsigned i = 0; i < num_of_streets; ++i){
        //for every step i
        Clause* cl = create_new_clause(formula);
        for(unsigned l = 0; l < num_of_streets; l++){
            //put every possible combination of existing streets for each step into clause 
            add_literal_to_clause(cl, true, i, streets[l].crossroad_from, streets[l].crossroad_to);
        }  
    }
    // ZDE PRIDAT KOD
}

// Tato funkce by mela do formule pridat klauzule predstavujici podminku 2)
// Křižovatky jsou reprezentovany cisly 0, 1, ..., num_of_crossroads-1
// Cislo num_of_streets predstavuje pocet ulic a proto i pocet kroku cesty
void at_most_one_street_for_each_step(CNF* formula, unsigned num_of_crossroads, unsigned num_of_streets) {
    assert(formula != NULL);
    assert(num_of_crossroads > 0);
    assert(num_of_streets > 0);

    // ZDE PRIDAT KOD
    for(unsigned i = 0; i < num_of_streets; ++i){
        //for every step i
        for(unsigned z = 0; z < num_of_crossroads; ++z){
            for(unsigned k = 0; k < num_of_crossroads; ++k){
                //for every combination of crossroads z,k
                for(unsigned zn = 0; zn < num_of_crossroads; ++zn){
                    for(unsigned kn = 0; kn < num_of_crossroads; ++kn){
                        //for every another combination of crossroads zn,kn
                        if(k != kn || zn != z){
                            //if there are two different combinations of crossroads
                            Clause* cl = create_new_clause(formula);
                            add_literal_to_clause(cl, false, i, z, k);
                            add_literal_to_clause(cl, false, i, zn, kn);
                           
                        }
                    }
                }
            }
        }        
    }
}
    



// Tato funkce by mela do formule pridat klauzule predstavujici podminku 3)
// Křižovatky jsou reprezentovany cisly 0, 1, ..., num_of_crossroads-1
// Cislo num_of_streets predstavuje pocet ulic a proto i pocet kroku cesty
void streets_connected(CNF* formula, unsigned num_of_crossroads, unsigned num_of_streets) {
    assert(formula != NULL);
    assert(num_of_crossroads > 0);
    assert(num_of_streets > 0);

    // ZDE PRIDAT KOD
    for(unsigned i = 0; i < num_of_streets; ++i){
        //for every step i
        for(unsigned z = 0; z < num_of_crossroads; ++z){
            for(unsigned k = 0; k < num_of_crossroads; ++k){
                //for every combinations of crossroads z,k
                Clause* cl = create_new_clause(formula);
                add_literal_to_clause(cl, false, i, z, k);
                for(unsigned kn = 0; kn < num_of_crossroads; ++kn){
                    //for every next combinations of crossroads k,kn    
                    add_literal_to_clause(cl, true, i + 1, k, kn);
                }
            }
        }
    }
    
}

// Tato funkce by mela do formule pridat klauzule predstavujici podminku 4)
// Křižovatky jsou reprezentovany cisly 0, 1, ..., num_of_crossroads-1
// Cislo num_of_streets predstavuje pocet ulic a proto i pocet kroku cesty
void streets_do_not_repeat(CNF* formula, unsigned num_of_crossroads, unsigned num_of_streets) {
    assert(formula != NULL);
    assert(num_of_crossroads > 0);
    assert(num_of_streets > 0);
    
    for (unsigned i = 0; i < num_of_streets; ++i) {
        // pro kazdy krok i
        for (unsigned j = 0; j < num_of_streets; ++j) {
            if (i != j) {
                // pro kazdy jiny krok j
                for (unsigned z = 0; z < num_of_crossroads; ++z) {
                    for (unsigned k = 0; k < num_of_crossroads; ++k) {
                        // pro kazdu dvojici krizovatek (z, k)
                        Clause* cl = create_new_clause(formula);
                        add_literal_to_clause(cl, false, i, z, k);
                        add_literal_to_clause(cl, false, j, z, k);
                    }
                }
            }
        }
    }
}
