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
CAREFULLY CHECK THE PAWN PROMOTION POSITIONS
*/


std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());

const int inf=(1e9);

PlayerColor our_player=WHITE;
PlayerColor opponent_player=BLACK;
bool vary_piece_points=false;
int board_size=7;
BoardType board_type=SEVEN_THREE;
std::unordered_map<PieceType,int> points;
std::map<PlayerColor,std::vector<U16>> prom_pos;
double factor=1;

void board_init(const Board& c)
{
    vary_piece_points=true;
    board_type=c.data.board_type;
    our_player = c.data.player_to_play;
    opponent_player = (PlayerColor)(c.data.player_to_play ^ (BLACK | WHITE));

    points[PAWN]=1;
    points[ROOK]=6;
    points[BISHOP]=4;
    points[KNIGHT]=4;
    points[KING]=10;


    if(board_type == SEVEN_THREE)
    {
        board_size=7;
        factor=1;
        prom_pos[WHITE]={pos(4,5),pos(4,6)};
        prom_pos[BLACK]={pos(2,0),pos(2,1)};
    }
    else if(board_type == EIGHT_FOUR)
    {
        board_size=8;
        factor=1.5;
        prom_pos[WHITE]={pos(5,6),pos(5,7)};
        prom_pos[BLACK]={pos(2,0),pos(2,1)};
    }
    else
    {
        board_size=8;
        factor=2;
        prom_pos[WHITE]={pos(5,5),pos(5,6),pos(5,7)};
        prom_pos[BLACK]={pos(2,0),pos(2,1),pos(2,2)};
    }
}

int piece_points(const Board& c,U8 piece_position)
{
    U8 piece=c.data.board_0[piece_position];
    PieceType p=piece_type(piece);
    PlayerColor side=PlayerColor(piece & (WHITE | BLACK));

    if(vary_piece_points)
    {
        if(p==PAWN)
        {
            int mindist=inf;
            for(auto pos:prom_pos[side])
            {
                mindist=std::min(mindist,
                abs(getx(pos)-getx(piece_position))+abs(gety(pos)-gety(piece_position)));
            }
            if(mindist<=2) return 3;
            else if(mindist<=5) return 2;
            else return 1;
        }
        else return points[p];
    }

    else return points[p];
}

int our_piece_points(const Board& c)
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

int opponent_piece_points(const Board& c)
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

bool under_threat_by_side(const Board& b,U8 piece_pos,PlayerColor color)
{
    auto pseudolegal_moves = b.get_pseudolegal_moves_for_side(color);

    for (auto move : pseudolegal_moves) {
        // std::cout << move_to_str(move) << " ";
        if (getp1(move) == piece_pos) {
            // std::cout << "<- causes check\n";
            return true;
        }
    }
    // std::cout << std::endl;

    return false;
}

std::vector<U8> which_positions_threatening(const Board& b,U8 piece_pos,PlayerColor color)
{
    std::vector<U8> positions_threatening;

    //this function finds the positions of pieces of "color" which threaten pos_piece
    auto pseudolegal_moves=b.get_pseudolegal_moves_for_side(color);
    for(auto move:pseudolegal_moves)
    {
        if(getp1(move)==piece_pos) positions_threatening.push_back(getp0(move));
    }
    return positions_threatening;
}

