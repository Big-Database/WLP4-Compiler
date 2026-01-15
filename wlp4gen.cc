#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map> 
#include <vector> 
#include <deque> 
#include <set>
// Forward declaration
struct Node;
void gen_lvalue(Node* lval, Node* expr); 
void gen_expr(Node* expr); 
std::string get_lvalue_id(Node * tree); 
std::string find_type(Node * type);
bool isTerminal(const std::string& word) {
    static const std::set<std::string> terminals = {
        "BOF", "EOF", "INT", "WAIN", "RETURN", "SEMI", "COMMA", "LPAREN", "RPAREN", "RBRACK", "LBRACK",
        "LBRACE", "RBRACE", "ID", "NUM", "NULL", "AMP", "STAR", "SLASH", "PCT", "PLUS", "MINUS",
        "BECOMES", "EQ", "NE", "LT", "GT", "LE", "GE", "IF", "ELSE", "WHILE", "NEW", "DELETE",
        "GETCHAR", "PRINTLN", "PUTCHAR"
    };
    return terminals.count(word) > 0;
}
struct Node {
    std::string lhs;             
    std::string rhs;              
    std::vector<Node*> children;  
    std::string type;             


    Node(std::string lhs, std::string rhs, std::string type) : lhs(lhs), rhs(rhs), type(type) {}
    Node(std::string lhs, std::string rhs) : lhs(lhs), rhs(rhs) {}

    ~Node() {
        for (auto child : children) {
            delete child;
        }
    }
};
std::vector<std::string> current_procedure;

            // [procedure name, offset]
std::unordered_map<std::string, int> procedure_param_offset; 
void add_offset(std::string proc_name){
    if (procedure_param_offset.count(proc_name)){
        procedure_param_offset[proc_name]+=4; 
    } else {
        procedure_param_offset[proc_name] = 4;
    }
    //std::cout<< ";2. arglist_offset: "<< procedure_param_offset[current_procedure.back()]<<std::endl;

}

// Global variables 
int frame_pointer_offset= 0;
int stack_pointer = 0; 
int offset = 8; 
int arglist_offset = 0;
bool ptr = false; 
bool lhs_int_star = false; 
bool rhs_int_star = false; 
bool lhs_lval = false; 
int func_arg_nest = 0; // uhhh why I need this again I'm unsure, probably because when i'm passing parameters I need to know where what goes 
int expr_augment = 0; // need to adjust incremend and drecement in amp and star respectively, and based on this check what expression to generate 
std::string expr_type = "empty"; 

                    // prodecure,       variable_name , offset      
std::unordered_map<std::string, std::unordered_map<std::string, int>> symbol_table; 
std::unordered_map<std::string, std::unordered_map<std::string, std::string>> symbol_table_type; 
int label_count = 0;
std::string increment_label(std::string base){
    label_count += 1;
    return "l"+base+std::to_string(label_count);
}
void add_symbol(std::string procedure, std::string varname, int num){
    if(varname.empty()){
        return; 
    }else{
        symbol_table[procedure][varname] = num; 
    }
}

void wain_add_symbol(std::string symbol, std::string procedure){
    if (symbol.empty()) return;
    symbol_table[procedure][symbol] = offset;
    offset -= 4; 
}
void print_Tree(Node* tree) {
    if (!tree) return;
    std::cout << tree->lhs << " " << tree->rhs;
    if (!tree->type.empty()) {
        std::cout << " : " << tree->type;
    }
    std::cout << std::endl;
    for (auto node : tree->children) {
        print_Tree(node);
    }
}
void printSymbolTable(){
    for (const auto& pair : symbol_table) {
        std::cout << ";  Scope: '" << pair.first << "'" << std::endl;
        const auto& inner_map = pair.second;
            for (const auto& inner_pair : inner_map) {
                std::cout << ";    Symbol: '" << inner_pair.first << "', Value: " << inner_pair.second << std::endl;
            }
    }
}
Node* build_Tree(int& position, const std::deque<std::vector<std::string>>& parsed_input) {
    const std::vector<std::string>& line = parsed_input[position];
    std::string lhs = line[0];
    std::string rule_body; 
    for(size_t i = 1; i < line.size(); ++i) {
        if (line[i] == ":") break; // Stop before type annotation
        rule_body += line[i] + " ";
    }
    if (!rule_body.empty()) rule_body.pop_back(); // Remove trailing space

    Node* new_node;
    if (line.back() != ":" && line.size() > 2 && line[line.size() - 2] == ":") {
        std::string type = line.back();
        new_node = new Node(lhs, rule_body, type);
    } else {
        new_node = new Node(lhs, rule_body);
    }

    position++;

    if (isTerminal(lhs)) {
        return new_node;
    }

    for (size_t i = 1; i < line.size(); ++i) {
        const std::string& symbol = line[i];
        if (symbol == ".EMPTY" || symbol == ":") {
            break;
        }
        new_node->children.push_back(build_Tree(position, parsed_input));
    }
    return new_node;
}
Node* read_and_build_tree(int argc, char* argv[]) {
    std::ifstream fp;
    std::istream &in = 
        (argc > 1 && std::string(argv[1]) != "-")
      ? [&]() -> std::istream& {
            fp.open(argv[1]);
            return fp;
        }()
      : std::cin; 

    if(!fp && argc > 1)
    {
        return nullptr;
    }

    std::deque<std::vector<std::string>> parsed_input;
    std::string line;

    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::vector<std::string> temp;
        std::string word;
        while (ss >> word) {
            temp.push_back(word);
        }
        if (!temp.empty()) {
            parsed_input.push_back(temp);
        }
    }

    int position = 0;
    return build_Tree(position, parsed_input);
}

