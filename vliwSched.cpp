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

/* BEGIN VALIDATION AGAINST HW1*/
std::vector<std::string> hw1_instructions = {
    // "ignore $r6 ignore ignore ignore",  // this instruct offsets the others so they start at index=1
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


// hw1 matching resources for validation purposes. Leave commmented out on deliverable
// std::unordered_map<std::string, int> system_resource_vector = 
// {{"alu", 1}, {"mul", 1}, {"ldw", 2}, {"stw", 1}, {"slots", 4}};

// std::vector<std::unordered_map<std::string, int>> alu_resources = 
// {{{"alu", 1}, {"mul", 0}, {"ldw", 0}, {"stw", 0}, {"slots", 1}},
// {{"alu", 1}, {"mul", 0}, {"ldw", 0}, {"stw", 0}, {"slots", 0}}};

// std::vector<std::unordered_map<std::string, int>> mpy_resources = 
// {{{"alu", 0}, {"mul", 1}, {"ldw", 0}, {"stw", 0}, {"slots", 1}},
// {{"alu", 0}, {"mul", 1}, {"ldw", 0}, {"stw", 0}, {"slots", 0}},
// {{"alu", 0}, {"mul", 1}, {"ldw", 0}, {"stw", 0}, {"slots", 0}},
// {{"alu", 0}, {"mul", 1}, {"ldw", 0}, {"stw", 0}, {"slots", 0}}};

// std::vector<std::unordered_map<std::string, int>> ldw_resources = 
// {{{"alu", 0}, {"mul", 0}, {"ldw", 1}, {"stw", 0}, {"slots", 1}},
// {{"alu", 0}, {"mul", 0}, {"ldw", 1}, {"stw", 0}, {"slots", 0}},
// {{"alu", 0}, {"mul", 0}, {"ldw", 1}, {"stw", 0}, {"slots", 0}},
// {{"alu", 0}, {"mul", 0}, {"ldw", 1}, {"stw", 0}, {"slots", 0}},
// {{"alu", 0}, {"mul", 0}, {"ldw", 1}, {"stw", 0}, {"slots", 0}}};

/* COMPLETE VALIDATION AGAINST HW1*/

// overall resources available to the machine. 
std::unordered_map<std::string, int> system_resource_vector = 
{{"alu", 4}, {"mul", 2}, {"ldw", 1}, {"stw", 1}, {"slots", 4}};

// alu resource commands and corresponding latency vector
std::vector<std::string> alu_alias = {"mov", "add", "addcg", "and",
"andc", "divs", "max", "min", "maxu", "minu", "or", "orc", "sh1add",
"sh2add", "sh3add", "sh4add", "shl", "shr", "shru", "sub", "sxtb",
"sxth", "zxtb", "zxth", "xor", // these are all table1
"compeq", "cmpge", "cmpgeu", "cmpgt", "cmpgtu", "cmple", "cmpleu",
"cmplt", "cmpltu", "cmpne", "nandl", "norl", "orl", "slct", "slctf", // table 3
"mov"};  // https://edstem.org/us/courses/43502/discussion/3438852?comment=8304126
std::vector<std::unordered_map<std::string, int>> alu_resources = 
{{{"alu", 1}, {"mul", 0}, {"ldw", 0}, {"stw", 0}, {"slots", 1}}};

// multiply resources for the machine and their vex commands
std::vector<std::string> mpy_alias = {"mpy", "mpyhu", "mpyhs", "mpyll", "mpyllu",
"mpylh", "mpylhu", "mpyhh", "mpyhhu", "mpyl", "mpylu", "mpyh"};
std::vector<std::unordered_map<std::string, int>> mpy_resources = 
{{{"alu", 0}, {"mul", 1}, {"ldw", 0}, {"stw", 0}, {"slots", 1}},
{{"alu", 0}, {"mul", 1}, {"ldw", 0}, {"stw", 0}, {"slots", 0}}};

// Here are the loads and the three latency resources corresponding to it
std::vector<std::string> ldw_alias = {"ldw", "ldh", "ldhu", "ldb", "ldbu"};
std::vector<std::unordered_map<std::string, int>> ldw_resources = 
{{{"alu", 0}, {"mul", 0}, {"ldw", 1}, {"stw", 0}, {"slots", 1}},
{{"alu", 0}, {"mul", 0}, {"ldw", 1}, {"stw", 0}, {"slots", 0}},
{{"alu", 0}, {"mul", 0}, {"ldw", 1}, {"stw", 0}, {"slots", 0}}};

// finally the stores
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
 * @return the same map with a new key/value set for dependencies 
 */
std::vector<std::unordered_map<std::string, std::string>> FindDependency(std::vector<std::unordered_map<std::string, std::string>> parsed_instructions)
{
    // Iterate through each of the instruction dictionaries returned by parser
    for (auto& map : parsed_instructions){
        // unpack some of the dict
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

        // Then the RAW dependency
        std::vector<std::string> current_read;
        current_read = splitString(map["operand"], delimiter);
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
    // for (std::unordered_map<std::string, std::string> myMap : parsed_instructions){
    //     std::cout << "\n";
    //     for (const auto& pair : myMap) {
    //         std::cout << "Key: " << pair.first << "\t, Value: " << pair.second << std::endl;
    //     }
    // }
    return parsed_instructions;
}


// Define a TreeNode class to represent individual instructions in the tree
class TreeNode {
public:
    int inst_numb;  // aka instruction 1
    std::string instruct;  //  string val of instruction
    std::string opcode;  // specifically the commmand (ex. ldw)
    int delay;  // corresponding delay for that opcode
    std::vector<TreeNode*> children; // Children nodes
    std::vector<TreeNode*> nodes_before;  // preceeding nodes
    int height;  // largest distance to bottom of treek (aka all delays summed)

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

// Define a Tree class to manage all of the nodes
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

    // looking at hw1, we're liable to have a forrest with several roots. This funct find other roots with no
    // edges pointing to them
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

    // prints the tree for each root seperately
    void printForest() {
        std::vector<TreeNode*> rootNodes = findOtherRoots();
        rootNodes.push_back(root);
        std::cout << "Here is your dependency forest!" << std::endl;
        for (size_t i = 0; i < rootNodes.size(); ++i) {
            bool isLastRoot = (i == rootNodes.size() - 1);
            printBT("", rootNodes[i], isLastRoot);
        }
    }

    // removes childs from node->children when applicable
    void trimTransitiveEdges(TreeNode* nodePtr) {
        if (nodePtr == NULL) {
            return;
        }

        TreeNode node = *nodePtr;
        std::vector<TreeNode*> subsequentNodes = node.getAllSubsequentChildrenNodes();
        std::vector<TreeNode*>& node_children = nodePtr->children;

        // if a child node also points to one of the other children, it is transitive and should be removed.
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

    // Helper to find how deep (aka high) a node path is
    int getPathDepth(std::vector<TreeNode*> path) {
        int depth = 0;
        for (TreeNode* node : path) {
            int delay = node->delay;
            depth += delay;
        }
        return depth;
    }

    // Starting from a certain node (param) this function returns the deepest path from that node
    std::vector<TreeNode*> findMaxDepth(TreeNode* node, std::vector<TreeNode*> visited){
        // Test this with actual weights later
        if (node == NULL) {
            return visited;
        }
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
        
        return max_path;
    }

    // iteratively sets the hight value of all the nodes in the tree using findMaxDepth()
    void findAllNodesHeight() {
        for (TreeNode* node : all_nodes) {
            std::vector<TreeNode*> path;
            path = findMaxDepth(node, path);
            int delay = 0;
            for (TreeNode* nod : path) {
                delay += nod->delay;
            }
            node->height = delay;
        }
    }

    // Checks the deepest path from each of the roots and returns the deepest overall path
    void printDeepestPath(){
        std::vector<TreeNode*> rootNodes = findOtherRoots();  
        rootNodes.push_back(root);  // we now have vector of all the roots
        std::vector<TreeNode*> path;
        int delay = 0;
        // iterate over these roots and find the deepest path amongst them
        for (TreeNode* root_n : rootNodes) {
            std::vector<TreeNode*> temp_path = findMaxDepth(root_n, {});
            int temp_delay = 0;
            for (TreeNode* node : temp_path) {
                temp_delay += node->delay;
            }
            if (temp_delay > delay) {
                delay = temp_delay;
                path = temp_path;
            }
        }
        // print out the deepest path to the terminal
        for (TreeNode* node : path) {
            std::cout << std::to_string(node->inst_numb) << ", ";
        }
        std::cout << "\nDeepest Path Length is " << delay << std::endl;
    }
};

// takes a vector of unordered maps representing nodes, makes the nodes, and adds the edges
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
            // unpack the dictionary values for this node
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
        // find the delay depending on the opcode
        if (isALUInst(op) | isSTWInst(op)) {
            // delay = 2;  // hw1 validation
            delay = 1;
        } else if (isMPYInst(op)) {
            // delay = 4;  // hw1 validation
            delay = 2;
        } else if (isLDWInst(op)) {
            // delay = 5;  // hw1 validation
            delay = 3;
        }else {
            std::cout << "WARNING, Unsuported opcode for vliwScheduler. Setting delay for " <<
            op << " to be delay = 1" << std::endl;
            delay = 1;
        }
        // make the node!
        TreeNode* node = tree.addNode(inst_numb, inst, op, delay);

        instruction_tree.push_back(node);  // technically this is the same as tree.all_nodes
    }
       
    // we've made the nodes, but now need to add the edges (seen below)
    int count = 0;
    for (std::unordered_map<std::string, std::string> instruct : instruction) {  // reiterate over the maps
        // get the dependencies (ex. 1, 2,), split them on ',' and iterate over the values
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
            // note that each "dependency" is the instruction value and the index in the vector
            tree.createEdge(instruction_tree[count], instruction_tree[edge]);
        }
        count += 1;
    }
    // finally prune the tree a bit
    tree.cleanTransEdgesFromTree();

    tree.printForest();
    return tree;
}

