#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <regex>

#include <thread>
#include <chrono>

std::vector<std::string> hw1_instructions = {
    "ignore $r6 ignore ignore ignore",
    "mpy $r6 = $r4, $r8",
    "ldw $r0 = 8[$r6]",
    "add $r8 = $r5, $r6",
    "ldw $r6 = 8[$r5]",
    "add $r6 = $r8, $r7",
    "add $r5 = $r8, $r2",
    "mpy $r1 = $r0, $r4",
    "ldw $r8 = 12[$r6]",
    "sub $r1 = $r8, $r0",
    "sub $r8 = $r5, $r7",
    "add $r8 = $r6, $r8",
    "sub $r2 = $r5, $r2",
    "sub $r1 = $r1, $r8",
    "ldw $r3 = 8[$r3]",
    "add $r6 = $r3, $r6",
};

// overall resources of the machine
std::unordered_map<std::string, int> resource_vector = 
{{"alu", 4}, {"mul", 2}, {"ldw", 1}, {"stw", 1}, {"slots", 4}};

// resources of certain commands
// All table 3 use this one
std::vector<std::string> alu_alias = {"mov", "add", "addcg", "and",
"andc", "divs", "max", "min", "maxu", "minu", "or", "orc", "sh1add",
"sh2add", "sh3add", "sh4add", "shl", "shr", "shru", "sub", "sxtb",
"sxth", "zxtb", "zxth", "xor", // these are all table1
"compeq", "cmpge", "cmpgeu", "cmpgt", "cmpgtu", "cmple", "cmpleu",
"cmplt", "cmpltu", "cmpne", "nandl", "norl", "orl", "slct", "slctf", // table 3
"mov"};  // https://edstem.org/us/courses/43502/discussion/3438852?comment=8304126
std::vector<std::unordered_map<std::string, int>> alu_resources = 
{{{"alu", 1}, {"mul", 0}, {"ldw", 0}, {"stw", 0}, {"slots", 1}}};

std::vector<std::string> mpy_alias = {"mpy", "mpyhu", "mpyhs", "mpyll", "mpyllu",
"mpylh", "mpylhu", "mpyhh", "mpyhhu", "mpyl", "mpylu", "mpyh"};
std::vector<std::unordered_map<std::string, int>> mpy_resources = 
{{{"alu", 0}, {"mul", 1}, {"ldw", 0}, {"stw", 0}, {"slots", 1}},
{{"alu", 0}, {"mul", 1}, {"ldw", 0}, {"stw", 0}, {"slots", 0}}};

std::vector<std::string> ldw_alias = {"ldw", "ldh", "ldhu", "ldb", "ldbu"};
std::vector<std::unordered_map<std::string, int>> ldw_resources = 
{{{"alu", 0}, {"mul", 0}, {"ldw", 1}, {"stw", 0}, {"slots", 1}},
{{"alu", 0}, {"mul", 0}, {"ldw", 1}, {"stw", 0}, {"slots", 0}},
{{"alu", 0}, {"mul", 0}, {"ldw", 1}, {"stw", 0}, {"slots", 0}}};

std::vector<std::string> stw_alias = {"stw", "sth", "stb"};
std::vector<std::unordered_map<std::string, int>> stw_resources = 
{{{"alu", 0}, {"mul", 0}, {"ldw", 0}, {"stw", 1}, {"slots", 1}}};

// Helpers to quickly check to see what opcode the instruction falls in
bool isALUInst(std::string instruct) {
    return std::find(alu_alias.begin(), alu_alias.end(), instruct) != alu_alias.end();
}
bool isMPYInst(std::string instruct) {
    return std::find(mpy_alias.begin(), mpy_alias.end(), instruct) != mpy_alias.end();
}
bool isLDWInst(std::string instruct) {
    return std::find(ldw_alias.begin(), ldw_alias.end(), instruct) != ldw_alias.end();
}
bool isSTWInst(std::string instruct) {
    return std::find(stw_alias.begin(), stw_alias.end(), instruct) != stw_alias.end();
}

std::vector<std::string> prologue;
std::vector<std::string> epilogue;