void push3() {
    std::cout << "sw $3, -4($30) ; PUSH " << std::endl;
    std::cout << "sub $30, $30, $4" << std::endl;
}
void pop5() {
    std::cout << "add $30, $30, $4 ; POP" << std::endl;
    std::cout << "lw $5, -4($30)" << std::endl;
}
void push(int reg) {
    std::cout << "sw $"<<reg <<", -4($30) ; PUSH" << std::endl;
    std::cout << "sub $30, $30, $4" << std::endl;
}
void pop(int reg) {
    std::cout << "add $30, $30, $4 ; POP" << std::endl;
    std::cout << "lw $"<<reg <<", -4($30)" << std::endl;
}
bool is_int_star(Node * lvalue){
    if (lvalue->lhs == "lvalue" && lvalue->rhs == "STAR factor"){
        return true; 
    }
    return false;
}
void prologue(){
    std::cout << std::endl; 
    std::cout<<"        ;begin wain prologue"<<std::endl; 
}
void epilogue(){ // should work for a 
    std::cout << std::endl; 
    std::cout<<"        ;begin "<< current_procedure.back() <<" epilogue"<<std::endl; 
    for (auto var: symbol_table[current_procedure.back()]){
        std::cout<<"add $30, $30, $4"<<std::endl; 
    }
    std::cout<<"jr $31"<<std::endl; 
}
void gen_expr(Node* expr) {
    std::string& rule = expr->rhs;
    if (expr->lhs == "expr" && rule == "expr PLUS term") {
        int temp_augment = expr_augment; 
        std::cout<<";expr_augment at PLUS: "<< expr_augment<<std::endl; 
        expr_augment = 0; 
        gen_expr(expr->children[0]); 
        push3();

        gen_expr(expr->children[2]); 
        pop5();
        std::string type1 = expr->children[0]->type; 
        std::string type2 = expr->children[2]->type; 
        
        // if the first one is a int* and it has a lvalue 
        if (type1 == "int*" && type2 == "int" ) { 
            std::cout<< "mult $3, $4; scale by by 4" << std::endl;
            std::cout<< "mflo $3" << std::endl;
            std::cout<< "add $3, $5, $3" << std::endl; 
        } else if (type1 == "int" && type2 == "int*"){
            std::cout<< "mult $5, $4; scale by by 4" << std::endl;
            std::cout<< "mflo $5" << std::endl;
            std::cout<< "add $3, $5, $3" << std::endl; 
        } else if (type1 == "int" && type2 == "int"){
            std::cout<< "add $3, $5, $3" <<std::endl;
        }
        if (temp_augment != 0 && !lhs_lval){
            std::cout<< "lw $3, 0($3) ; augment not zero at PLUS"<<std::endl; 
        }
        

    } else if (expr->lhs == "expr" && rule =="expr MINUS term") {
        int temp_augment = expr_augment; 
        std::cout<<";expr_augment at MINUS: "<< expr_augment<<std::endl; 

        expr_augment = 0; 

        gen_expr(expr->children[0]);
        push3();
        gen_expr(expr->children[2]);
        pop5();
        std::string type1 = expr->children[0]->type; 
        std::string type2 = expr->children[2]->type; 

        if (type1 == "int*" && type2 == "int") { 
            std::cout<< "mult $3, $4; scale by by 4" << std::endl;
            std::cout<< "mflo $3" << std::endl;
            std::cout<< "sub $3, $5, $3" << std::endl; 

        }else if (type1 == "int*" && type2 == "int*") { 
            std::cout<< "sub $3, $5, $3" <<std::endl;
            std::cout<< "div $3, $4; scale by by 4" << std::endl;
            std::cout<< "mflo $3" << std::endl;

        } else {
            std::cout<< "sub $3, $5, $3" <<std::endl;
        }

        if (temp_augment != 0 && !lhs_lval){
            std::cout<< "lw $3, 0($3) ; augment not zero at MINUS"<<std::endl; 
        }

    } else if (expr->lhs == "term" && rule =="term STAR factor") {
        gen_expr(expr->children[0]);
        push3();
        gen_expr(expr->children[2]);
        pop5();
        std::cout<< "mult $5, $3" <<std::endl;
        std::cout<< "mflo $3" <<std::endl;

    } else if (expr->lhs == "term" && rule =="term SLASH factor") {
        gen_expr(expr->children[0]);
        push3();
        gen_expr(expr->children[2]);
        pop5();
        std::cout<< "div $5, $3" <<std::endl;
        std::cout<< "mflo $3" <<std::endl;

    } else if (expr->lhs == "term" && rule =="term PCT factor") {

        gen_expr(expr->children[0]);
        push3();
        gen_expr(expr->children[2]);
        pop5();
        std::cout<< "div $5, $3" <<std::endl;
        std::cout<< "mfhi $3"<<std::endl;

    } else if ((expr->lhs =="expr" && rule =="term") ||(expr->lhs =="term" &&rule =="factor")) {
        expr_type = expr->type; 
        gen_expr(expr->children[0]);

    } else if (expr->lhs == "arglist" && rule=="expr"){
        gen_expr(expr->children[0]); // stored in $3
        push3(); 

    } else if (expr->lhs == "arglist" && rule =="expr COMMA arglist"){
        gen_expr(expr->children[0]);
        push3();
        gen_expr(expr->children[2]); 
    } else if (expr->lhs == "factor" && rule =="ID LPAREN RPAREN"){

        std::string proc_name = "P"+expr->children[0]->rhs; 
        current_procedure.push_back(proc_name); 
        push(29); 
        push(31); 
        std::cout<< "lis $3" << std::endl; 
        std::cout<< ".word "<<proc_name << std::endl; 
        std::cout<< "jalr $3"<<std::endl;  
        current_procedure.pop_back();
        pop(31); 
        pop(29); 
    }else if (expr->lhs == "factor" && rule =="NEW INT LBRACK expr RBRACK"){
        gen_expr(expr->children[3]); 
        push(29); 
        push(31); 
        std::cout<<"add $1, $3, $0"<<std::endl; 
        std::cout<<"lis $3"<<std::endl; 
        std::cout<<".word new"<<std::endl; 
        std::cout<<"jalr $3" <<std::endl;
        pop(31); 
        pop(29);  
        std::cout << "bne $3, $0, 2" << std::endl;
        std::cout << "lis $3"<<std::endl; 
        std::cout << ".word 1"<<std::endl; 


    }else if (expr->lhs == "factor" && rule =="ID LPAREN arglist RPAREN"){
        push(29); 
        push(31);
        func_arg_nest += 1; 
        gen_expr(expr->children[2]); //for the arguments to push to the stack 
        func_arg_nest -= 1; 
        std::string proc_name = "P"+expr->children[0]->rhs; 
        current_procedure.push_back(proc_name); 
        std::cout << "sub $29, $30, $4" << std::endl; 
        std::cout<< "lis $3" << std::endl; 
        std::cout<< ".word " << proc_name << std::endl; 
        std::cout<< "jalr $3"<<std::endl; 
        pop(31); 
        pop(29); 
        current_procedure.pop_back();

    }else if (expr->lhs == "factor" && rule =="GETCHAR LPAREN RPAREN"){
        std::cout<< "lis $5"<<std::endl; 
        std::cout<<".word 0xffff0004"<<std::endl; 
        std::cout<<"lw $3, 0($5)"<<std::endl; 

    } else if (expr->lhs == "factor" && expr->rhs == "lvalue") {
        gen_expr(expr->children[0]); 

    } else if (expr->lhs == "lvalue" && expr->rhs == "STAR factor"){
        expr_augment-= 1; 
        rhs_int_star = true; 
        gen_expr(expr->children[1]); 
        rhs_int_star = false; 

    } else if (expr->lhs == "factor" && expr->rhs == "STAR factor"){
        rhs_int_star = true; 
        expr_augment-= 1; 
        gen_expr(expr->children[1]); 
        rhs_int_star = false; 

    } else if (expr->lhs == "factor" && rule =="AMP lvalue"){ // to get the address in mips it's $29 + offset
        expr_augment += 1;
        gen_expr(expr->children[1]); 

    } else if (expr->lhs == "lvalue" && expr->rhs == "LPAREN lvalue RPAREN"){
        gen_expr(expr->children[1]); 

    } else if ((expr->lhs == "lvalue" && expr->rhs == "ID")||
                expr->lhs == "factor" && rule =="ID"){
        std::string variable_name = get_lvalue_id(expr); 
        int offset_by = symbol_table[current_procedure.back()][variable_name];
        std::string rhs_type = symbol_table_type[current_procedure.back()][variable_name]; 

        std::cout<<";expr_augment at the end: "<< expr_augment<<std::endl; 
       if (func_arg_nest){
            if (rhs_type == "int" && expr_augment == 0 && !lhs_lval){ 
                std::cout<< "lw $3, " << offset_by << "($29) ; just load it in, value to value"<<std::endl;
                
            }else if (rhs_type == "int" &&  expr_augment != 0 && !lhs_lval){ 
                std::cout<<"lis $3"<<std::endl; 
                std::cout<<".word "<< offset_by<<std::endl; 
                std::cout<<"add $3, $3, $29 ; address of variable " << variable_name << "in $3"<<std::endl; 

            }else if (rhs_type == "int*" && expr_augment == 0 && !lhs_lval){ 
                std::cout<< "lw $3, " << offset_by << "($29) ; just load it in address to address "<<std::endl;

            }else if (rhs_type == "int*" && expr_augment != 0 && !lhs_lval){ 
                std::cout<<"lw $3,"<< offset_by <<"($29)"<<std::endl; 
                std::cout<<"lw $3, 0($3) ; got value from address stored in variable "<< variable_name<<std::endl; 

            } else { // lhs 
                std::cout<< "lw $3, " << offset_by << "($29) ; Base case lhs"<<std::endl;
            }
       } else if ((!ptr && rhs_type == "int" ) && !lhs_int_star){ // 1. a = b 
            std::cout<< "lw $3, " << offset_by << "($29) ; lhs and rhs are INT, RHS varname: " << variable_name <<std::endl;

        } else if ((!ptr && rhs_type == "int*") && !lhs_int_star){ // 2. a = *c 
            std::cout<< "lw $3, " << offset_by << "($29) ; lhs is INT, RHS is INT*, RHS var_name: " << variable_name <<std::endl;
            std::cout<< "lw $3, 0($3)"<<std::endl; 

        } else if ((ptr && rhs_type == "int*") && !lhs_int_star){ // 3. d = c 
            std::cout<< "lw $3, " << offset_by << "($29) ; lhs and rhs are INT*, RHS varname: " << variable_name <<std::endl;

        } else if ((ptr && rhs_type == "int") && lhs_int_star){ // 4. *d = a 
            std::cout<< "lw $3, " << offset_by << "($29); lhs is INT* rhs is INT, RHS varname: " << variable_name <<std::endl;

        } else if ((ptr && rhs_type == "int") && !lhs_int_star ){ // 5. d = &b 
            std::cout<<"lis $3"<<std::endl; 
            std::cout<<".word " << offset_by<<std::endl; 
            std::cout<<"add $3, $3, $29 ; now $3 holds &rhs"<<std::endl; 

        } else if ((ptr && rhs_type == "int*") && lhs_int_star && rhs_int_star){ // 6. *d = *c 
            std::cout<<"lw $3, "<< offset_by<< "($29)"<<std::endl; 
            std::cout<<"lw $3, 0($3)"<<std::endl; 
        } else {
            std::cout<<"; something went horribly wrong in factor ID or lavalue ID"<<std::endl;
            std::cout<<";pointer: "<< ptr <<std::endl;
            std::cout<<";type: "<< rhs_type <<std::endl;
            std::cout<<";rhs_int_star: "<< rhs_int_star <<std::endl;
            std::cout<<";lhs_int_star: "<< lhs_int_star <<std::endl<<std::endl;
            
        }
        expr_augment = 0;
        ptr = false;
    } else if (expr->lhs == "factor" && rule =="NULL"){
        if (lhs_int_star){
            std::cout<< "lis $3" <<std::endl;
            std::cout<< ".word 1 ; "<< rule <<std::endl<<std::endl; 
              
        } else {
            std::cout<< "lis $3" <<std::endl;
            std::cout<< ".word 1 ;" << rule <<std::endl<<std::endl; 
        }

    } else if (expr->lhs == "factor" && rule =="NUM"){
        Node* child = expr->children[0];
        if (lhs_int_star){
            std::cout<< "lis $3" <<std::endl;
            std::cout<< ".word " << child->rhs <<std::endl<<std::endl; 
              
        } else {
            std::cout<< "lis $3" <<std::endl;
            std::cout<< ".word " << child->rhs <<std::endl<<std::endl; 
        }

    } else if (expr->lhs == "factor" && rule == "LPAREN expr RPAREN") {
        gen_expr(expr->children[1]); 
    }

}