// Custom comparison function to sort TreeNode* pointers based on inst_numb
bool compareByInstNumb(const TreeNode* a, const TreeNode* b) {
    return a->inst_numb < b->inst_numb;  // lowest to highest
}

// to be used in std::sort
bool compareByFanOut(const TreeNode* a, const TreeNode* b) {
    return a->children.size() > b->children.size();  // count of children from largest to smallest
}

// same but comparison by size
bool compareByDepth(const TreeNode* a, const TreeNode* b) {
    return a->height > b->height;  // largest to smallest hight
}

// next two functions are helpers for vector comparison
bool areAllElementsInVector2(std::vector<TreeNode*>& vector1, std::vector<TreeNode*>& vector2) {
    for (TreeNode* element : vector1) {
        if (std::find(vector2.begin(), vector2.end(), element) == vector2.end()) {
            // Element from vector1 not found in vector2
            return false;
        }
    }
    return true;
}

bool anyElementsInVector2(std::vector<TreeNode*>& vector1, std::vector<TreeNode*>& vector2) {
    for (TreeNode* element : vector1) {
        if (std::find(vector2.begin(), vector2.end(), element) != vector2.end()) {
            // Element from vector1 not found in vector2
            return true;
        }
    }
    return false;
}

// helper to re-order the unsearched nodes for the flatten tree funct
void moveVectTwoToFrontVectOne(std::vector<TreeNode*>& unsearched_nodes, const std::vector<TreeNode*>& longest_path) {
    // Create a temporary vector to store the reordered elements
    std::vector<TreeNode*> reordered_nodes;

    // Iterate through longest_path and add matching elements to reordered_nodes
    for (const TreeNode* node : longest_path) {
        auto it = std::find(unsearched_nodes.begin(), unsearched_nodes.end(), node);
        if (it != unsearched_nodes.end()) {
            reordered_nodes.push_back(*it);
            unsearched_nodes.erase(it);
        }
    }

    // Append the remaining unsearched_nodes to reordered_nodes
    reordered_nodes.insert(reordered_nodes.end(), unsearched_nodes.begin(), unsearched_nodes.end());

    // Assign the reordered_nodes back to unsearched_nodes
    unsearched_nodes = reordered_nodes;
}