/*
Read instructions from an input file and return all instructions 
*/
std::vector<std::string> readInput(const char* fname)
{
    std::string str;
    std::vector<std::string> instructions;
    
    std::ifstream filestream(fname);
    std::string state = "prologue";
    int start_of_data = 2;

    while (std::getline(filestream, str))
    {
        std::size_t pos = str.find("#### BEGIN BASIC BLOCK ####");
        if (pos == 0) {
            prologue.push_back(str);
            state = "instructions";
            continue;
        }
        pos = str.find("#### END BASIC BLOCK ####");
        if (pos == 0) {
            state = "epilogue";
        }
        if (state == "prologue"){
            prologue.push_back(str);
        }
        if (state == "instructions"){
            // Filter out the ;; and comments
            if (str.substr(0, 2) != ";;") {
                size_t index = str.find('#');
                std::string stripped_str;  // stores clean string
                stripped_str = str.substr(7, index-7);  // removes things before instruct
                stripped_str.erase(stripped_str.find_last_not_of(" \t") + 1);  // removes comments and white space
                instructions.push_back(stripped_str);
            }
        }
        if (state == "epilogue"){
            epilogue.push_back(str);
        }
    }
   
   return instructions;
}

/*
Print scheduled VLIW instructions to a file.
*/
void printOutput(const char* fname, std::vector<std::string> scheduledVLIW)
{
    std::ofstream outfile;
    outfile.open(fname);
  
    for (int i = 0; i < prologue.size(); i++)
        outfile << prologue[i] << "\n";

    for (int i = 0; i < scheduledVLIW.size(); i++)
        outfile << scheduledVLIW[i] << "\n";

    for (int i = 0; i < epilogue.size(); i++)
        outfile << epilogue[i] << "\n";

    outfile.close();
}

// ****************** Helpers *************************

/** @Parse Instructions
 * @brief funct used to take clean Vex instructions and convert to iterable dictionary items of instructions
 * 
 * @param instructions vector of strings that are parsed into dict
 * @return std::vector<std::unordered_map<std::string, std::string>> 
 */
std::vector<std::unordered_map<std::string, std::string>> parseInstructions(std::vector<std::string> instructions)
{
    std::vector<std::unordered_map<std::string, std::string>> inst_dict;
    std::unordered_map<std::string, std::string> temp_dict;
    int inst_number = 0;

    for (std::string value : instructions) {
        // Take each instruction and split it on " "
        std::vector<std::string> tokens;
        std::string token;
        
        std::istringstream iss(value);

        temp_dict["inst_numb"] = std::to_string(inst_number);
        inst_number += 1;
        temp_dict["instruct"] = value;

        std::string temp_string;
        bool keep_going = false;
        while (std::getline(iss, token, ' ')) {
            // Handles strange item at end of instructions with spaces
            if (token.find('(') != std::string::npos) {
                temp_string += token;
                temp_string += " ";
                keep_going = true;
            }
            // end of that strange item
            else if (token.find(')') != std::string::npos) {
                temp_string += token;
                keep_going = false;
                tokens.push_back(temp_string);
                std::cout << temp_string << std::endl;
            }
            // add items in strange item to dict value
            else if (keep_going) {
                temp_string += token;
                temp_string += " ";

            }

            // Typically runs to this case where tokens are pushed to vector
            else {
                tokens.push_back(token);
            }
        }

        int count = 0;
        // Take the tokens in order into dictionary
        for (std::string token : tokens) {
            // op, write, = (ignored), operands

            if (count == 0){
                temp_dict["op"] = token;
            }
            else if (count == 1){
                temp_dict["write"] = token;
            }
            else if (count == 3) {
                // Only care about the register for the load, not the offset
                if (token.find('[') != std::string::npos) {
                    // filter the offset number and parse the reg
                    std::regex pattern("\\[(.*?)\\]");
                    std::smatch match;
                    std::regex_search(token, match, pattern);
                    std::string content_in_brackets = match.str(1);
                    temp_dict["operand"] = content_in_brackets;
                }
                else {
                    temp_dict["operand"] = token;
                }
            }
            // This catches when there are two operands
            else if (count == 4) {
                temp_dict["operand"] += token;
            }
            count += 1;
        }
        inst_dict.push_back(temp_dict);

        // Now, 'tokens' vector contains individual words
        // for (const std::string& word : tokens) {
        //     std::cout << word << std::endl;
        // }
    }
    
    return inst_dict;
}



std::vector<std::string> splitString(const std::string& input, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(input);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        result.push_back(token);
    }
    
    return result;
}
/** FindDependency
 * @brief Takes a vector of instrucitons split into an unordered map and finds the 
 * next dependent instruction
 * 
 * @param instructions vector of strings that are parsed into dict
 * @return std::vector<std::unordered_map<std::string, std::string>> 
 */