void debug_print(Node* tree, int indent = 0) {
    for (int i = 0; i < indent; ++i) std::cout << "  ";
    std::cout << tree-> lhs<< " " << tree->rhs << " ";
    if (!tree->type.empty()) {
        std::cout << ": " << tree->type;
    }
    std::cout << std::endl;
    for (Node* child : tree->children) {
        debug_print(child, indent +1);
    }
}


std::string find_type(Node * dcl){
    if (dcl->lhs == "dcl" && dcl->rhs == "type ID"){
        return find_type(dcl->children[1]); 
    }else if (dcl->lhs=="ID"){
        return dcl->type; 
    } else if ((dcl->lhs =="lvalue" || dcl->lhs =="factor" ) && dcl->rhs == "ID"){
        return dcl->children[0]->type; 
    }else if (dcl->lhs =="lvalue" && dcl->rhs == "STAR factor"){
        return find_type(dcl->children[1]); 
    }else if (dcl->lhs =="dcl" && dcl->rhs == "type ID"){
        return find_type(dcl->children[1]); 
    }else if (dcl->lhs =="factor" && dcl->rhs == "AMP lvalue"){
        return find_type(dcl->children[1]);  
    } else if (dcl->lhs == "lvalue" && dcl->rhs == "LPAREN lvalue RPAREN" ){
        return find_type(dcl->children[1]); 
    } else if (dcl->lhs == "factor" && dcl->rhs == "LPAREN expr RPAREN" ){
        return find_type(dcl->children[1]); 
    } else if (dcl->lhs == "expr" && dcl->rhs == "term" ){
        return find_type(dcl->children[0]); 
    } else if (dcl->lhs == "term" && dcl->rhs == "factor" ){
        return find_type(dcl->children[0]); 
    } else if (dcl->lhs == "expr" && (dcl->rhs == "expr PLUS term" || dcl->rhs == "expr MINUS term")){
        if (dcl->children[0]->type == "int*" || dcl->children[2]->type == "int*" ){
            return "int*";
        }
        return "int"; 
    }
}

