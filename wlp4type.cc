#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map> 
#include <vector> 
#include <deque> 
#include <set>

// Global Variables 
std::unordered_map<std::string, std::unordered_map<std::string, std::string>> procedure_variable_table; 
std::unordered_map<std::string, std::vector<std::string> > procedure_params; 
std::vector<std::string> temp_arglist; 
std::string current_procedure = ""; 

bool error = false; 
int nest = 0; 
bool procedure_exists(std::string proc_name){
    if (procedure_params.count(proc_name)){
        return true; 
    }
    return false; 
}

// true if success false otherwise 
bool var_exists(const std::string& proc, const std::string& id){
    // if it exists 
    if (procedure_variable_table[proc].count(id)){
        return true; 
    }
    return false; 
}

std::string get_var(const std::string& proc, const std::string& id){
    return procedure_variable_table[proc][id]; 
}

bool isLexeme(const std::string word) {
    std::set<std::string> terminals = {
        // ##########################################################################################
        "BOF", "EOF", "INT", "WAIN", "RETURN", "SEMI", "COMMA", "LPAREN", "RPAREN", "RBRACK", "LBRACK",
        "LBRACE", "RBRACE", "ID", "NUM", "NULL", "AMP", "STAR", "SLASH", "PCT", "PLUS", "MINUS", 
        "BECOMES", "EQ", "NE", "LT", "GT", "LE", "GE", "IF", "ELSE", "WHILE", "NEW", "DELETE",
         "GETCHAR", "PRINTLN", "PUTCHAR"
        // ##########################################################################################
    };
    return terminals.count(word) > 0;
}



struct Node {
    std::vector<std::string> rule;  
    std::vector<Node*> children; 
    std::string type;           
    std::string lexeme;  


    bool isEqual(const std::vector<std::string>& otherRule) const {
        return rule == otherRule;
    }

    Node(const std::vector<std::string>& rule) : rule(rule), type(""), lexeme("") {}
    ~Node() {
        for (auto child: children){
            delete child;
        }
    }   
}; 

std::string get_name(Node * tree){
    return tree->children[0]->rule[1]; 
}
std::string get_proc_name(Node * tree){
    return tree->children[1]->rule[1]; 
}

bool isInt(Node * node){
    return "int" == node->type; 
}

void print_Tree(Node* tree){
    for(auto r: tree->rule){
        std::cout<<r<<" ";
    }
    if (!tree->type.empty() ){
        std::cout<<": "<< tree->type;
    }
    std::cout<<std::endl; 
    for (auto node: tree->children){
        print_Tree(node);  
    }
}

Node * build_Tree(int & position, std::deque<std::vector<std::string>> parsed_input){
    //std::cout<<position<<std::endl;
    std::vector<std::string>& line = parsed_input[position];
    std::string lhs; 
    if (!line.empty()){
        lhs = line[0]; 
    } else {
        std::cerr<<"ERROR empty line";
    }
    Node* new_node = new Node(line);
    // if it's a Terminal  
    if( (line.size() == 2 && isLexeme(lhs))){
        position++; 
        new_node->lexeme = line[1]; 
        return new_node; 

        // If it's Non-Terminal 
    } else {
        position++;  
        for (size_t i = 1; i < line.size(); ++i) {  
            if (new_node->rule[i] == ".EMPTY") {
                continue;  
            }
            new_node->children.push_back(build_Tree(position, parsed_input));
        }
        return new_node; 
    }
    std::cerr<<"   ERROR    "; 
    return new Node({"THERE IS AN ERROR"});
}

std::string annotate_dcl(Node * tree){
        std::string type;
        std::string id;

        Node* type_node = tree->children[0];
        Node* id_node = tree->children[1];

        if (type_node->isEqual({"type", "INT", "STAR"})) {
            type = "int*";
        } else {
            type = "int";
        } 
        id = id_node->lexeme;  

        if (!var_exists(current_procedure, id)){
            procedure_variable_table[current_procedure][id] = type; 
        } else {
            std::cerr << "ERROR: variable name repeated: " << id << std::endl;
            error = true;
        }
        id_node->type = type;
    return type; 
}
void handle_paramlist(Node* node, std::vector<std::string>& temp) {
    // paramlist → dcl
    if (node->children.size() == 1) {
        temp.push_back(annotate_dcl(node->children[0]));
    }
    else if (node->children.size() == 3) {
        temp.push_back(annotate_dcl(node->children[0]));
        handle_paramlist(node->children[2], temp);
    }
}
void set_params(Node* params, std::vector<std::string>& temp) {
    // params → paramlist
    if (params->children.size() == 1){
        Node* paramlist = params->children[0];
        handle_paramlist(paramlist, temp);
    }
}