std::vector<std::unordered_map<std::string, std::string>> FindDependency(std::vector<std::unordered_map<std::string, std::string>> parsed_instructions)
{
    // Iterate through each of the instruction dictionaries returned by parser
    for (auto& map : parsed_instructions){
        int instruct_numb = std::stoi(map["inst_numb"]);
        std::string current_write = map["write"];
        std::string upcoming_write = "";

        // First look for the ldw/store dependency 
        if (map["op"] == "ldw" or map["op"] == "stw"){
            // Iterate through all of the instructions following this one
            for (int i = instruct_numb + 1; i < parsed_instructions.size(); ++i) {
                std::unordered_map<std::string, std::string> upcoming_instruct;
                upcoming_instruct = parsed_instructions[i]; // Instruct we're comparing against
                // Checks for that ldw/store dependency. There can only be one
                
                // handles LDW then STR (WAR) or STR then STR (WAW)
                if (upcoming_instruct["op"] == "ldw" or upcoming_instruct["op"] == "stw") {
                    map["dependency"] += upcoming_instruct["inst_numb"] + ",";
                    break;
                }
                // handles str then load (RAW)
                if (map["op"] == "stw" and upcoming_instruct["op"] == "ldw") {
                    map["dependency"] += upcoming_instruct["inst_numb"] + ",";
                    break;
                }
                // Don't want LDW dependency if reg is rewritten
                upcoming_write = upcoming_instruct["write"];
                if (current_write == upcoming_write) {
                    break;
            }
            }
        }
        
        // Then handle the RAW and WAW dependency
        char delimiter = ',';
        std::vector<std::string> upcoming_read;
        // Again, iterate through all of the instructions following the current one
        for (int i = instruct_numb + 1; i < parsed_instructions.size(); ++i) {
            std::unordered_map<std::string, std::string> upcoming_instruct;
            upcoming_instruct = parsed_instructions[i];  // instruct to compare with

            // Here is the RAW check
            upcoming_read = splitString(upcoming_instruct["operand"], delimiter);
            // check if write value exists in vector
            auto it = std::find(upcoming_read.begin(), upcoming_read.end(), current_write);
            if (it != upcoming_read.end()) {
                map["dependency"] += upcoming_instruct["inst_numb"] + ",";
            }

            // Now is the WAW check
            upcoming_write = upcoming_instruct["write"];
            if (current_write == upcoming_write) {
                size_t found = map["dependency"].find(upcoming_instruct["inst_numb"]);
                if (found == std::string::npos) {
                    map["dependency"] += upcoming_instruct["inst_numb"] + ",";
                }
                // If write is rewritten, no more RAW or WAW dependencies following this instruct
                break;
            }
            
        }

        std::vector<std::string> current_read;
        current_read = splitString(map["operand"], delimiter);
        // Then the RAW dependency
        for (int i = instruct_numb + 1; i < parsed_instructions.size(); ++i) {
            std::unordered_map<std::string, std::string> upcoming_instruct;
            upcoming_instruct = parsed_instructions[i];
            // get the current write value
            std::string upcoming_write = upcoming_instruct["write"];
            // check if write value exists in vector
            auto it = std::find(current_read.begin(), current_read.end(), upcoming_write);
            if (it != current_read.end()) {
                // Value found, remove it from the vector since that read has no more dependencies now
                current_read.erase(it);
                size_t found = map["dependency"].find(upcoming_instruct["inst_numb"]);
                if (found == std::string::npos) {
                    map["dependency"] += upcoming_instruct["inst_numb"] + ",";
                }
            }
            // If all the current map reads have found dependencies, break early
            if (current_read.size() == 0){
                break;
            }
        }
    }

    // Print all of the dictionary items
    std::cout << "HERE" << std::endl;
    for (std::unordered_map<std::string, std::string> myMap : parsed_instructions){
        std::cout << "\n";
        for (const auto& pair : myMap) {
            std::cout << "Key: " << pair.first << "\t, Value: " << pair.second << std::endl;
        }
    }
    return parsed_instructions;
}


// Define a TreeNode class to represent individual nodes in the tree
class TreeNode {
public:
    int inst_numb;
    std::string instruct;
    std::string opcode;
    int delay;
    std::vector<TreeNode*> children; // Children nodes
    std::vector<TreeNode*> nodes_before;