void push_param(Node * tree, int reg) {
    std::string id = tree->children[1]->rhs;
    std::cout<<"sw $"<<reg <<", -4($30)"<<std::endl; 
    std::cout<<"sub $30, $30, $4"<<std::endl;
    std::string type = find_type(tree);
    wain_add_symbol(id, current_procedure.back());
    symbol_table_type[current_procedure.back()][id] = type;
}

                        // dcl → type ID                    // the number 
void gen_initial_declaration(Node * tree, std::string proc, int& num){
    std::cout<<std::endl;
    std::string id = get_lvalue_id(tree); 

    std::cout<<"lis $3 ; load value"<<std::endl; 
    std::cout<<".word "<< num <<std::endl; 
    std::cout<<"sw $3, -4($30)"<<std::endl;  
    std::cout<<"sub $30, $30, $4"<<std::endl; 
    std::cout<<std::endl; 
    // symbol_table[proc][id] = offset; 
    add_symbol(proc, id, offset); 
    symbol_table_type[proc][id] = "int";
    offset -= 4;  
}

void gen_procedure_var(Node * tree, std::string proc, int& num){
    std::cout<<std::endl;
    std::string id = get_lvalue_id(tree);
    std::cout<<"lis $3 ; load local value"<<std::endl;
    std::cout<<".word "<< num <<std::endl;
    std::cout<<"sw $3, -4($30)"<<std::endl;
    std::cout<<"sub $30, $30, $4"<<std::endl;
    std::cout<<std::endl;
    // symbol_table[proc][id] = frame_pointer_offset; 
    add_symbol(proc, id, frame_pointer_offset); 
    symbol_table_type[proc][id] = "int"; 
    frame_pointer_offset -= 4; 
}

