#include <algorithm>
#include <random>
#include <iostream>
#include <thread>
#include<bits/stdc++.h>
#include<chrono>

#include "board.hpp"
#include "engine.hpp"
#include "butils.hpp"

std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());

int moves_completed=0;
int sim_count=0;
class Node
{
    public:
    double q=0;
    double n=0;
    std::vector<std::pair<Node*,U16>> children;
    std::vector<U16> unexplored_moves;
    Board board;
    Node(const Board b)
    {
        this->board=Board(b);
        for(auto move:this->board.get_legal_moves()) this->unexplored_moves.push_back(move);
    }
    Node(){}
};

class MCTS
{
    public:
    Node* root;
    double exploration_weight=0;
    double iters=0;
    double sims=0;
    std::vector<Node*> all_nodes;

    MCTS(const Board b,double weight,double iters,double sims)
    {
        this->root=new Node(b);
        this->all_nodes.push_back(this->root);
        this->exploration_weight=weight;
        this->iters=iters;
        this->sims=sims;
    }

    Node* create_node(Board b)
    {
        Node* node_ptr=new Node(b);
        this->all_nodes.push_back(node_ptr);
        return node_ptr;
    }

    U16 random_move(const Board b)
    {
        std::vector<U16> moves;
        for(auto move:b.get_legal_moves()) moves.push_back(move);
        shuffle(moves.begin(),moves.end(),rng);
        return moves[0];
    }

    std::pair<Node*,U16> best_UCT_move(Node* node)
    {
        double log_node=log(node->n);
        double uct=-1e18;
        Node* next_node=NULL;
        U16 next_move=0;
        for(auto element:node->children)
        {
            Node* child=element.first;
            U16 move=element.second;
            double val=(child->q/child->n)+this->exploration_weight*(sqrt(log_node/child->n));
            if(val>uct)
            {
                next_node=child;
                next_move=move;
                uct=val;
            }
        }
        assert((uct>(-1e18)) && (next_node!=NULL) && (next_move!=0));

        return {next_node,next_move};
    }

    std::vector<Node*> select()
    {
        std::vector<Node*> path;
        Node* node_ptr=this->root;
        while(true)
        {
            path.push_back(node_ptr);

            if(node_ptr->unexplored_moves.size() != 0)
            {
                Board b=Board(node_ptr->board);
                U16 next_move=node_ptr->unexplored_moves.back();
                b.do_move_(next_move);
                node_ptr->unexplored_moves.pop_back();

                Node* new_node=create_node(b);
                node_ptr->children.push_back({new_node,next_move});
                path.push_back(new_node);
                break;
            }

            node_ptr=this->best_UCT_move(node_ptr).first;
        }

        return path;
    }

    double simulate(Node* node_ptr)
    {
        int simulations=0;
        bool invert=true;
        Board b=node_ptr->board;
        while(simulations<this->sims)
        {
            if((b.get_legal_moves()).size() == 0)
            {
                if(! invert) return 0;
                else return 1;

                sim_count++;
            }
            U16 move=this->random_move(b);
            b.do_move_(move);
            invert=!invert;
            simulations+=1;
        }
        return 0.5;
    }

    void update(std::vector<Node*> nodes,double reward)
    {
        for(int i=nodes.size()-1;i>=0;i--)
        {
            nodes[i]->n+=1;
            nodes[i]->q+=reward;
            reward=1-reward;
        }
    }

    void iterate()
    {
        std::vector<Node*> path=this->select();
        Node* expansion_node=path.back();
        double reward=this->simulate(expansion_node);
        this->update(path,reward);
    }

    void clearMCT()
    {
        for(auto ptr:this->all_nodes) delete(ptr);
    }

    U16 startMCT()
    {
        for(int i=0;i<this->iters;i++) 
        {
            auto start_time=std::chrono::high_resolution_clock::now();
            this->iterate();
            auto end_time=std::chrono::high_resolution_clock::now();
            //std::cout<<i<<' '<<this->all_nodes.size()<<' '<<std::chrono::duration_cast<std::chrono::milliseconds>(end_time-start_time).count()<<'\n';
            std::cout<<i<<' '<<sizeof(*this)<<'\n';
        }

        std::cout<<this->best_UCT_move(this->root).first<<'\n';
        U16 move=this->best_UCT_move(this->root).second;
        this->clearMCT();
        return move;
    }
};

void Engine::find_best_move(const Board& b) {

    // pick a random move
    
    // auto moveset = b.get_legal_moves();
    // if (moveset.size() == 0) {
    //     std::cout << "Could not get any moves from board!\n";
    //     std::cout << board_to_str(&b.data);
    //     this->best_move = 0;
    // }
    // else {
    //     std::vector<U16> moves;
    //     std::cout << show_moves(&b.data, moveset) << std::endl;
    //     for (auto m : moveset) {
    //         std::cout << move_to_str(m) << " ";
    //     }
    //     std::cout << std::endl;
    //     std::sample(
    //         moveset.begin(),
    //         moveset.end(),
    //         std::back_inserter(moves),
    //         1,
    //         std::mt19937{std::random_device{}()}
    //     );
    //     this->best_move = moves[0];
    // }

    // // just for debugging, to slow down the moves
    // std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    moves_completed+=1;
    sim_count=0;

    std::cout<<"This is for move: "<<moves_completed<<'\n';
    MCTS game(b,sqrt(2),100,100);

    auto start_time=std::chrono::high_resolution_clock::now();
    U16 move=game.startMCT();
    auto end_time=std::chrono::high_resolution_clock::now();
    std::cout<<"Final total time: "<<std::chrono::duration_cast<std::chrono::milliseconds>(end_time-start_time).count()<<'\n';
    std::cout<<"Terminal states reached: "<<sim_count<<'\n';
    std::cout<<"Final size of MCT"<<sizeof(game)<<'\n';
    this->best_move=move;
}