    // Constructor to initialize a node with metadata
    TreeNode(int numb, std::string inst, std::string opcod, int del) 
    : inst_numb(numb), instruct(inst), opcode(opcod), delay(del) {}

    // Method to add a child node to this node
    void addChild(TreeNode* child) {
        children.push_back(child);
        child->nodes_before.push_back(this);
    }

    // Function to get all subsequent nodes
    std::vector<TreeNode*> getAllSubsequentChildrenNodes() {
        std::vector<TreeNode*> subsequentNodes;
        if (!children.empty()){
            for (TreeNode* child : children) {
                if (!child->children.empty()){
                    for (TreeNode* child_child : child->children) {
                        getAllSubsequentNodesRecursive(child_child, subsequentNodes);
                    }
                }
            }
        }
        return subsequentNodes;
    }

private:
    // Helper function for recursive traversal
    void getAllSubsequentNodesRecursive(TreeNode* node, std::vector<TreeNode*>& subsequentNodes) {
        // Add the current node to the vector
        subsequentNodes.push_back(node);

        // Recursively traverse the children
        for (TreeNode* child : node->children) {
            getAllSubsequentNodesRecursive(child, subsequentNodes);
        }
    }
};



// Define a Tree class to manage the tree structure
class Tree {
public:
    TreeNode* root; // Root node of the tree
    std::vector<TreeNode*> all_nodes;

    // Constructor to create an empty tree
    Tree() : root(nullptr) {}

    // Method to add a node with metadata to the tree
    TreeNode* addNode(int numb, std::string inst, std::string opcod, int del) {
        TreeNode* newNode = new TreeNode(numb, inst, opcod, del);
        all_nodes.push_back(newNode);

        // If the tree is empty, set the new node as the root
        if (!root) {
            root = newNode;
        }


        return newNode;
    }

    // Method to create an edge between two nodes (parent and child)
    void createEdge(TreeNode* parent, TreeNode* child) {
        if (parent) {
            parent->addChild(child);
        }
    }

    // Helper function for printing tree structure
    void printBT(const std::string& prefix, TreeNode* node, bool isLeft) {
        if (node != nullptr) {
            std::cout << prefix;
            std::cout << (isLeft ? "├──" : "└──");
            std::cout << node->inst_numb << std::endl;

            // Iterate over children and print their edges horizontally
            for (size_t i = 0; i < node->children.size(); ++i) {
                bool isLast = (i == node->children.size() - 1);
                printBT(prefix + (isLeft ? "│   " : "    "), node->children[i], !isLast);
            }
        }
    }

    void printBT(TreeNode* node) {
        printBT("", node, false);
    }

    void printTree() {
        printBT(root);
    }

    std::vector<TreeNode*> findOtherRoots() {
        std::vector<TreeNode*> rootNodes;

        std::vector<TreeNode*> nodes_from_main_root = root->getAllSubsequentChildrenNodes();
        nodes_from_main_root.push_back(root);

        // Iterate through all nodes in the forest and find nodes that are not dependencies
        for (const TreeNode* node : all_nodes) {
            if (std::find(nodes_from_main_root.begin(), nodes_from_main_root.end(), node) == nodes_from_main_root.end()
                & node->nodes_before.empty()) {
                // filter for nodes not in main path and have no dependencies
                rootNodes.push_back(const_cast<TreeNode*>(node));
            }
        }

        return rootNodes;
    }

    void printForest() {
        std::vector<TreeNode*> rootNodes = findOtherRoots();
        rootNodes.push_back(root);
        for (size_t i = 0; i < rootNodes.size(); ++i) {
            bool isLastRoot = (i == rootNodes.size() - 1);
            printBT("", rootNodes[i], isLastRoot);
        }
    }

