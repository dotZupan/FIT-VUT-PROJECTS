//======= Copyright (c) 2024, FIT VUT Brno, All rights reserved. ============//
//
// Purpose:     Red-Black Tree - public interface tests
//
// $NoKeywords: $ivs_project_1 $black_box_tests.cpp
// $Author:     MATUS FIGNAR <xfignam00@stud.fit.vutbr.cz>
// $Date:       $2024-02-14
//============================================================================//
/**
 * @file black_box_tests.cpp
 * @author MATÚŠ FIGNÁR
 * 
 * @brief Implementace testu binarniho stromu.
 */

#include <vector>

#include "gtest/gtest.h"

#include "red_black_tree.h"

//============================================================================//
// ** ZDE DOPLNTE TESTY **
//
// Zde doplnte testy Red-Black Tree, testujte nasledujici:
// 1. Verejne rozhrani stromu
//    - InsertNode/DeleteNode a FindNode
//    - Chovani techto metod testuje pro prazdny i neprazdny strom.
// 2. Axiomy (tedy vzdy platne vlastnosti) Red-Black Tree:
//    - Vsechny listove uzly stromu jsou *VZDY* cerne.
//    - Kazdy cerveny uzel muze mit *POUZE* cerne potomky.
//    - Vsechny cesty od kazdeho listoveho uzlu ke koreni stromu obsahuji
//      *STEJNY* pocet cernych uzlu.
//============================================================================//

class EmptyTree : public ::testing::Test{
protected:
    BinaryTree tree;
};


class NonEmptyTree : public ::testing::Test{


protected:
    virtual void SetUp(){
        int values[] = {1, 2, 3, 4, 5};
        for(auto value : values){
            tree.InsertNode(value);
        }
    }
    BinaryTree tree;
};


class TreeAxioms : public ::testing::Test{


protected:
    virtual void SetUp(){
        int values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        for(auto value : values){
            tree.InsertNode(value);
        }
    }
    BinaryTree tree;
};


/*** ____________ TEST FOR EMPTY TREE ____________ ***/


TEST_F(EmptyTree, InsertNode){
    auto result = tree.InsertNode(13);
    ASSERT_EQ(result.first, true);
    EXPECT_NE(result.second, nullptr);
}

TEST_F(EmptyTree, DeleteNode){
    auto result = tree.DeleteNode(2);
    EXPECT_EQ(result, false);
}

TEST_F(EmptyTree, FindNode){
    auto result = tree.FindNode(1);
    EXPECT_EQ(result, nullptr);
}


/*** ____________ TEST FOR NON EMPTY TREE ____________ ***/


TEST_F(NonEmptyTree, InsertNode_VALID){
    auto result = tree.InsertNode(6);
    ASSERT_EQ(result.first, true);
    EXPECT_NE(result.second, nullptr);
}

TEST_F(NonEmptyTree, InsertNode_INVALID){
    auto result = tree.InsertNode(3);
    ASSERT_EQ(result.first, false);
    EXPECT_EQ(result.second, tree.FindNode(3));
}

TEST_F(NonEmptyTree, DeleteNode_VALID){
    auto result = tree.DeleteNode(1);
    EXPECT_EQ(result, true);
}

TEST_F(NonEmptyTree, DeleteNode_INVALID){
    auto result = tree.DeleteNode(6);
    EXPECT_EQ(result, false);
}

TEST_F(NonEmptyTree, FindNode_VALID){
    auto result = tree.FindNode(2);
    EXPECT_NE(result, nullptr);
}

TEST_F(NonEmptyTree, FindNode_INVALID){
    auto result = tree.FindNode(6);
    EXPECT_EQ(result, nullptr);
}


/*** ____________ TESTS FOR AXIOMS ____________ ***/

TEST_F(TreeAxioms, Axiom1){
    std::vector<Node_t *> leafs;
    tree.GetLeafNodes(leafs);
    for(auto leaf : leafs){
        EXPECT_EQ(leaf->color, BLACK);
    }
}

TEST_F(TreeAxioms, Axiom2){
    std::vector<Node_t *> leafs;
    tree.GetNonLeafNodes(leafs);
    for(auto leaf : leafs){
        if(leaf->color == RED){
        EXPECT_EQ(leaf->pLeft->color, BLACK);
        EXPECT_EQ(leaf->pRight->color, BLACK);
        }
    }
}


TEST_F(TreeAxioms, Axiom3){
    std::vector<Node_t *> leafs;
    tree.GetLeafNodes(leafs);
    int count1 = -1;
    for(auto leaf : leafs){
        int count = 0;
        while(leaf != nullptr){ //iterate until you get to the root
            if(leaf->color == BLACK){
                count++;
            }
            leaf = leaf->pParent;
        }
        if(count1 == -1){ 
            count1 = count; //set value to be compared
        }
        else{
            EXPECT_EQ(count1, count);
        }
    }
}


/*** Konec souboru black_box_tests.cpp ***/
