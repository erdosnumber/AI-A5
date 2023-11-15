#include <algorithm>
#include <random>
#include <iostream>
#include <thread>
#include<bits/stdc++.h>
#include<chrono>

#include "board.hpp"
#include "engine.hpp"
#include "butils.hpp"
#define piece_type(p) (PieceType)((p) & (EMPTY | ROOK | BISHOP | KING | PAWN | KNIGHT))

/*
Remember to not keep 7 as hardcoded anywhere, THE THIRD TYPE OF BOARD HAS 8*8 SQUARE

CHECK INVERT CONDITION IN SIMULATE CAREFULLY

YOU WERE ALSO GETTING ASSERTION  ERROR IN UCT_MOVES THING. MAKE SURE TO FIX THAT
*/


std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());

int moves_completed=0;
int sim_count=0;
PlayerColor our_player=WHITE;
PlayerColor opponent_player=BLACK;
bool vary_piece_points=false;
int board_size=7;
std::ofstream output;
std::ofstream MCTstatus;

int piece_points(const Board &c,U8 piece_position)
{
    U8 piece=c.data.board_0[piece_position];
    PieceType p=piece_type(piece);
    PlayerColor side=PlayerColor(piece & (WHITE | BLACK));

    if(vary_piece_points)
    {
        if(p==PAWN)
        {
            U8 prom_pos1,prom_pos2;
            if(side == WHITE) prom_pos1=pos(4,5),prom_pos2=pos(4,6);
            else prom_pos1=pos(2,0),prom_pos2=pos(2,1);
            int mindist=std::min(abs(getx(prom_pos1)-getx(piece_position))+abs(gety(prom_pos1)-gety(piece_position)),
            abs(getx(prom_pos2)-getx(piece_position))+abs(gety(prom_pos2)-gety(piece_position)));
            if(mindist<=2) return 3;
            else if(mindist<=5) return 2;
            else return  1;
        }
        else if(p==ROOK)
        {
            return 6;
        }
        else if(p==BISHOP)
        {
            return 4;
        }
        else if(p==KING)
        {
            return 10;
        }
    }

    else
    {
        if(p==PAWN) return 1;
        else if(p==ROOK) return 5;
        else if(p==BISHOP) return 3;
        else if(p==KNIGHT) return 3;
        else if(p==KING) return 10;
    }
}

int our_piece_points(const Board & c)
{
    int our_pieces=0;

    for(int row=0;row<board_size;row++)
    {
        for(int col=0;col<board_size;col++)
        {
            //under_threat takes argument as piece position
            U8 position=pos(row,col);
            U8 piece=c.data.board_0[position];
            if(piece & our_player) our_pieces+=piece_points(c,position);
        }
    }

    return our_pieces;
}

int opponent_piece_points(const Board & c)
{
    int their_pieces=0;

    for(int row=0;row<board_size;row++)
    {
        for(int col=0;col<board_size;col++)
        {
            //under_threat takes argument as piece position
            U8 position=pos(row,col);
            U8 piece=c.data.board_0[position];
            if(piece & opponent_player) their_pieces+=piece_points(c,position);
        }
    }

    return their_pieces;
}


int termination_condition(const Board& b)
{
    if(b.get_legal_moves().size() == 0) return 1;
    else if(our_piece_points(b)==10 && opponent_piece_points(b)==10) return 2;
    else return 3;
}