int opponent_pieces_threatened(const Board & c)
{
    int max_theirs_in_threat=0;

    if(c.data.player_to_play==our_player)
    {
        auto moves=c.get_legal_moves();
        for(int row=0;row<board_size;row++)
        {
            for(int col=0;col<board_size;col++)
            {
                //under_threat takes argument as piece position
                U8 position=pos(row,col);
                U8 piece=c.data.board_0[position];
                if((piece & opponent_player) && (piece_type(piece)!=KING))
                {
                    bool in_attack=false;
                    int op_points=piece_points(c,position);
                    int our_points=0;
                    U16 attacking_move;
                    for(auto move:moves)
                    {
                        if(getp1(move)==position)
                        {
                            in_attack=true;
                            attacking_move=move;
                            our_points=piece_points(c,getp0(move));
                        }
                    }

                    if(in_attack)
                    {
                        // Board* cpy=c.copy();
                        // Board& d=*cpy;
                        Board d(c);
                        d.do_move_(attacking_move);
                        auto next_moves=d.get_legal_moves();
                        bool counter_attack=false;
                        for(auto move:next_moves)
                        {
                            if(getp1(move)==position) counter_attack=true;
                        }
                        if(counter_attack) max_theirs_in_threat=std::max(max_theirs_in_threat,op_points-our_points);
                        else max_theirs_in_threat=std::max(max_theirs_in_threat,op_points);
                    }
                }
            }
        }
    }

    return max_theirs_in_threat;
}

int our_pieces_threatened(const Board & c)
{
    int max_ours_in_threat=0;

    if(c.data.player_to_play==opponent_player)
    {
        auto moves=c.get_legal_moves();
        for(int row=0;row<board_size;row++)
        {
            for(int col=0;col<board_size;col++)
            {
                //under_threat takes argument as piece position
                U8 position=pos(row,col);
                U8 piece=c.data.board_0[position];
                if((piece & our_player) && (piece_type(piece)!=KING))
                {
                    bool in_attack=false;
                    int our_points=piece_points(c,position);
                    int op_points=0;
                    U16 attacking_move;
                    for(auto move:moves)
                    {
                        if(getp1(move)==position)
                        {
                            in_attack=true;
                            attacking_move=move;
                            op_points=piece_points(c,getp0(move));
                        }
                    }

                    if(in_attack)
                    {
                        // Board* cpy=c.copy();
                        // Board& d=*cpy;
                        Board d(c);
                        d.do_move_(attacking_move);
                        auto next_moves=d.get_legal_moves();
                        bool counter_attack=false;
                        for(auto move:next_moves)
                        {
                            if(getp1(move)==position) counter_attack=true;
                        }
                        if(counter_attack) max_ours_in_threat=std::max(max_ours_in_threat,our_points-op_points);
                        else max_ours_in_threat=std::max(max_ours_in_threat,our_points);
                    }
                }
            }
        }
    }
    return max_ours_in_threat;
}

int future_moves(const Board & c)
{
    int points=c.get_pseudolegal_moves_for_side(our_player).size();
    return points;
}

int king_in_check(const Board & c)
{
    int points=0;
    U8 white_king_pos=c.data.w_king;
    U8 black_king_pos=c.data.b_king;

    U8 our_king_pos,opponent_king_pos;
    if(our_player == WHITE) our_king_pos=white_king_pos;
    else our_king_pos=black_king_pos;

    if(opponent_player == WHITE) opponent_king_pos=white_king_pos;
    else opponent_king_pos=black_king_pos;

    if(under_threat_by_side(c,our_king_pos,opponent_player) && (c.data.player_to_play == opponent_player))
    {
        bool can_check_be_avoided=true;
        auto positions_threatening=which_positions_threatening(c,our_king_pos,opponent_player);
        assert(positions_threatening.size()>0);
        if(positions_threatening.size()>1) can_check_be_avoided=false;
        else if(! under_threat_by_side(c,positions_threatening[0],our_player)) can_check_be_avoided=false;

        if(!can_check_be_avoided) points=-1;
    }

    if(under_threat_by_side(c,opponent_king_pos,our_player) && (c.data.player_to_play == our_player))
    {
        bool can_check_be_avoided=true;
        auto positions_threatening=which_positions_threatening(c,opponent_king_pos,our_player);
        assert(positions_threatening.size()>0);
        if(positions_threatening.size()>1) can_check_be_avoided=false;
        else if(! under_threat_by_side(c,positions_threatening[0],opponent_player)) can_check_be_avoided=false;

        if(!can_check_be_avoided) points=1;
    }

    return points;
}