// identifies the most critical resource as defined by numb_uses / numb_of_that_resource
std::vector<TreeNode*> findCriticality(std::vector<TreeNode*> all_nodes) {
    std::unordered_map<std::string, int> resources_used_count = {{"alu", 0}, {"ldw", 0}, {"stw", 0}, {"mul", 0}};
    
    // tally up the number of instructions using each resource
    for (TreeNode* node : all_nodes) {
        std::string op = node->opcode;
        if (isALUInst(op)) {
            resources_used_count["alu"] += 1;
        } else if (isMPYInst(op)) {
            resources_used_count["mul"] += 1;
        } else if (isLDWInst(op)) {
            resources_used_count["ldw"] += 1;
        } else if (isSTWInst(op)) {
            resources_used_count["stw"] += 1;
        }else {
            std::cout << "WARNING, Unsuported opcode for vliwScheduler. Assuming it is a ALU for criticality calc." << std::endl;
            resources_used_count["alu"] += 1;
        }
    }

    // calculate the criticality and save the highest criticality to two values below
    std::string critical_op;
    float current_criticality = 0;
    for (const auto& pair : resources_used_count) {
        std::string key = pair.first;
        int value = pair.second;
        int item = system_resource_vector[key];
        float criticality_ratio = value / system_resource_vector[key];
        if (criticality_ratio > current_criticality) {
            current_criticality = criticality_ratio;
            critical_op = key;
        }
    }

    // now flag each of the nodes using critical resource in a vector
    std::vector<TreeNode*> nodes_with_crit_resource;
    for (TreeNode* node : all_nodes) {
        if (node->opcode == critical_op) {
            nodes_with_crit_resource.push_back(node);
        }
    }

    // This vector is used later to resort the flattening of the tree
    return nodes_with_crit_resource;
}