bool equal_boards(const Board& b1,const Board &b2)
{
    bool equal=true;

    equal &= (b1.data.w_rook_1 == b2.data.w_rook_1);
    equal &= (b1.data.w_rook_2 == b2.data.w_rook_2);
    equal &= (b1.data.w_king == b2.data.w_king);
    equal &= (b1.data.w_bishop == b2.data.w_bishop);
    equal &= (b1.data.w_knight_1 == b2.data.w_knight_1);
    equal &= (b1.data.w_knight_2 == b2.data.w_knight_2);
    equal &= (b1.data.w_pawn_1 == b2.data.w_pawn_1);
    equal &= (b1.data.w_pawn_2 == b2.data.w_pawn_2);
    equal &= (b1.data.w_pawn_3 == b2.data.w_pawn_3);
    equal &= (b1.data.w_pawn_4 == b2.data.w_pawn_4);

    equal &= (b1.data.b_rook_1 == b2.data.b_rook_1);
    equal &= (b1.data.b_rook_2 == b2.data.b_rook_2);
    equal &= (b1.data.b_king == b2.data.b_king);
    equal &= (b1.data.b_bishop == b2.data.b_bishop);
    equal &= (b1.data.b_knight_1 == b2.data.b_knight_1);
    equal &= (b1.data.b_knight_2 == b2.data.b_knight_2);
    equal &= (b1.data.b_pawn_1 == b2.data.b_pawn_1);
    equal &= (b1.data.b_pawn_2 == b2.data.b_pawn_2);
    equal &= (b1.data.b_pawn_3 == b2.data.b_pawn_3);
    equal &= (b1.data.b_pawn_4 == b2.data.b_pawn_4);

    equal &= (b1.data.player_to_play == b2.data.player_to_play);
    equal &= (b1.data.board_type == b2.data.board_type);

    return equal;
}

U16 random_move(const Board& b)
{
    std::vector<U16> moves;
    for(auto move:b.get_legal_moves()) moves.push_back(move);
    shuffle(moves.begin(),moves.end(),rng);
    return moves[0];
}

/********************************* NODE ************************************************************************/

class Node
{
    public:

    double q=0;
    double n=0;
    int depth=0;
    int id=0;
    std::vector<std::pair<int,U16>> children={};
    std::vector<U16> unexplored_moves={};
    Board board;

    Node(const Board& b,int depth,int id)
    {
        this->board=b;
        for(auto move:this->board.get_legal_moves()) this->unexplored_moves.push_back(move);
        this->depth=depth;
        this->id=id;
    }

    Node(){}
};

/********************************* MCTS **************************************************************************/

class MCTS
{
    public:

    int root=0;
    double weight=0;
    double iters=0;
    double sims=0;
    std::vector<Node> all_nodes={};

    MCTS() {}

    MCTS(const Board& b,double weight,double iters,double sims)
    {
        Node node(b,0,0);
        this->root=0;
        this->all_nodes.push_back(node);
        this->weight=weight;
        this->iters=iters;
        this->sims=sims;
    }

    int create_node(const Board& b,int depth)
    {
        Node node(b,depth,this->all_nodes.size());
        this->all_nodes.push_back(node);
        return node.id;
    }

    std::pair<int,U16> best_UCT_move(int id)
    {
        double log_node=log(this->all_nodes[id].n);
        double uct=-1e18;
        int next_node=-1;
        U16 next_move=0;

        for(auto element:this->all_nodes[id].children)
        {
            int child=element.first;
            U16 move=element.second;
            double val=(this->all_nodes[child].q/this->all_nodes[child].n)
            +this->weight*(sqrt(log_node/this->all_nodes[child].n));
            if(val>uct)
            {
                next_node=child;
                next_move=move;
                uct=val;
            }
        }

        assert((uct>(-1e18)) && (next_node != -1) && (next_move != 0));

        return {next_node,next_move};
    }

    std::vector<int> select()
    {
        std::vector<int> path;
        int node_ptr=this->root;

        while(true)
        {
            path.push_back(node_ptr);

            if((this->all_nodes[node_ptr].unexplored_moves).size() != 0)
            {
                Board b(this->all_nodes[node_ptr].board);
                U16 next_move=this->all_nodes[node_ptr].unexplored_moves.back();
                b.do_move_(next_move);
                this->all_nodes[node_ptr].unexplored_moves.pop_back();

                int new_node=create_node(b,this->all_nodes[node_ptr].depth+1);
                this->all_nodes[node_ptr].children.push_back({new_node,next_move});
                path.push_back(new_node);
                break;
            }

            //here u would need to ensure that node_ptr is not a terminal state
            if(this->all_nodes[node_ptr].children.size()!=0) node_ptr=this->best_UCT_move(node_ptr).first;
            else break;
        }

        return path;
    }