int our_pieces_with_no_support(const Board &c)
{
    int no_support=0;

    if(c.data.player_to_play==opponent_player)
    {
        auto moves=c.get_legal_moves();
        for(int row=0;row<board_size;row++)
        {
            for(int col=0;col<board_size;col++)
            {
                U8 position=pos(row,col);
                U8 piece=c.data.board_0[position];
                if(piece & our_player)
                {
                    int our_points=piece_points(c,position);
                    U16 attacking_move;
                    bool in_attack=false;
                    for(auto move:moves)
                    {
                        if(getp1(move)==position) 
                        {
                            attacking_move=move;
                            in_attack=true;
                        }
                    }
                    if(in_attack)
                    {
                        Board d(c);
                        d.do_move_(attacking_move);
                        auto next_moves=d.get_legal_moves(); //returns moves of our player
                        bool counter_attack=false;
                        for(auto move:next_moves)
                        {
                            if(getp1(move)==position) counter_attack=true;
                        }
                        if(!counter_attack) no_support+=our_points;
                    }
                }
            }
        }
    }
    return no_support;
}

int opponent_pieces_with_no_support(const Board & c)
{
    int no_support=0;

    if(c.data.player_to_play==our_player)
    {
        auto moves=c.get_legal_moves();
        for(int row=0;row<board_size;row++)
        {
            for(int col=0;col<board_size;col++)
            {
                U8 position=pos(row,col);
                U8 piece=c.data.board_0[position];
                if(piece & opponent_player)
                {
                    int op_points=piece_points(c,position);
                    U16 attacking_move;
                    bool in_attack=false;
                    for(auto move:moves)
                    {
                        if(getp1(move)==position) 
                        {
                            attacking_move=move;
                            in_attack=true;
                        }
                    }
                    if(in_attack)
                    {
                        Board d(c);
                        d.do_move_(attacking_move);
                        auto next_moves=d.get_legal_moves(); //returns moves of opponent player
                        bool counter_attack=false;
                        for(auto move:next_moves)
                        {
                            if(getp1(move)==position) counter_attack=true;
                        }
                        if(!counter_attack) no_support+=op_points;
                    }
                }
            }
        }
    }

    return no_support;
}

/************************************************************ Here ends the heuristics ***********************************************************************************/

int nodes_visited=0;
int pruned=0;
auto start_time=std::chrono::high_resolution_clock::now();
double time_buffer=10;
int current_move=0;

int heuristic_for_selection(const Board & c)
{
    int pieces_in_threat=0;
    for(int row=0;row<board_size;row++)
    {
        for(int col=0;col<board_size;col++)
        {
            U8 position=pos(row,col);
            U8 piece=c.data.board_0[position];
            if((piece & our_player) && (under_threat_by_side(c,position,opponent_player)))
            pieces_in_threat+=piece_points(c,position);
        }
    }

    int piece_difference=our_piece_points(c)-opponent_piece_points(c);

    return piece_difference-pieces_in_threat;
}

int evaluation_function(const Board & c)
{
    if((c.data.player_to_play == our_player) && (c.get_legal_moves().size())==0) return -inf;
    else if((c.data.player_to_play==opponent_player) && (c.get_legal_moves().size()==0)) return inf;
    else
    {
        int points1=0,points2=0,points3=0,points4=0,points5=0,points6=0;

        points1=our_piece_points(c)-opponent_piece_points(c);
        points2=future_moves(c);
        points3=king_in_check(c);
        points4=opponent_pieces_with_no_support(c)-our_pieces_with_no_support(c);
        points5=opponent_pieces_threatened(c)-our_pieces_threatened(c);
        int points;

        points=(100*points1+points2+100*points3+10*points4+100*points5); //dont change any weights now
        return points;
    }
}

double get_time_left(double total_time)
{
    auto cur_time=std::chrono::high_resolution_clock::now();
    double time_passed=std::chrono::duration_cast<std::chrono::milliseconds>(cur_time-start_time).count();
    return (total_time-time_passed);
}