    void trimTransitiveEdges(TreeNode* nodePtr) {
        if (nodePtr == NULL) {
            return;
        }

        TreeNode node = *nodePtr;
        std::vector<TreeNode*> subsequentNodes = node.getAllSubsequentChildrenNodes();
        std::vector<TreeNode*>& node_children = nodePtr->children;
        // std::cout << "Current Node is " << nodePtr->inst_numb << std::endl;
        // for (TreeNode* nod : subsequentNodes) {
        //     std::cout << nod->inst_numb << " ";
        // }
        // std::cout << std::endl;

        // for (size_t i = 0; i < node_children.size(); ++i) {
        //     std::cout<< "here" << std::endl;
        //     // There is a duplicate node in the subsequent nodes
        //     auto it = std::find(subsequentNodes.begin(), subsequentNodes.end(), node_children[i]);
        //     if (it != node_children.end()) {
        //         // Element found, erase it from the vector
        //         for (TreeNode* nod : node_children) {
        //             std::cout << nod->inst_numb << " ";
        //         }
        //         std::cout << std::endl;
        //         node_children.erase(it);
        //         // std::cout << "Trimming " << subsequentNodes[i]->inst_numb << " From " << nodePtr->inst_numb << std::endl;
        //     }
        // }
        node_children.erase(std::remove_if(node_children.begin(), node_children.end(),
            [&subsequentNodes](TreeNode* element) {
                // Use a lambda function to check if the element is in subsequentNodes
                return std::find(subsequentNodes.begin(), subsequentNodes.end(), element) != subsequentNodes.end();
            }),
            node_children.end());

        for (TreeNode* node : node_children) {
            trimTransitiveEdges(node);
        }
    }

    void cleanTransEdgesFromTree() {
        trimTransitiveEdges(root);
    }

    int getPathDepth(std::vector<TreeNode*> path) {
        int depth = 0;
        for (TreeNode* node : path) {
            int delay = node->delay;
            depth += delay;
            // std::cout << std::to_string(node->delay) << std::endl;
        }
        return depth;
    }

    std::vector<TreeNode*> findMaxDepth(TreeNode* node, std::vector<TreeNode*> visited){
        // Test this with actual weights later
        if (node == NULL) {
            return visited;
        }
        // std::cout << std::to_string(node->inst_numb) << std::endl;
        std::vector<TreeNode*> current_path = visited;
        current_path.push_back(node);
        int depth = getPathDepth(current_path);
        int curr_depth = depth;
            
        int max_depth = 0;
        std::vector<TreeNode*> max_path = current_path;

        if (! node->children.empty()){
            for (TreeNode* child : node->children) {
                std::vector<TreeNode*> potential_path = findMaxDepth(child, current_path);
                int potential_depth = getPathDepth(potential_path);
                if (max_depth < potential_depth){
                    max_depth = potential_depth;
                    max_path = potential_path;
                }
            }
        }
        // for (TreeNode* vist : max_path) {
        //     std::cout << vist-> inst_numb << " ";
        // }
        // std::cout << "Other" << std::endl;
        
        return max_path;
    }

    void printDeepestPath(){
        std::vector<TreeNode*> path;
        path = findMaxDepth(root, path);
        int delay = 0;
        for (TreeNode* node : path) {
            delay += node->delay;
            std::cout << std::to_string(node->inst_numb) << "/" << delay << " " << std::endl;
        }
    }

    std::vector<TreeNode*> findAllNodeDependencies(TreeNode* node) {
        std::vector<TreeNode*> dependency_nodes;
        std::vector<TreeNode*> unsearched_nodes;

        unsearched_nodes.push_back(root);
        while (!unsearched_nodes.empty()) {
            for (TreeNode* search_node: unsearched_nodes) {
                std::vector<TreeNode*> childs = search_node->children;
                auto it = std::find(unsearched_nodes.begin(), unsearched_nodes.end(), search_node);

                unsearched_nodes.erase(it);
                for (TreeNode* child : childs) {
                    if (child == node) {
                        dependency_nodes.push_back(child);
                    }
                    else {
                        unsearched_nodes.push_back(child);
                    }
                }
            }
        }
        return dependency_nodes;
    }
};



