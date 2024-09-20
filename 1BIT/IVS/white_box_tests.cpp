//======= Copyright (c) 2024, FIT VUT Brno, All rights reserved. ============//
//
// Purpose:     White Box - test suite
//
// $NoKeywords: $ivs_project_1 $white_box_tests.cpp
// $Author:     Matúš Fignár <xfignam00@stud.fit.vutbr.cz>
// $Date:       $2024-02-14
//============================================================================//
/**
 * @file white_box_tests.cpp
 * @author Matúš Fignár
 * 
 * @brief Implementace testu hasovaci tabulky.
 */

#include <vector>

#include "gtest/gtest.h"

#include "white_box_code.h"

//============================================================================//
// ** ZDE DOPLNTE TESTY **
//
// Zde doplnte testy hasovaci tabulky, testujte nasledujici:
// 1. Verejne rozhrani hasovaci tabulky
//     - Vsechny funkce z white_box_code.h
//     - Chovani techto metod testuje pro prazdnou i neprazdnou tabulku.
// 2. Chovani tabulky v hranicnich pripadech
//     - Otestujte chovani pri kolizich ruznych klicu se stejnym hashem 
//     - Otestujte chovani pri kolizich hashu namapovane na stejne misto v 
//       indexu
//============================================================================//


class HashMapTests : public ::testing::Test{
    void SetUp() override{
        map = hash_map_ctor();
        ASSERT_NE(map, nullptr);
    }

    void TearDown() override{
        hash_map_dtor(map);
        EXPECT_EQ(map->allocated, 0);
    }
    
    protected:
        hash_map_t* map;
};



TEST_F(HashMapTests, put_single){
    ASSERT_EQ(hash_map_put(map, "aloha", 1), OK);
    EXPECT_EQ(hash_map_contains(map, "aloha"), true);
    EXPECT_EQ(hash_map_put(map, "aloha", 1), KEY_ALREADY_EXISTS);
}

TEST_F(HashMapTests, put_many){
    std::string key;
    auto size = map->allocated;
    for(int i = 0; i < size; i++){
        key = "aloha" + std::to_string(i);
        ASSERT_EQ(hash_map_put(map, key.c_str(), i), OK);
        EXPECT_EQ(hash_map_contains(map, key.c_str()), true);
    }
    EXPECT_EQ(map->allocated, 16);
}


TEST_F(HashMapTests, get_existing){
    hash_map_put(map, "aloha", 13);
    int value;
    ASSERT_EQ(hash_map_get(map, "aloha", &value), OK);
    EXPECT_EQ(13, value);
}

TEST_F(HashMapTests, get_notexisting){
    hash_map_put(map, "aloha", 13);
    int value;
    ASSERT_EQ(hash_map_get(map, "key", &value), KEY_ERROR);
    EXPECT_NE(value, 13);
}

TEST_F(HashMapTests, get_existing_last){
    hash_map_put(map, "aloha1", 13);
    hash_map_put(map, "aloha2", 14);
    hash_map_put(map, "aloha3", 72);
    int value;
    ASSERT_EQ(hash_map_pop(map, "aloha3", &value), OK);
    EXPECT_EQ(72, value);
    EXPECT_EQ(hash_map_contains(map, "aloha3"), false);
}

TEST_F(HashMapTests, get_collisions){
    hash_map_put(map, "cba", 1);
    hash_map_put(map, "abc", 2);
     hash_map_put(map, "bac", 3);
    hash_map_put(map, "cab", 4);
    hash_map_put(map, "acb", 5);
    hash_map_put(map, "bca", 6);
    int value;
    EXPECT_EQ(hash_map_get(map,"abc", &value), OK);
    EXPECT_EQ(value, 2);
    EXPECT_EQ(hash_map_get(map,"bac", &value), OK);
    EXPECT_EQ(value, 3);
    EXPECT_EQ(hash_map_get(map,"bca", &value), OK);
    EXPECT_EQ(value, 6);
}


TEST_F(HashMapTests, reserve_less){
    hash_map_put(map, "aloha1", 1);
    hash_map_put(map, "aloha2", 2);
    EXPECT_EQ(hash_map_reserve(map, map->used-1), VALUE_ERROR);
}

TEST_F(HashMapTests, reserve_same){
    hash_map_put(map, "aloha1", 1);
    hash_map_put(map, "aloha2", 2);
    EXPECT_EQ(hash_map_reserve(map, map->used), OK);
}

TEST_F(HashMapTests, reserve_more){
    hash_map_put(map, "aloha1", 1);
    hash_map_put(map, "aloha2", 2);
    EXPECT_EQ(hash_map_reserve(map, map->used+1), OK);
}

TEST_F(HashMapTests, reserve_sys_fault){
    EXPECT_EQ(hash_map_reserve(map, SIZE_MAX), MEMORY_ERROR);
}

TEST_F(HashMapTests, reserve_sys_fault_nonempty){
    hash_map_put(map, "aloha1", 1);
    EXPECT_EQ(hash_map_reserve(map, SIZE_MAX), MEMORY_ERROR);
}


TEST_F(HashMapTests, pop_existing){
    hash_map_put(map, "aloha", 13);
    int value;
    EXPECT_EQ(hash_map_pop(map, "aloha", &value), OK);
    EXPECT_EQ(value, 13);
}

TEST_F(HashMapTests, pop_middle){
    hash_map_put(map, "aloha", 13);
    hash_map_put(map, "aloha1", 32);
    hash_map_put(map, "aloha2", 113);
    int value;
    EXPECT_EQ(hash_map_pop(map, "aloha1", &value), OK);
}

TEST_F(HashMapTests, remove_existing){
    hash_map_put(map, "aloha", 13);
    int value;
    ASSERT_EQ(hash_map_remove(map, "aloha"), OK);
    EXPECT_EQ(hash_map_contains(map, "aloha"), false);
}

TEST_F(HashMapTests, remove_notexisting){
    hash_map_put(map, "aloha", 13);
    int value;
    ASSERT_EQ(hash_map_remove(map, "aloha1"), KEY_ERROR);
    EXPECT_EQ(hash_map_contains(map, "aloha"), true);
}


TEST_F(HashMapTests, size){
    hash_map_put(map, "aloha", 1);
    ASSERT_EQ(hash_map_size(map), 1);
}


TEST_F(HashMapTests, size_empty){
    ASSERT_EQ(hash_map_size(map), 0);
}

TEST_F(HashMapTests, capacity){
    ASSERT_EQ(hash_map_capacity(map), HASH_MAP_INIT_SIZE);
}

//test the difference between treshold value in implementation and specifiation
TEST_F(HashMapTests, reallocation_threshold){
    auto threshold = 2/3.;
    std::string key;
    auto size = map->allocated;
    int value = 1;
    for(int i = 0; i < 30; i++){
        key = "aloha" + std::to_string(i);
        hash_map_put(map, key.c_str(), i);
        if(map->allocated > (HASH_MAP_INIT_SIZE * value)){
            EXPECT_GE(((float)(i)/(float)(HASH_MAP_INIT_SIZE * value)), threshold);
            value *=2;
        }
    }
}


TEST_F(HashMapTests, clear){
    hash_map_put(map, "aloha", 13);
    hash_map_clear(map);
    EXPECT_EQ(hash_map_contains(map, "aloha"), false);
}

TEST_F(HashMapTests, clear_empty){
    hash_map_clear(map);
    ASSERT_EQ(hash_map_size(map), 0);
}

/*** Konec souboru white_box_tests.cpp ***/