std::string get_lvalue_id(Node * tree){
    if ((tree->lhs =="lvalue" || tree->lhs =="factor" ) && tree->rhs == "ID"){
        return tree->children[0]->rhs; 

    }else if (tree->lhs =="lvalue" && tree->rhs == "STAR factor"){
        return get_lvalue_id(tree->children[1]); 

    }else if (tree->lhs =="ID"){
        return tree->rhs; 

    }else if (tree->lhs =="dcl" && tree->rhs == "type ID"){
        return get_lvalue_id(tree->children[1]); 

    }else if (tree->lhs =="factor" && tree->rhs == "AMP lvalue"){
        return get_lvalue_id(tree->children[1]);  

    } else if (tree->lhs == "lvalue" && tree->rhs == "LPAREN lvalue RPAREN" ){
        return get_lvalue_id(tree->children[1]); 
    }else {
        std::cerr << "[WARNING] get_lvalue_id() got non-ID node: "
                  << tree->lhs << " → " << tree->rhs << std::endl;
        return ""; 
    }

}

int get_num(Node* tree) { // pass a NUM -> 5
    return std::stoi(tree->rhs);  
}

bool gen_test(Node * tree, const std::string& body, const std::string& branch) {
    func_arg_nest += 1; 
    gen_expr(tree->children[0]);
    func_arg_nest -= 1; 
 
    push3();
    func_arg_nest += 1; 
    gen_expr(tree->children[2]); 
    func_arg_nest -= 1; 
    pop5();
   std::string oper = tree->children[1]->lhs;
   std::string type1 = tree->children[0]->type; 
   std::string type2 = tree->children[2]->type; 
   if (type1 == "int" && type2 == "int"){
        if (oper == "EQ" ) {
            std::cout<<"bne $3, $5, " << branch <<std::endl;
            return true;
        } else if (oper == "NE"){
            std::cout<<body<<":"<<"beq $3, $5, " << branch <<std::endl;
            return true;
        } else if (oper == "LT"){
            std::cout<< "slt $6, $5, $3" <<std::endl;
            std::cout<<"beq $6, $0, " << branch <<std::endl;
            return true;
        } else if (oper =="GT"){
            std::cout<< "slt $6, $3, $5" << std::endl;
            std::cout<<"beq $6, $0, " << branch <<std::endl;
            return true;
        } else if (oper =="LE"){
            std::cout<<"slt $6, $3, $5" <<std::endl;
            std::cout<<"sub $6, $11, $6" << std::endl;
            std::cout<<"beq $6, $0, " <<branch <<std::endl;
            return true;
        } else if (oper =="GE"){
            std::cout<<"slt $6, $5, $3" << std::endl;
            std::cout<<"sub $6, $11, $6" << std::endl;
            std::cout<<"beq $6, $0, " << branch <<std::endl;
            return true;
        }
   } else if (type1 == "int*" && type2 == "int*"){
        if (oper == "EQ" ) {
            std::cout<<"bne $3, $5, " << branch <<std::endl;
            return true;
        } else if (oper == "NE"){
            std::cout<<body<<":"<<"beq $3, $5, " << branch <<std::endl;
            return true;
        } else if (oper == "LT"){
            std::cout<< "sltu $6, $5, $3" <<std::endl;
            std::cout<<"beq $6, $0, " << branch <<std::endl;
            return true;
        } else if (oper =="GT"){
            std::cout<< "sltu $6, $3, $5" << std::endl;
            std::cout<<"beq $6, $0, " << branch <<std::endl;
            return true;
        } else if (oper =="LE"){
            std::cout<<"sltu $6, $3, $5" <<std::endl;
            std::cout<<"sub $6, $11, $6" << std::endl;
            std::cout<<"beq $6, $0, " <<branch <<std::endl;
            return true;
        } else if (oper =="GE"){
            std::cout<<"sltu $6, $5, $3" << std::endl;
            std::cout<<"sub $6, $11, $6" << std::endl;
            std::cout<<"beq $6, $0, " << branch <<std::endl;
            return true;
        }
   }
    return false; 
}
std::string get_proc_name(Node * tree){
    return tree->children[1]->rhs; 
}
std::string get_variable_name(Node * tree){

} 

