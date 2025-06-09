//======= Copyright (c) 2024, FIT VUT Brno, All rights reserved. ============//
//
// Purpose:     Test Driven Development - graph
//
// $NoKeywords: $ivs_project_1 $tdd_code.cpp
// $Author:     MATÚŠ FIGNÁR <xfignam00@stud.fit.vutbr.cz>
// $Date:       $2024-02-14
//============================================================================//
/**
 * @file tdd_code.cpp
 * @author Martin Dočekal
 * @author Karel Ondřej
 *
 * @brief Implementace metod tridy reprezentujici graf.
 */

#include "tdd_code.h"


Graph::Graph(){


}

Graph::~Graph(){
    clear();
}

std::vector<Node*> Graph::nodes() {
    std::vector<Node*> nodes = nodes_;

    return nodes;
}

std::vector<Edge> Graph::edges() const{
    std::vector<Edge> edges = edges_;

    return edges;
}

Node* Graph::addNode(size_t nodeId) {
    for(auto node : nodes_){
        if(node->id == nodeId){ //if node already exists end the funcion
            return nullptr;
        }
    }
    Node* newNode = new Node;
    nodes_.push_back(newNode); //push new created node to the back of the list
    newNode->id = nodeId;
    newNode->color = 0;
    return newNode;
}

bool Graph::addEdge(const Edge& edge){
    if(containsEdge(edge)){
        return false;
    }

    if(edge.a == edge.b){
        return false;
    }
    Edge newEdge =  Edge(edge.a, edge.b);
    auto buffera = getNode(edge.a);
    auto bufferb = getNode(edge.b);
    //if edges aren't nodes add them to the list
    if(buffera == nullptr){
        buffera = addNode(edge.a);
    }
    if(bufferb == nullptr){
        bufferb = addNode(edge.b);

    }
    //pair the nodes by adding eachoter to their list of neighbours
    buffera->nodes.push_back(bufferb);
    bufferb->nodes.push_back(buffera);
    edges_.push_back(newEdge);
    return true;

}

void Graph::addMultipleEdges(const std::vector<Edge>& edges) {
    for(auto edge : edges){
        addEdge(edge);
    }
    
}

Node* Graph::getNode(size_t nodeId){
    for(auto node : nodes_){ 
        if(node->id == nodeId){
            return node;
        }
    }
    return nullptr;
}

bool Graph::containsEdge(const Edge& edge) const{
    for(auto buffer : edges_){
        if(edge==buffer){
            return true;
        }
    }
    return false;
}

void Graph::removeNode(size_t nodeId){
    auto node = getNode(nodeId);
    if(node == nullptr){ //if node doesn't exist
        throw std::out_of_range("Node out of the range."); 
    }

    //if edge contains node remove it
    for(int i = 0; i < edges_.size(); i++){
        if(edges_[i].a == nodeId || edges_[i].b == nodeId){
            removeEdge(edges_[i]);
            --i;
        }
    }

    for(int i = 0; i < nodes_.size(); i++){
        if(nodes_[i]->id == nodeId){
            nodes_.erase(nodes_.begin() + i);
            break;
        }
    }
    delete node;
}

void Graph::removeEdge(const Edge& edge){
    if(!containsEdge(edge)){ //if edge doesn't exist
      throw std::out_of_range("Edge out of the range.");  
    }
    for(int i = 0; i < edges_.size(); i++){
        if(edge == edges_[i]){
            edges_.erase(edges_.begin() + i);
            break;
        }
    }
    
    
}

size_t Graph::nodeCount() const{
    return nodes_.size();
}

size_t Graph::edgeCount() const{
    return edges_.size();
}

size_t Graph::nodeDegree(size_t nodeId) const{
    auto node = const_cast<Graph*>(this)->getNode(nodeId);
    if(!node){
        throw std::out_of_range("Invalid node ID");
    }
    return node->nodes.size();
}

size_t Graph::graphDegree() const{
    size_t max = 0;
    size_t buffer = 0;
    for(auto node : nodes_){
        buffer = node->nodes.size();
        if(max < buffer){
            max = buffer;
        }
    }
    return max;
}

void Graph::coloring(){
    if(nodeCount() == 0){ //when list is empty return the funcion
        return;
    }

    size_t numColor = graphDegree() + 1;
    bool colored = false;
    for(auto node : nodes_){
        for(size_t color = 1; color <= numColor; color++){
            node->color = color;
            for(auto neighbour : node->nodes){
                //if color matches with neighbour color determine variable colored as false and break from the cycle 
                //otherwise set variable colored as true
                if(node->color == neighbour->color){ 
                    colored = false;
                    break;
                }
                colored = true;
            }
            if(colored){
                colored = false;
                break;
            }
            
        }
    }
}

void Graph::clear() {
    for(auto node : nodes_){
        node->nodes.clear();
        delete node;
    }
    nodes_.clear();
    edges_.clear();

}

/*** Konec souboru tdd_code.cpp ***/