    double simulate(int node_ptr)
    {
        int simulations=0;
        //check invert condition here carefully
        bool invert=false;

        Board b(this->all_nodes[node_ptr].board);
        while(simulations<this->sims)
        {
            output<<simulations<<' '<<our_piece_points(b)<<' '<<opponent_piece_points(b)<<'\n';
            if(termination_condition(b)!=3)
            {
                if(termination_condition(b)==2) return 0.5;
                sim_count++;
                if(invert) return 0;
                else return 1;
            }

            U16 move=random_move(b);
            b.do_move_(move);
            invert=!invert;
            simulations+=1;
        }
        return 0.5;
    }

    void update(std::vector<int> nodes,double reward)
    {
        for(int i=nodes.size()-1;i>=0;i--)
        {
            this->all_nodes[nodes[i]].n+=1;
            this->all_nodes[nodes[i]].q+=reward;
            reward=1-reward;
        }
    }

    void iterate()
    {
        std::vector<int> path=this->select();
        int expansion_node=path.back();
        double reward=this->simulate(expansion_node);
        this->update(path,reward);
    }

    void showMCT()
    {
        for(auto node:this->all_nodes)
        {
            MCTstatus<<node.depth<<' '<<node.n<<' '<<node.q<<'\n';
        }
    }

    void clearMCT()
    {
        return;
    }

    U16 startMCT()
    {
        for(int i=0;i<this->iters;i++) 
        {
            auto start_time=std::chrono::high_resolution_clock::now();
            output<<"This is for game = "<<i<<"\n\n";
            this->iterate();
            auto end_time=std::chrono::high_resolution_clock::now();
            //std::cout<<i<<' '<<this->all_nodes.size()<<' '<<std::chrono::duration_cast<std::chrono::milliseconds>(end_time-start_time).count()<<'\n';
        }
        U16 move=this->best_UCT_move(this->root).second;
        this->root=this->best_UCT_move(this->root).first;
        this->showMCT();
        return move;
    }

    void reroot(const Board& b)
    {
        std::vector<U16> unexplored;
        int cnt=0;

        for(auto move:this->all_nodes[root].unexplored_moves)
        {
            Board c(this->all_nodes[root].board);
            c.do_move_(move);
            if(equal_boards(c,b))
            {
                cnt++;
                int node_ptr=this->create_node(b,this->all_nodes[root].depth+1);
                this->all_nodes[root].children.push_back({node_ptr,move});
            }
            else unexplored.push_back(move);
        }

        assert(cnt<=1);

        this->all_nodes[root].unexplored_moves=unexplored;

        int next_root;
        cnt=0;

        for(auto child:this->all_nodes[root].children)
        {
            Board c(this->all_nodes[root].board);
            c.do_move_(child.second);
            if(equal_boards(c,b))
            {
                cnt=1;
                next_root=child.first;
                break;
            }
        }

        assert(cnt==1);
        this->root=next_root;
    }

};

/***********************************  ACTION STARTS HERE  **********************************************************************************/

MCTS game;

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

    if(moves_completed==0) 
    {
        output.open("simulation_log");
        MCTstatus.open("mct_log");
        output<<"LESGO\n\n";
        MCTstatus<<"LESGO\n\n";
        game = MCTS(b,0.5,500,500);
    }

    else game.reroot(b);

    moves_completed+=1;
    sim_count=0;

    output<<"\n\nTHIS IS FOR MOVES COMPLETED = "<<moves_completed<<"\n\n";
    MCTstatus<<"\n\nTHIS IS FOR MOVES COMPLETED = "<<moves_completed<<"\n\n";

    std::cout<<"This is for move: "<<moves_completed<<'\n';

    auto start_time=std::chrono::high_resolution_clock::now();
    U16 move=game.startMCT();
    auto end_time=std::chrono::high_resolution_clock::now();

    std::cout<<"Final total time: "<<std::chrono::duration_cast<std::chrono::milliseconds>(end_time-start_time).count()<<'\n';
    std::cout<<"Terminal states reached: "<<sim_count<<'\n';
    std::cout<<"Total nodes till now: "<<game.all_nodes.size()<<'\n';

    this->best_move=move;
}