Tree bulidTree(std::vector<std::unordered_map<std::string, std::string>>& instruction) {
// Tree bulidTree(const auto& instruction) {
    std::vector<TreeNode*> instruction_tree;

    Tree tree;

    for (const auto& unorderedMap : instruction) {
        // the variables to unpack from the dict
        int inst_numb;
        std::string inst;
        int delay;
        std::string op; 
        for (const auto& pair : unorderedMap) {
            
            const std::string& key = pair.first;
            const std::string& value = pair.second;
            if (key == "inst_numb") {
                inst_numb = std::stoi(value);
            }
            if (key == "instruct") {
                inst = value;
            }
            if (key == "op") {
                op = value;
            }
        }
        if (isALUInst(op) | isSTWInst(op)) {
            delay = 1;
        } else if (isMPYInst(op)) {
            delay = 2;
        } else if (isLDWInst(op)) {
            delay = 3;
        }else {
            std::cout << "WARNING, Unsuported opcode for vliwScheduler. Setting delay for " <<
            op << " to be delay = 1" << std::endl;
            delay = 1;
        }
        TreeNode* node = tree.addNode(inst_numb, inst, op, delay);
        
        instruction_tree.push_back(node);
        
    }
       
    int count = 0;
    for (std::unordered_map<std::string, std::string> instruct : instruction) {
        std::string str_edges = instruct["dependency"];
        std::istringstream iss(str_edges);
        std::string token; 
        std::vector<int> edges;

        // split those edges up into a vector
        while (std::getline(iss, token, ',')) {
            // Convert each token to an integer and add it to the vector
            int number;
            std::istringstream(token) >> number;
            edges.push_back(number);
        }

        // Now that we have the nodes made and edges ready, add edges in tree
        for (int edge : edges) {
            tree.createEdge(instruction_tree[count], instruction_tree[edge]);
        }
        count += 1;
    }

    tree.cleanTransEdgesFromTree();

    tree.printForest();
    return tree;
}

// Custom comparison function to sort TreeNode* pointers based on inst_numb
bool compareByInstNumb(const TreeNode* a, const TreeNode* b) {
    return a->inst_numb < b->inst_numb;
}


bool areAllElementsInVector2(std::vector<TreeNode*>& vector1, std::vector<TreeNode*>& vector2) {
    for (TreeNode* element : vector1) {
        if (std::find(vector2.begin(), vector2.end(), element) == vector2.end()) {
            // Element from vector1 not found in vector2
            return false;
        }
    }
    return true;
}
// WARNIGN, AFTER TESTING, CHANGE BACK TO instruct and not inst_Numb
// WARNING, doesn't check above dependencies
std::vector<TreeNode*> scheduleOnSource(Tree tree) {
    std::vector<int> schedule;
    TreeNode* root = tree.root;
    // schedule.push_back(root->instruct); 

    std::vector<TreeNode*> unsearched_nodes;  // nodes to search next iteration
    std::vector<TreeNode*> scheduled_nodes;  // nodes that already have been scheduled
    std::vector<TreeNode*> reschedule_search;  // nodes that still need dependency


    // initialize the search with the root node
    unsearched_nodes.push_back(root);
    for (TreeNode* root_node : tree.findOtherRoots()) {
        unsearched_nodes.push_back(root_node);
    }
    while (!unsearched_nodes.empty()) {
        // This heuristic schedules based on order
        std::sort(unsearched_nodes.begin(), unsearched_nodes.end(), compareByInstNumb);

        // std::this_thread::sleep_for(std::chrono::seconds(2));

        // add current nodes it schedule if they're ready
        for (TreeNode* node : unsearched_nodes) {
            std::vector<TreeNode*> nodes_bef = node->nodes_before;
            if (std::find(scheduled_nodes.begin(), scheduled_nodes.end(), node) != scheduled_nodes.end()) {
                // don't reschedule a node if it has already been scheduled!
                continue;
            }
            else if (nodes_bef.empty()) {  // root node has no dependency
                schedule.push_back(node->inst_numb);
                scheduled_nodes.push_back(node);
            }
            else if (areAllElementsInVector2(nodes_bef, scheduled_nodes)) {
                schedule.push_back(node->inst_numb);
                scheduled_nodes.push_back(node);
            }
            else {
                reschedule_search.push_back(node);
            }
        }
        
        for (TreeNode* search_node: unsearched_nodes) {
            // take the children of the unsearched node and add the to the list
            std::vector<TreeNode*> childs = search_node->children;
            // The node has now officially been searched and can be dropped.
            for (TreeNode* child : childs) {
                reschedule_search.push_back(child);
            }
        }

        // clear followed by add is to protect memory
        unsearched_nodes.clear();
        // reschedule the nodes for the next pass
        // std::cout<< "here" <<std::endl;
        for (TreeNode* i : reschedule_search) {
            unsearched_nodes.push_back(i);
            // std::cout << i->inst_numb << std::endl;
        }

        for (TreeNode* i : scheduled_nodes){
            std::cout << i ->inst_numb <<std::endl;
        }
        std::cout << "\n\n";

        reschedule_search.clear();
    }

    std::cout << schedule.empty() << std::endl;
    for (int inst_numb : schedule) {
        std::cout << inst_numb << std::endl;
    }

    return scheduled_nodes;
}