/* flattens the dependency tree into a loopable instance. Input for list sched alg */
std::vector<TreeNode*> flattenTree(Tree tree, bool use_deepest_path = false, 
                                        std::vector<TreeNode*> nodes_using_crit_resource = {}, 
                                        bool use_fanout = false) {
    /* BRIEF: Takes every node in the tree and addes it to a vector to search. WHILE the
    vector is not empty, it begins by resorting the unsearched vector based on the heuristic.
    Then it iterates over each of the unsorted nodes and addes it to a queue (aka vector) that
    is used by the list scheduling algorithm. This only adds a node to the schedule if all of
    it's preceeding nodes have already been added. Otherwise, it adds it back to the search and repeats */
    std::vector<std::string> schedule;
    TreeNode* root = tree.root;

    // helps control flow of what heuristic to use. Flags if ony a single heuristic is being used
    bool using_single_heur = ((!nodes_using_crit_resource.empty()) + use_deepest_path + use_fanout) == 1;

    std::vector<TreeNode*> unsearched_nodes;  // nodes to search next iteration
    std::vector<TreeNode*> scheduled_nodes;  // nodes that already have been scheduled
    std::vector<TreeNode*> reschedule_search;  // nodes that still need dependency
    std::vector<TreeNode*> scheduled_this_iteration;  // a list of nodes just scheduled

    std::vector<TreeNode*> deepest_path;
    deepest_path = tree.findMaxDepth(tree.root, deepest_path);

    // initialize the search with the root node
    // unsearched_nodes.push_back(root);
    // for (TreeNode* root_node : tree.findOtherRoots()) {
    //     unsearched_nodes.push_back(root_node);
    // }
    // above comments are for old method I used. Now I initialize all nodes to be flattened immediately
    unsearched_nodes = tree.all_nodes;
    while (!unsearched_nodes.empty()) {
        // This heuristic schedules based on order
        std::sort(unsearched_nodes.begin(), unsearched_nodes.end(), compareByInstNumb);

        // std::cout << "BEFORE" << std::endl;
        // for (TreeNode* nod : unsearched_nodes) {
        //     std::cout << nod->inst_numb << std::endl;
        // }
        // apply the deepest path heuristic if it is a param
        if (use_deepest_path) {
            if (using_single_heur) {
                // moveVectTwoToFrontVectOne(unsearched_nodes, deepest_path);
                std::sort(unsearched_nodes.begin(), unsearched_nodes.end(), compareByDepth);
            } else if (use_fanout) {
                std::sort(unsearched_nodes.begin(), unsearched_nodes.end(), compareByFanOut);
                std::sort(unsearched_nodes.begin(), unsearched_nodes.end(), compareByDepth);
                // moveVectTwoToFrontVectOne(unsearched_nodes, deepest_path);
            } else { // This is the critical path, resource, then source
                // first you bring the crit resources forward
                moveVectTwoToFrontVectOne(unsearched_nodes, nodes_using_crit_resource);
                // then you bring the longest path forward in front of the resources
                std::sort(unsearched_nodes.begin(), unsearched_nodes.end(), compareByDepth);
                // moveVectTwoToFrontVectOne(unsearched_nodes, deepest_path);
                // the deepest path and resources should already be in source order
            }
            
        }
        else if (!nodes_using_crit_resource.empty()) {
            std::sort(unsearched_nodes.begin(), unsearched_nodes.end(), compareByDepth);
        }
        else if (use_fanout) {
            std::sort(unsearched_nodes.begin(), unsearched_nodes.end(), compareByFanOut);
        }
        // std::cout << "DEBUG" << std::endl;
        // for (TreeNode* nod : unsearched_nodes) {
        //     std::cout << nod->inst_numb << std::endl;
        // }

        scheduled_this_iteration.clear();
        // add current nodes it schedule if they're ready
        for (TreeNode* node : unsearched_nodes) {
            std::vector<TreeNode*> nodes_bef = node->nodes_before;
            if (std::find(scheduled_nodes.begin(), scheduled_nodes.end(), node) != scheduled_nodes.end()) {
                // don't reschedule a node if it has already been scheduled!
                continue;
            }
            else if (nodes_bef.empty()) {  // root node has no dependency
                schedule.push_back(node->instruct);
                scheduled_nodes.push_back(node);
            }
            else if (areAllElementsInVector2(nodes_bef, scheduled_nodes) & !anyElementsInVector2(nodes_bef, scheduled_this_iteration)) {  
                // first check is if all nodes before have already been scheduled
                // second check is to make sure nodes before didn't just get scheduled (helps with heuristic)              
                schedule.push_back(node->instruct);
                scheduled_nodes.push_back(node);
                scheduled_this_iteration.push_back(node);
            }
            else {
                reschedule_search.push_back(node);
            }
        }
        
        // OLD METHOD AND CAN BE IGNORED
        // for (TreeNode* search_node: unsearched_nodes) {
        //     // take the children of the unsearched node and add the to the list
        //     std::vector<TreeNode*> childs = search_node->children;
        //     // The node has now officially been searched and can be dropped.
        //     for (TreeNode* child : childs) {
        //         reschedule_search.push_back(child);
        //     }
        // }

        // clear followed by add is to protect memory
        unsearched_nodes.clear();
        // reschedule the nodes for the next pass
        for (TreeNode* i : reschedule_search) {
            unsearched_nodes.push_back(i);
        }

        reschedule_search.clear();
    }

    return scheduled_nodes;
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

/* Finds the last cycle the instructions end at*/
int findLengthOfSchedule(std::vector<std::unordered_map<std::string, int>> global_rt) {
    int sched_count = 0;
    int count = 0;
    for (std::unordered_map<std::string, int> map : global_rt) {
        bool all_zero = true;
        for (const auto& pair : map) {
            const std::string& key = pair.first;
            int value = pair.second;
            
            // Check if the key exists in the target map
            if (value != 0) {
                sched_count = count;
                all_zero = false;
            } 
        }
        // if all resources used are zero, this must be where instructions end
        if (all_zero) {
            break;
        }
        count += 1;
    }

    return sched_count;
}

// Takes the schedule list of list of nodes and converts to printOutput param
std::vector<std::string> formatNodeSchedToString(std::vector<std::vector<TreeNode*>> schedule, int length_of_schedule) {
    std::vector<std::string> format_schedule; 
    
    for (int i = 0; i < length_of_schedule + 1; i++) {
        std::vector<TreeNode*> instruction_set = schedule[i];
        if (!instruction_set.empty()) {
            for (TreeNode* node : instruction_set) {
                std::string header = "\tc0\t";
                header += node->instruct;
                format_schedule.push_back(header);
            }
        }
        format_schedule.push_back(";;");

    }

    return format_schedule;
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
    parsed = parseInstructions(instructions);  // project1 actual input
    // parsed = parseInstructions(hw1_instructions);  // hw1 instructions used for validation

    std::vector<std::unordered_map<std::string, std::string>> dependencies;
    dependencies = FindDependency(parsed);

    Tree tree = bulidTree(dependencies);

    tree.findAllNodesHeight();  // set the height values of all nodes for heuristic


    std::cout << "The deepest path is: " << std::endl;
    tree.printDeepestPath();

    std::vector<TreeNode*> initial_schedule;
    std::vector<TreeNode*> is_critical_node = findCriticality(tree.all_nodes);
    std::cout << "You have chosen the ";
    switch (mode) {
        case 0: 
            // Source heruistic: first in program is first in tie
            std::cout << "Source Heuristic" << std::endl;
            initial_schedule = flattenTree(tree, false, {}, false);
            break;
        case 1:
            // Critical Path - Source
            std::cout << "Critical path - Source" << std::endl;
            initial_schedule = flattenTree(tree, true, {}, false);  // use deepest path is set true here
            break;
        case 2:
            // resource heuristic
            std::cout << "Resource - Source Heuristic" << std::endl;
            initial_schedule = flattenTree(tree, false, is_critical_node, false); 
            break;
        case 3:
            std::cout << "Fanout - Source Heuristic" << std::endl;
            initial_schedule = flattenTree(tree, false, {}, true); 
            break;
        case 4:
            std::cout << "Critical path - Resource - Source Heuristic" << std::endl;
            initial_schedule = flattenTree(tree, true, is_critical_node, false); 
            break;
        case 5:
            std::cout << "Critical path - Fanout - Source Heuristic" << std::endl;
            initial_schedule = flattenTree(tree, true, {}, true); 
            break;
   }
   std::cout << "FLAT" << std::endl;
   for (TreeNode* node : initial_schedule) {
    std::cout << node->inst_numb << std::endl;
   }

    int empty_slots_initialize = 9000;  // TOD CHANGE THIS GUY

   // Here is where we place the scheduling algorithm
   std::vector<std::vector<TreeNode*>> algoirthm_sched;
   algoirthm_sched.resize(empty_slots_initialize);
    std::vector<std::unordered_map<std::string, int>> global_rt;
    std::unordered_map<std::string, int> empty_resources = 
    {{"alu", 0}, {"mul", 0}, {"ldw", 0}, {"stw", 0}, {"slots", 0}};

    for (int i = 0; i < empty_slots_initialize; i++) {
        global_rt.push_back(empty_resources);
    }
   for (TreeNode* node : initial_schedule) {
        // line2 of algorithm to find delay from last predecessor
        std::vector<TreeNode*> predecessors = node->nodes_before;
        int index; 
        int s_index = 0;
        if (!predecessors.empty()) {
            for (TreeNode* pred : predecessors) {
                // Find the index of the predecessor and add the delay
                index = findIndexInVectorOfVectors(algoirthm_sched, pred);
                if (index != -1) {
                    index += pred->delay;
                }
                // filter for the max predecessor delay
                if (index > s_index) {
                    s_index = index;
                }
            }
        }

        // line 3 of algorithm
        bool keep_searching_rt = true;

        // create a variable to test what it'd look like if inst added at s_index
        std::vector<std::unordered_map<std::string, int>> potential_rt;
        while (keep_searching_rt) {
            potential_rt = global_rt;

            std::vector<std::unordered_map<std::string, int>> inst_resouces;

            std::string opcode_to_check = node->opcode;

            if (isMPYInst(opcode_to_check)) {
                inst_resouces = mpy_resources;
            } else if (isLDWInst(opcode_to_check)) {
                inst_resouces = ldw_resources;
            } else if (isSTWInst(opcode_to_check)) {
                inst_resouces = stw_resources;
            } else if (isALUInst(opcode_to_check)) {
                inst_resouces = alu_resources;
            } else {
                std::cout << "Warning: Did not find op-code match for inst. Defaulting to alu resources used"<< std::endl;
                inst_resouces = alu_resources;
            }
            
            bool too_many_resources_used = false;
            // This is the current resources used for this step in sched
            int counter = 0;  // this is offset from s_index

            // iterate over each resource requirement of the opcode
            for (int i = 0 ; i < inst_resouces.size() ; i++) {
                std::unordered_map<std::string, int> resource = inst_resouces[0];


                // TOTOTODO!!! This guy is being indexed badly
                std::unordered_map<std::string, int> current_resource_in_sched = potential_rt[s_index + counter];

                // unpack the current resources used and add that to the potential resource table
                for (const auto& pair : resource) {
                    const std::string& key = pair.first;
                    int value = pair.second;
                    
                    // Check if the key exists in the target map
                    if (current_resource_in_sched.find(key) != current_resource_in_sched.end()) {
                        // Key exists, add the values
                        current_resource_in_sched[key] += value;
                    } else {
                        // Key doesn't exist, create it
                        current_resource_in_sched[key] = value;
                    }
                }
                // now check if this current resource exceed system limits
                for (const auto& pair : resource) {
                    const std::string& key = pair.first;
                    int value = pair.second;
                    if (current_resource_in_sched[key] > system_resource_vector[key]) {
                        too_many_resources_used = true;
                    }
                }
                if (too_many_resources_used) {
                    break;
                } else {
                    counter += 1;
                }
            }
            // Need to keep looking in resource table for
            if (too_many_resources_used) {
                s_index += 1;
            } else {  // too_many_resc used flag low meaning we found a good spot in the schedule
                keep_searching_rt = false;

                // add the instruction to the official schedule
                algoirthm_sched[s_index].push_back(node);

                // Now go through and add this instructs resources to the global_rt
                int instruct_delay = inst_resouces.size();  // find the length of the vector, AKA delay
                int iteration = 0;
                for (size_t i = s_index; i < s_index + instruct_delay && i < global_rt.size(); ++i) {
                    
                    // std::unordered_map<std::string, int> resources_used = global_rt[i];
                // for (auto& map : global_rt[s_index, s_index + instruct_delay]) {
                    // Iterate over the key-value pairs in each map and modify the values
                    for (const auto& kvp : inst_resouces[iteration]) {  //[i - s_index]) {
                        global_rt[i][kvp.first] += kvp.second;
                    }
                    iteration += 1;
                }
                // }
            }
            
        }

   }

   int cycles_of_schedule = findLengthOfSchedule(global_rt); 
   std::cout << "Total Cycles: " << cycles_of_schedule + 1 << std::endl;

   scheduledVLIW = formatNodeSchedToString(algoirthm_sched, cycles_of_schedule);

   for (std::string i : scheduledVLIW) {
    std::cout << i << std::endl;
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
   int mode = 0; // default value
   mode = atoi(argv[3]);

   std::vector<std::string> instructions;
   std::vector<std::string> scheduledVLIW;
 
   /* Read instructions from the file */
   instructions = readInput(inputFile);
   int count = 0;

   /* Schedule instructions */
   scheduledVLIW = scheduleVLIW(instructions, mode);

   /* Print scheduled instructions to file */
   printOutput(vliwSchedulerOutput, scheduledVLIW);
}