std::pair<int,U16> alpha_beta_pruning(const Board& c,int alpha,int beta,int cutoff,double total_time)
{
    if(get_time_left(total_time)<time_buffer)
    {
        if(c.data.player_to_play == our_player) return {-inf,(U16)0};
        else return {inf,(U16)0};
    }

    nodes_visited++;
    if(cutoff == 0) 
    {
        return {evaluation_function(c),(U16)0};
    }

    std::unordered_set<U16> valid_moves = c.get_legal_moves();

    if(valid_moves.size() == 0)
    {
        if(c.data.player_to_play == our_player) return {-inf,(U16)0};
        else return {inf,(U16)0};
    }

    else
    {
        std::vector<std::pair<int,U16>> move_choices;
        for(auto move:valid_moves) 
        {
            Board cpy(c);
            cpy.do_move_(move);
            move_choices.push_back({heuristic_for_selection(cpy),move});
        }

        std::sort(move_choices.begin(),move_choices.end());

        if(c.data.player_to_play == our_player)
        {
            //we behaving as a maximizing node
            int val=-inf;
            U16 next_move=0;
            for(int i=(int)move_choices.size()-1;i>=0;i--)
            {
                U16 move=move_choices[i].second;
                Board cnew(c); cnew.do_move_(move);
                assert(cnew.data.player_to_play == (c.data.player_to_play ^ (BLACK | WHITE)));

                std::pair<int,U16> new_move=alpha_beta_pruning(cnew,alpha,beta,cutoff-1,total_time);
                if(new_move.first>val)
                {
                    val=new_move.first;
                    next_move=move;
                }

                if(get_time_left(total_time)<time_buffer) return {val,next_move};
                alpha=std::max(alpha,new_move.first);
                if(alpha >= beta) 
                {
                    pruned++;
                    return {val,next_move};
                }
            }

            return {val,next_move};
        }

        else
        {
            //opponent functioning as a minimizing node
            int val=inf;
            U16 next_move=0;
            for(int i=0;i<move_choices.size();i++)
            {
                U16 move=move_choices[i].second;

                //make a copy of c and then pass to
                //new alpha beta function
                Board cnew(c); cnew.do_move_(move);
                assert(cnew.data.player_to_play == (c.data.player_to_play ^ (BLACK | WHITE)));

                std::pair<int,U16> new_move=alpha_beta_pruning(cnew,alpha,beta,cutoff-1,total_time);
                if(new_move.first<val)
                {
                    val=new_move.first;
                    next_move=move;
                }
            
                if(get_time_left(total_time)<time_buffer) return {val,next_move};
                beta=std::min(beta,new_move.first);
                if(alpha >= beta) 
                {
                    pruned++;
                    return {val,next_move};
                }
            }
            return {val,next_move};
        }
    }
}

void undo_last_move(Board& c,U16 move)
{
    c.undo_last_move_without_flip_(move);
    c.flip_player_();
}

std::unordered_map<std::string,int> board_hash;

double piece_time(PieceType piece_type)
{
    if(piece_type == PAWN || piece_type == KNIGHT || piece_type == KING) return 1;
    else if(piece_type == BISHOP && board_type == EIGHT_FOUR) return 1;
    else return 3;
}