void gen_params_setup(Node * paramlist){
    if (paramlist->lhs == "params" && paramlist->rhs == "paramlist" ){
        gen_params_setup(paramlist->children[0]); // the child is the actual list 
    } else if (paramlist->lhs == "dcl" && paramlist->rhs == "type ID"){
        add_offset(current_procedure.back()); 
        Node * type = paramlist->children[1]; 
        std::string id = paramlist->children[1]->rhs;
        std::string temp = find_type(type); 
        symbol_table_type[current_procedure.back()][id] = temp; 
    } else if (paramlist->rhs == "dcl COMMA paramlist"){
        gen_params_setup(paramlist->children[0]);
        gen_params_setup(paramlist->children[2]);
    } else if (paramlist->rhs == "dcl"){
        gen_params_setup(paramlist->children[0]);
    }
}

void gen_symbol_setup(Node * paramlist){
    if (paramlist->rhs == "paramlist"){
        gen_symbol_setup(paramlist->children[0]);// the child is the actual list 
    } else if (paramlist->lhs == "dcl" && paramlist->rhs == "type ID"){
        std::string var_name = paramlist->children[1]->rhs; 
        add_symbol(current_procedure.back(), var_name, arglist_offset); 
        arglist_offset -= 4; 
    } else if (paramlist->rhs == "dcl COMMA paramlist"){
        gen_symbol_setup(paramlist->children[0]);
        gen_symbol_setup(paramlist->children[2]);
    } else if (paramlist->rhs == "dcl"){
        gen_symbol_setup(paramlist->children[0]);
    }
}

void proc_epilogue(){
    std::cout << std::endl; 
    std::cout<<"        ;begin "<< current_procedure.back() <<" epilogue"<<std::endl; 
    for (auto var: symbol_table[current_procedure.back()]){
        std::cout<<"add $30, $30, $4 ; pop parameter"<<std::endl; 
    }
    std::cout<<"jr $31"<<std::endl; 
}