int get_arglist_length(Node * arglist){
    if (arglist->children.size() == 1 && arglist->rule[1] == "expr"){
        return 1; 
    }else if( arglist->children.size() == 3 ){
        return 1 + get_arglist_length(arglist->children[2]); 
    }
}




std::string findType(Node * tree){
    if (tree->isEqual({"factor", "NUM"})){
        tree->children[0]->type = "int"; 
        tree->type = "int"; 
        return "int";
    } else if (tree->isEqual({"NULL", "NULL"})){
        tree->type = "int*"; 
        return "int*"; 
    } else if (tree->rule[0] == "NUM"){
        tree->type = "int"; 
        return "int"; 
    } else if (tree->isEqual({"factor", "NULL"})){
        tree->type = tree->children.size() > 0? findType(tree->children[0]): "int*" ; 
        return "int*";

    } else if (tree->isEqual({"factor", "ID", "LPAREN", "arglist", "RPAREN"})){
        // populate arglist first 
        std::string proc_id = get_name(tree); 
        if (!procedure_exists(proc_id)){
            std::cerr<<"ERROR : use of procedure before declaration"<<std::endl; 
            error = true; 
            return "int"; 
        } else {
            Node * arglist = tree->children[2]; 
            int len = get_arglist_length(arglist);
            if (len == procedure_params[proc_id].size()){
                if (arglist->children.size() == 1){
                    temp_arglist.push_back(findType(arglist->children[0])); 
                } else {
                    temp_arglist.push_back(findType(arglist->children[0])); 
                    findType(arglist->children[2]);  
                }

                for (int i = len-1; i >= 0; --i){
                    std::string type = temp_arglist.back();
                    temp_arglist.pop_back(); 
                    if (type != procedure_params[proc_id][i]){
                        std::cerr<<"ERROR : arguments passed don't match defined use of procedure!"<<std::endl; 
                        std::cerr <<"proc_id: "<<proc_id<<std::endl;
                        std::cerr<<"current_procedure: "<<current_procedure<<std::endl;
                        std::cerr<<"length: " <<len<<std::endl;
                        std::cerr<<"i: " <<i<<std::endl;
                        std::cerr<<"temp_arg len: " <<temp_arglist.size()<<std::endl;
                        error = true; 
                        return "int"; 
                    }

                }

            } else {
                std::cerr<<"ERROR : arguments passed don't match defined use of procedure"<<std::endl; 
                std::cerr<<get_arglist_length(arglist)<<std::endl;
                std::cerr<<(int)procedure_params[proc_id].size()<<std::endl;
                
            }
            tree->type = "int"; 
            return "int"; 
        }
        findType(tree->children[2]); 
        // ASSUMING we know what procedure we are in the current_procedure varirable


    }else if (tree->isEqual({"factor", "ID", "LPAREN", "RPAREN"})){
        std::string proc_id = get_name(tree); 
        if (!procedure_exists(proc_id)){
            std::cerr<<"ERROR : use of procedure before declaration"<<std::endl;
            error = true; 
            return "int"; 
        } else if (procedure_params[proc_id].size() > 0){
            std::cerr<<"ERROR : no arguments given for procedure that requires parameters"<<std::endl;
            error = true; 
            return "int"; 
        }else {
            tree->type = "int"; 
            return "int"; 
        }

    }else if (tree->isEqual({"factor", "GETCHAR" , "LPAREN", "RPAREN"})){
        tree->type = "int";
        // not really sure what's suppose to go here
        return "int"; 
    }else if (tree->rule[0] == "test" && (tree->rule[2] == "EQ" || tree->rule[2] == "NE")) {
        std::string lhs = findType(tree->children[0]); 
        std::string rhs = findType(tree->children[2]); 
        if (lhs != rhs ){
            std::cerr<<"ERROR: In rule [test expr EQ or NE expr]: exprs have different types"<<std::endl; 
            error = true; 
            return ""; 
        }else if (lhs == rhs && lhs == "int*"){
            return "int*";
        } else {
            return "int";
        }
    }else if (tree->rule[0] == "test" && (tree->rule[2] == "LT" ||
     tree->rule[2] == "LE" || 
     tree->rule[2] == "GE" || 
     tree->rule[2] == "GT")) {
        std::string lhs = findType(tree->children[0]); 
        std::string rhs = findType(tree->children[2]); 
        if (lhs != rhs ){
            std::cerr<<"ERROR: In rule [test expr GT/LT/ expr]: exprs have different types"<<std::endl; 
            error = true; 
            return "int*";
        } else {
            return "int";
        }

    }else if (tree->isEqual({"arglist", "expr"})){
        // needs to be here since it has expressions which need to be handled 
        std::string type = findType(tree->children[0]); 
        temp_arglist.push_back(type);
        return type; 
    }else if (tree->isEqual({"arglist", "expr", "COMMA","arglist"})){ 
        temp_arglist.push_back(findType(tree->children[0])); 
        findType(tree->children[2]); 
        return ""; 
    }else if (tree->isEqual({"factor", "ID"})){
        std::string id = tree->children[0]->rule[1]; 
        if (var_exists(current_procedure, id)){
            tree->type = get_var(current_procedure, id); 
            tree->children[0]->type = get_var(current_procedure, id); 
        }else {
            std::cerr<<"ERROR : undeclared variable"<<std::endl; 
            error = true; 
        }
        return tree->type;

    }else if (tree->isEqual({"factor", "LPAREN", "expr", "RPAREN"})){
        tree->type = findType(tree->children[1]);
        return tree->type; 
    } else if (tree->isEqual({"factor", "NEW", "INT", "LBRACK", "expr", "RBRACK"})) {
        std::string temp = findType(tree->children[3]);
        if (temp != "int") {
            std::cerr << "ERROR : " << std::endl;
            error = true; 
        }
        tree->type = "int*";
        return "int*";
    } else if (tree->isEqual({"factor", "AMP", "lvalue"})) {
        std::string temp = findType(tree->children[1]);
        if (temp != "int") {
            std::cerr << "ERROR : &a where a is already int*" << std::endl;
            error = true; 
        }
        tree->type = "int*";
        return "int*";
    } else if (tree->isEqual({"factor", "STAR", "factor"})) {
        std::string temp = findType(tree->children[1]);
        if (temp != "int*") {
            std::cerr << "ERROR: dereferencing from int" << std::endl;
            error = true; 
        }
        tree->type = "int";
        return "int"; 
    } else if (tree->isEqual({"factor", "LPAREN", "expr", "RPAREN"})) {
        tree->type = findType(tree->children[1]);
        return tree->type;
    } else if (tree->isEqual({"term", "term", "STAR", "factor"}) ||
               tree->isEqual({"term", "term", "SLASH", "factor"}) ||
               tree->isEqual({"term", "term", "PCT", "factor"})) {
        std::string left = findType(tree->children[0]);
        std::string right = findType(tree->children[2]);
        if (left != "int" || right != "int") {
            std::cerr << "ERROR" << std::endl;
            error = true; 
        }
        tree->type = "int";
        return "int";
    } else if (tree->isEqual({"term", "factor"}) ||
        tree->isEqual({"expr", "term"})){
        tree->type = findType(tree->children[0]);
        return tree->type; 
    } else if (tree->isEqual({"expr", "expr", "PLUS", "term"})){
        findType(tree->children[0]); 
        std::string term_type = findType(tree->children[2]); 
        if (term_type == "int*" && term_type == tree->children[0]->type){
            std::cerr<<"ERROR : pointer plus pointer ";
            error = true; 
        } else if (term_type != tree->children[0]->type){
            tree->type = "int*"; 
            return "int*";
        } else {
            tree->type = "int"; 
            return "int";
        }
    } else if (tree->isEqual({"expr", "expr", "MINUS", "term"})){
        findType(tree->children[0]); 
        std::string term_type = findType(tree->children[2]); 
            if ((term_type == "int*") && ("int" == tree->children[0]->type)){
                std::cerr<<"ERROR : integer minus pointer ";
                error = true; 
            } else if ((term_type == "int*") && ("int*" == tree->children[0]->type)){
                tree->type = "int"; 
                return "int";
            } else {
                tree->type = "int*"; 
                    return "int*";
            }
    } else if (tree->rule[0] == "lvalue") {
        if (tree->isEqual({"lvalue", "ID"})) {
            std::string id = tree->children[0]->rule[1]; 
            if (var_exists(current_procedure,id)){
                tree->type = get_var(current_procedure, id);
                tree->children[0]->type = get_var(current_procedure, id);
            }else {
                std::cerr<<"ERROR : undeclared variable"<<std::endl; 
                error = true; 
            }
            return tree->type;

        } else if (tree->isEqual({"lvalue", "STAR", "factor"})) {
            std::string l_type = findType(tree->children[1]);
            if (l_type != "int*") {
                std::cerr << "ERROR : int dereferencing" << std::endl;
                error = true; 
            }
            tree->type = "int";
            return "int";

        } else if (tree->isEqual({"lvalue", "LPAREN", "lvalue", "RPAREN"})) {
            tree->type = findType(tree->children[1]);
            return tree->type;
        }
    }
}