int piece_time_contribution(const Board& c)
{
    //keeping time linear in piece?
    int cnt=0;
    for(int i=0;i<board_size;i++)
    {
        for(int j=0;j<board_size;j++)
        {
            if(c.data.board_0[pos(i,j)] & our_player) cnt+=piece_time(piece_type(c.data.board_0[pos(i,j)]));
        }
    }
    return 100*cnt;
}

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

    start_time=std::chrono::high_resolution_clock::now();
    board_init(b);

    current_move++;
    std::cout<<"Current move "<<current_move<<" till now\n";

    this->best_move=0;
    if(this->time_left.count()<5) return;

    auto moveset=b.get_legal_moves();

    if(moveset.size()==0) return;

    //this->move
    std::vector<std::pair<int,U16>> move_choices;
    for(auto move:moveset) 
    {
        Board c(b);
        c.do_move_(move);
        move_choices.push_back({heuristic_for_selection(c),move});
    }

    std::sort(move_choices.begin(),move_choices.end());

    // std::cout<<"Number of valid moves: "<<moveset.size()<<std::endl;
    // for(auto choices:move_choices)
    // {
    //     std::cout<<choices.first<<" "<<move_to_str(choices.second)<<' ';
    // }
    // std::cout<<std::endl;

    Board c(b);
    this->best_move=move_choices.back().second;
    c.do_move_(this->best_move);
    std::string prev_hs=board_to_str(&(c.data));
    undo_last_move(c,this->best_move);

    if(board_hash[prev_hs]<2) board_hash[prev_hs]++;
    else if(move_choices.size()>1)
    {
        this->best_move=move_choices[move_choices.size()-2].second;
        c.do_move_(this->best_move);
        prev_hs=board_to_str(&(c.data));
        undo_last_move(c,this->best_move);
        board_hash[prev_hs]++;
    }
    

    int depth_cutoff=1;
    //vary time with number of moves

    double total_time=0;
    double time_left=this->time_left.count();
    if(current_move<=3) total_time=std::min(1000*factor,time_left);
    else if(current_move<=6) total_time=std::min(1500*factor,time_left);
    else if(time_left>10000*factor)
    {
        if(our_piece_points(b)-opponent_piece_points(b)>10) total_time=std::min(1000*factor,time_left);
        else if(our_piece_points(b)-opponent_piece_points(b)>6) total_time=std::min(1500*factor,time_left);
        else if(our_piece_points(b)>=opponent_piece_points(b)) total_time=std::min(2500*factor,time_left);
        else if(our_piece_points(b)-opponent_piece_points(b) > -4) total_time=std::min(3000*factor,time_left);
        else if(our_piece_points(b)-opponent_piece_points(b) > -6) total_time=std::min(3500*factor,time_left);
        else total_time=std::min(4000*factor,time_left);

        // double time_value=500*piece_time_contribution(b);
        // if(our_piece_points(b)-opponent_piece_points(b)>10) total_time=std::min(1000*factor,time_value);
        // else if(our_piece_points(b)-opponent_piece_points(b)>6) total_time=std::min(2000*factor,time_value);
        // else if(our_piece_points(b)>=opponent_piece_points(b)) total_time=std::min(2500*factor,time_value);
        // else if(our_piece_points(b)-opponent_piece_points(b) > -4) total_time=std::min(3000*factor,1.5*time_value);
        // else if(our_piece_points(b)-opponent_piece_points(b) > -6) total_time=std::min(4000*factor,2*time_value);
        // else total_time=std::min(5000*factor,2.5*time_value);

        // total_time=std::min(total_time,time_left);
    }
    else
    {
        total_time=std::min(100*factor,time_left);
    }

    while(true)
    {
        std::cout<<"Starting alpha beta at depth: "<<depth_cutoff<<std::endl;
        nodes_visited=0;
        pruned=0;

        std::pair<int,U16> play_move=alpha_beta_pruning(c,-inf,inf,depth_cutoff,total_time);

        if(get_time_left(total_time) < time_buffer) break;
        else if(play_move.second != 0)
        {
            c.do_move_(play_move.second);
            std::string hs=board_to_str(&(c.data));
            undo_last_move(c,play_move.second);
            if(board_hash[hs]<2)
            {
                //update best_move
                this->best_move=play_move.second;
                board_hash[prev_hs]--;
                board_hash[hs]++;
                prev_hs=hs;
            }
        }
        //so we completed the search at this depth
        std::cout<<"Completed alpha beta at depth: "<<depth_cutoff<<std::endl;
        std::cout<<"Nodes visited: "<<nodes_visited<<std::endl;
        std::cout<<"Pruned nodes: " << pruned << std::endl;
        std::cout<<"Move found at depth: "<<depth_cutoff<<" is: "<<move_to_str(play_move.second)<<std::endl;
        std::cout<<"Time remaining: "<<get_time_left(total_time)<<std::endl;
        depth_cutoff++;
    }
}