std::vector<std::string> listSchedAlg(std::vector<TreeNode*> topo_inst) {
    std::vector<std::string> schedule;



    return schedule;
}


int findIndexInVectorOfVectors(const std::vector<std::vector<TreeNode*>>& vecOfVecs, TreeNode* value) {
    for (size_t i = 0; i < vecOfVecs.size(); ++i) {
        const std::vector<TreeNode*>& innerVector = vecOfVecs[i];
        for (size_t j = 0; j < innerVector.size(); ++j) {
            if (innerVector[j] == value) {
                return i; // Return the index of the outer vector
            }
        }
    }
    return -1; // Return -1 if the value is not found
}

/*
Inputs:
    - std::vector<std::string> instructions. The input is a vector of strings. Each
      string in the vector is an instruction in the original vex code.
    - <int> mode: value indicating which heuristic ordering to use
Returns : std::vector<std::string>

The function should return a vector of strings. Each string should be a scheduled instruction or ;; that marks the end of a VLIW instruction word.
*/
std::vector<std::string>  scheduleVLIW(std::vector<std::string> instructions,
                                       int mode)
{
    std::vector<std::string> scheduledVLIW;

    std::vector<std::unordered_map<std::string, std::string>> parsed;
//    parsed = parseInstructions(instructions);  // project1 actual input
    parsed = parseInstructions(hw1_instructions);  // hw1 refrence input

    std::vector<std::unordered_map<std::string, std::string>> dependencies;
    dependencies = FindDependency(parsed);

    Tree tree = bulidTree(dependencies);

    tree.printDeepestPath();

    std::vector<TreeNode*> initial_schedule;
    switch (mode) {
        case 0: 
            // Source heruistic: first in program is first in tie
            initial_schedule = scheduleOnSource(tree);
            break;
        
   }

   // Here is where we place the scheduling algorithm
   std::vector<std::vector<TreeNode*>> algoirthm_sched;
   for (TreeNode* node : initial_schedule) {
        // line2 of algorithm to find delay from last predecessor
        std::vector<TreeNode*> predecessors = node->nodes_before;
        int index; 
        int s_index = 0;
        for (TreeNode* pred : predecessors) {
            // Find the index of the predecessor and add the delay
            index = findIndexInVectorOfVectors(algoirthm_sched, pred);
            index += pred->delay;
            // filter for the max predecessor delay
            if (index > s_index) {
                s_index = index;
            }

        }

        // line 3 of algorithm
        bool keep_searching_rt = true;
        std::vector<std::unordered_map<std::string, int>> global_rt;
        // create a variable to test what it'd look like if inst added at s_index
        std::vector<std::unordered_map<std::string, int>> potential_rt;
        while (keep_searching_rt) {
            potential_rt = global_rt;

            std::vector<std::unordered_map<std::string, int>> inst_resouces;
            if (isALUInst(node->opcode)) {
                inst_resouces = alu_resources;
            } else if (isMPYInst(node->opcode)) {
                inst_resouces = mpy_resources;
            } else if (isLDWInst(node->opcode)) {
                inst_resouces = ldw_resources;
            } else if (isSTWInst(node->opcode)) {
                inst_resouces = stw_resources;
            }

            for (std::unordered_map<std::string, int> resource : inst_resouces) {
                // TODO add each part of resource map to potential map
                // and see if the potential resource table items remain within
                // bounds of resouce_vector
            }
        }

   }

    return scheduledVLIW;
}

int main(int argc, char *argv[])
{

   if(argc != 4) {
       std::cout << "Invalid parameters \n";
       std::cout << "Expected use ./vliwScheduler ";
       std::cout << "<input file name> <output file name> <mode>\n";
   }
 
   const char* inputFile = argv[1];
   const char* vliwSchedulerOutput = argv[2];
   int mode = atoi(argv[3]);

   std::vector<std::string> instructions;
   std::vector<std::string> scheduledVLIW;
 
   /* Read instructions from the file */
   instructions = readInput(inputFile);
   int count = 0;
   for (const std::string& value : instructions) {
    std::cout << std::to_string(count) << value << "\n";
    count += 1;
   }

   /* Schedule instructions */
   scheduledVLIW = scheduleVLIW(instructions, mode);

   /* Print scheduled instructions to file */
   printOutput(vliwSchedulerOutput, scheduledVLIW);
}