void annotateTypes(Node * tree){
    // declarations
    if (error || tree == NULL){
        return;
    }
            // handling procedures 
    if (tree->rule[0] == "procedures" && tree->rule[1] == "procedure" ){
            for (Node * child: tree->children) {
                    annotateTypes(child);
            }
            // the above ^ technically redundant but lets be specific for now 
    } else if (tree->rule[0] == "procedure" && tree->rule[1] == "INT"){
       // std::cout<<" Here 2"<<std::endl;
        
        std::string proc_name = get_proc_name(tree);
        if (procedure_exists(proc_name)){
            std::cerr<< "ERROR : double declaration of procedure"<<std::endl; 
            error = true; 
            return; 
        } else {
            current_procedure = proc_name; 
            std::vector<std::string> temp; 
            set_params(tree->children[3], temp); 
            procedure_params[current_procedure] = temp;
        }
        //temp_arglist.clear(); 
        for (u_int i = 4; i < tree->children.size(); i++){
            annotateTypes(tree->children[i]); 
        }
        if (tree->children[9]->type != "int"){
            std::cerr<<"ERROR : the return type is not int"<<std::endl;
            error = true;
        }

    } else if (tree->rule[0] == "procedures" && tree->rule[1] == "main"){
        current_procedure = "wain"; 
        for (Node * child: tree->children){
                annotateTypes(child); 
        }

    } else if (tree->isEqual({"dcl", "type", "ID"})){
        std::string type = annotate_dcl(tree); 

        // for expressions 
    } else if (tree->isEqual({"dcls", "dcls", "dcl", "BECOMES", "NULL", "SEMI"})){
        // for the dcls case 
        annotateTypes(tree->children[0]); 
        std::string dcl_type = annotate_dcl(tree->children[1]); 
        std::string val_type = findType(tree->children[3]); 
        if (dcl_type != val_type){
            std::cerr<<"ERROR : mismatch between variable ["+tree->children[1]->children[1]->rule[1]+"] declaration and assignment value type"<<std::endl; 
            error = true; 
            return; 
        }
                    // statement → lvalue BECOMES expr SEMI
    }else if ( tree->rule[0] == "statement" && tree->rule[1] == "lvalue") {
        std::string lhs = findType(tree->children[0]);
        std::string rhs = findType(tree->children[2]);
        if (lhs != rhs) {
            std::cerr << "ERROR : assignment type mismatch"<<std::endl;
            error = true;
            return; 
        }
    // statement → IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE
    }else if (tree->rule[0] == "statement" && tree->rule[1] == "IF") {
        std::string temp = findType(tree->children[2]);
        if (temp != "int*") {
            std::cerr << "ERROR: delete requires int* type"<<std::endl;
            error = true;
            return; 
        }
        annotateTypes(tree->children[5]); 
        annotateTypes(tree->children[9]); 

    //statement → WHILE LPAREN test RPAREN LBRACE statements RBRACE
    }else if (tree->rule[0] == "statement" && tree->rule[1] == "WHILE") {
        std::string temp = findType(tree->children[2]);
        annotateTypes(tree->children[5]); 

//PRINTLN LPAREN expr RPAREN SEMI           PUTCHAR LPAREN expr RPAREN SEMI
    }else if (tree->rule[0] == "statement" && (tree->rule[1] == "PRINTLN" || tree->rule[1] == "PUTCHAR")) {
        std::string temp = findType(tree->children[2]);
        if (temp != "int") {
            std::cerr << "ERROR: IO operations require int"<<std::endl;
            error = true;
            return; 
        }

        //DELETE LBRACK RBRACK expr SEMI
    } else if (tree->rule[0] == "statement" && tree->rule[1] == "DELETE") {
        std::string temp = findType(tree->children[3]);
        if (temp != "int*") {
            std::cerr << "ERROR: delete requires int* type\n";
            error = true;
            return; 
        }
    } else if (tree->isEqual({"expr", "expr", "PLUS", "term"})){
        annotateTypes(tree->children[0]); 
        std::string term_type = findType(tree->children[2]); 
        if ((term_type == "int*") && (term_type == tree->children[0]->type)){
            std::cerr<<"ERROR : pointer plus pointer ";
            error = true; 
            return; 
        } else if (term_type != tree->children[0]->type){
            tree->type = "int*"; 
        } else {
            tree->type = "int"; 
        }
    } else if (tree->isEqual({"expr", "expr" ,"MINUS", "term"})){
        annotateTypes(tree->children[0]); 
        std::string term_type = findType(tree->children[2]); 
            if ((term_type == "int") && ("int*" == tree->children[0]->type)){
                std::cerr<<"ERROR : integer minus pointer ";
                error = true; 
                return; 
            } else if ((term_type == "int*") && ("int*" == tree->children[0]->type)){
                tree->type = "int"; 
            } else if ((term_type == "int") && ("int" == tree->children[0]->type)){
                tree->type = "int"; 
            } else {
                tree->type = "int*"; 
            }
    } else if (tree->isEqual({"expr", "term"})){
        tree->type = findType(tree->children[0]); 
    
    } else if (tree->isEqual({"NULL", "NULL"})){
        tree->type = "int*"; 
    } else if (tree->rule[0] == "NUM"){
        tree->type = "int"; 
    } else {
        for (Node * child: tree->children){
            annotateTypes(child);  
        }
    }
}