void generate_functionality(Node * tree){
    if (!tree){
        return; 
    }
    if (tree->lhs == "procedure" && tree->rhs == "INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE"){
        current_procedure.push_back("P"+get_proc_name(tree)); 
        std::cout << std::endl; 
        std::cout << current_procedure.back() << ":" << std::endl;
        if (tree->children[3]->children.size() > 0){
            gen_params_setup(tree->children[3]); 
            arglist_offset = procedure_param_offset[current_procedure.back()];
            gen_symbol_setup(tree->children[3]); 
        }
        // decarations to be used in the procedure (dcls), need to adjust offset 
        generate_functionality(tree->children[6]); 
        generate_functionality(tree->children[7]); 
        func_arg_nest +=1;
        gen_expr(tree->children[9]); 
        func_arg_nest -=1;
        proc_epilogue(); 
        current_procedure.pop_back(); 
        frame_pointer_offset = 0; 

    } else if (tree->lhs == "main"){
    // main → INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE 
        // store the parameters in a place 
        current_procedure.push_back("Pwain");
        prologue(); 
        
        std::cout << std::endl; 
        std::cout << current_procedure.back()<< ":"<< std::endl;
        push_param(tree->children[3], 1); 
        push_param(tree->children[5], 2);

        if (tree->children[3]->children[0]->rhs != "INT STAR"){
            std::cout <<"add $2, $0, $0 "<<std::endl; 
        }

        push(31);
        std::cout <<"lis $3"<<std::endl; 
        std::cout << ".word init"<<std::endl; 
        std::cout << "jalr $3"<<std::endl; 
        pop(31); 

        std::cout<<"sub $29, $30, $4"<<std::endl; // for the frame pointer
        std::cout<<"; declarations --> "<<std::endl;   

        generate_functionality(tree->children[8]);
        std::cout<<"; statements -->"<<std::endl; 

        generate_functionality(tree->children[9]);

        std::cout<<"; return -->"<<std::endl; 
        func_arg_nest +=1;
        gen_expr(tree->children[11]); 
        func_arg_nest -=1;
        epilogue();

    }else if (tree->lhs == "dcls" && tree->children.size() > 0){
        if (tree->rhs == "dcls dcl BECOMES NUM SEMI"){
            generate_functionality(tree->children[0]); // for more statements 
            int num = get_num(tree->children[3]);

            if (current_procedure.back() == "Pwain"){
                gen_initial_declaration(tree->children[1], current_procedure.back(), num); 
            } else {
                gen_procedure_var(tree->children[1], current_procedure.back(), num); 
            }

            ptr = false; 
        } else if (tree->rhs == "dcls dcl BECOMES NULL SEMI") {
            generate_functionality(tree->children[0]);
            std::string id = get_lvalue_id(tree->children[1]); 
            std::cout<<std::endl;
            std::cout<<"lis $3      ; load initial value for ptr"<<std::endl; 
            std::cout<<".word 1"<<std::endl; 
            std::cout<< "sw $3, -4($30)"<<std::endl; 
            std::cout<< "sub $30, $30, $4"<<std::endl;
            std::cout<<std::endl;
            if (current_procedure.back() == "Pwain"){
                add_symbol("Pwain", id, offset);
                //symbol_table["Pwain"][id] = offset; 
                symbol_table_type["Pwain"][id] = "int*"; 
                offset -= 4; 
            }else {
                add_symbol(current_procedure.back(), id, frame_pointer_offset);
                //symbol_table[current_procedure.back()][id] = frame_pointer_offset; 
                symbol_table_type[current_procedure.back()][id] = "int*"; 
                frame_pointer_offset -= 4; 
            }
        }

    } else if (tree->lhs =="statement" && tree->rhs.find("WHILE") != std::string::npos){
        //statement → WHILE LPAREN test RPAREN LBRACE statements RBRACE

        std::string start =increment_label("while");
        std::string body = increment_label("body");
        std::string end = increment_label("endwhile");
        std::cout<< start<< ":"<<std::endl;
        gen_test(tree->children[2], body, end);
        generate_functionality(tree->children[5]); // the body 
        std::cout<< "beq $0, $0, " << start <<std::endl; // back up top
        std::cout<<end<<":"<<std::endl;


    } else if (tree->lhs =="statement" && tree->rhs.find("IF") != std::string::npos){
// statement → IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE
        std::string if_true = increment_label("ifTrue"); 
        std::string if_false = increment_label("else"); 
        std::string end = increment_label("endif"); 

        gen_test(tree->children[2], if_true, if_false); // it'll continue if true, if not jump to if_false
        generate_functionality(tree->children[5]);
        std::cout<<"beq $0, $0, "<< end<<std::endl;
        std::cout<< if_false<< ":"<<std::endl;
        generate_functionality(tree->children[9]);
        std::cout<<end<<":"<<std::endl;

    } else if (tree->lhs =="statement" && tree->rhs == "DELETE LBRACK RBRACK expr SEMI"){
        func_arg_nest += 1;
        gen_expr(tree->children[3]); 

        func_arg_nest -= 1;
        std::cout <<"beq $11, $3, 12"<<std::endl;
        push(29); 
        push(31); 
        std::cout <<"add $1, $3, $0"<<std::endl; // 1 line 5
        std::cout <<"lis $3"<<std::endl; // 1 line 6
        std::cout <<".word delete"<<std::endl; // 1 line  7
        std::cout<<"jalr $3"<<std::endl;  // 1 line 8
        pop(31);// 2 lines  12
        pop(29); // 2 lines 10

    


        
    } else if (tree->lhs =="statement" && tree->rhs == "PUTCHAR LPAREN expr RPAREN SEMI"){
        func_arg_nest += 1; 
        gen_expr(tree->children[2]);
        func_arg_nest -= 1; 
        std::cout<<"lis $5"<<std::endl; 
        std::cout<<".word 0xffff000c"<<std::endl; 
        std::cout<<"sw $3, 0($5)"<<std::endl; 

    } else if (tree->lhs =="statement" && tree->rhs == "PRINTLN LPAREN expr RPAREN SEMI"){     //LGTM q2
        std::cout<<"sw $1, -4($30)"<<std::endl; 
        std::cout<<"sub $30, $30, $4"<<std::endl; 
        func_arg_nest += 1; 
        gen_expr(tree->children[2]); 
        func_arg_nest -= 1; 
        std::cout<<"add $1, $3, $0"<<std::endl; 
        std::cout<<"sw $31, -4($30)"<<std::endl; 
        std::cout<<"sub $30, $30, $4"<<std::endl; 
        std::cout<<"lis $5"<<std::endl;
        std::cout<< ".word print"<<std::endl; 
        std::cout<<"jalr $5"<<std::endl; 
        std::cout<<"add $30, $30, $4"<<std::endl; 
        std::cout<<"lw $31, -4($30)"<<std::endl;
        std::cout<<"add $30, $30, $4"<<std::endl; 
        std::cout<<"lw $1, -4($30)"<<std::endl; 

    } else if (tree->lhs =="statements" && tree->rhs == "statements statement"){              // LGTM q2
        //statements → statements statement
        generate_functionality(tree->children[0]);
        generate_functionality(tree->children[1]);
    }else if (tree->lhs =="statement" && tree->rhs =="lvalue BECOMES expr SEMI"){      // ajusting for q2 
        std::string type = find_type(tree->children[0]); 
        ptr = (type == "int*") ? true : false;  

        lhs_int_star = is_int_star(tree->children[0]); 

        if (tree->children[0]->rhs == "STAR factor"){
            lhs_lval = true; 
            func_arg_nest += 1; 
            gen_expr(tree->children[0]);
            func_arg_nest -= 1;
            lhs_lval = false; 
            push3(); 
            func_arg_nest += 1; 
            gen_expr(tree->children[2]);
            func_arg_nest -= 1; 
            pop5(); 
            std::cout<<"sw $3 , 0($5)"<<std::endl; 
        } else {
            std::string id = get_lvalue_id(tree->children[0]); 
            int temp_offset = symbol_table[current_procedure.back()][id]; 
            func_arg_nest += 1; 
            gen_expr(tree->children[2]);
            func_arg_nest -= 1; 
            std::cout<<"sw $3 , " << temp_offset<<"($29)" <<"; store in $29 + " << temp_offset <<std::endl<<std::endl;
        }
        
        lhs_int_star = false; 
        ptr = false;

    }else {
        for(Node* child: tree->children){
            generate_functionality(child); 
        }
    }
}

void generate_mips(Node * tree){
    std::cout<<".import print"<<std::endl;
    std::cout<<".import init"<<std::endl;
    std::cout<<".import new"<<std::endl;
    std::cout<<".import delete"<<std::endl;
    std::cout<<"lis $4"<<std::endl;
    std::cout<<".word 4"<<std::endl;
    std::cout<<"lis $11"<<std::endl; 
    std::cout<<".word 1"<<std::endl; 
    std::cout<<"beq $0, $0, Pwain"<<std::endl;
    generate_functionality(tree); 
}



int main(int argc, char* argv[]) {
    try {
        Node* tree = read_and_build_tree(argc, argv);
        if (tree) {
            generate_mips(tree); 
            printSymbolTable(); 
        }
        delete tree;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