void check_second_param_wain(Node * tree){
    if (tree->isEqual({"procedures", "main"})){
        if ("int" != tree->children[0]->children[5]->children[1]->type){
            std::cerr<<"ERROR : second param wain is not int"<<std::endl; 
        }
        
    } else {
        for (Node * child: tree->children){
            check_second_param_wain(child); 
        }
    }
}


void check_return_int(Node * tree){
    if (tree->rule[0] == "main" && tree->rule.size() >= 13 && tree->rule[11] == "RETURN"){
        if ("int" != tree->children[11]->type){
            std::cerr<<"ERROR : wain return is not of type int"<<std::endl; 
        }
    } else {
        for (Node * child: tree->children){
            check_return_int(child); 
        }
    }
}

void debug_print(Node* tree, int indent = 0) {
    for (int i = 0; i < indent; ++i) std::cout << "  ";
    for (const auto& token : tree->rule) {
        std::cout << token << " ";
    }
    if (!tree->type.empty()) {
        std::cout << ": " << tree->type;
    }
    std::cout << std::endl;
    for (Node* child : tree->children) {
        debug_print(child, indent +1);
    }
}


int main(int argc, char * argv[]) {
    std::string line;
    std::ifstream fp; // input file stream
    std::istream &in1 = 
        (argc > 1 && std::string(argv[1]) != "-")
      ? [&]() -> std::istream& {
            fp.open(argv[1]);
            return fp;
        }()
      : std::cin; 

    if(!fp && argc > 1)
    {
        return 1;
    }

    std::deque<std::vector<std::string>> parsed_input; 
    while(std::getline(in1, line)){
        std::stringstream ss(line); 
        std::string word; 
        std::vector<std::string> temp; 
        while(ss>>word){
            temp.emplace_back(word); 
        }
        parsed_input.emplace_back(temp); 
    }

    int position = 0; 
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> prodedure_variable_table; 

    Node * tree = build_Tree(position,parsed_input);
    annotateTypes(tree); 
    //debug_print(tree);
    if (!error){
        check_second_param_wain(tree); 
        check_return_int(tree); 
        print_Tree(tree); 
    }

    delete tree; 
}