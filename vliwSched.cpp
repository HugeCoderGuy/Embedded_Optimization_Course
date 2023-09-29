#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <regex>

std::vector<std::string> hw1_instructions = {
    "ignore ignore ignore ignore ignore",
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
                if (upcoming_instruct["op"] == "ldw" or upcoming_instruct["op"] == "stw") {
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
}


// Define a TreeNode class to represent individual nodes in the tree
class TreeNode {
public:
    int inst_numb;
    std::string instruct;
    int delay;
    std::vector<TreeNode*> children; // Children nodes

    // Constructor to initialize a node with metadata
    TreeNode(int numb, std::string inst, int del) 
    : inst_numb(numb), instruct(inst), delay(del) {}

    // Method to add a child node to this node
    void addChild(TreeNode* child) {
        children.push_back(child);
    }
};


// Define a Tree class to manage the tree structure
class Tree {
public:
    TreeNode* root; // Root node of the tree

    // Constructor to create an empty tree
    Tree() : root(nullptr) {}

    // Method to add a node with metadata to the tree
    TreeNode* addNode(int numb, std::string inst, int del) {
        TreeNode* newNode = new TreeNode(numb, inst, del);

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

    // Method to print the entire tree structure
    void printTree(TreeNode* node, int depth = 0) {
        if (!node) return;

        // Print metadata and indentation based on depth
        for (int i = 0; i < depth; i++) {
            std::cout << "  ";
        }
        std::cout << "|--" << node->inst_numb << std::endl;

        // Recursively print children nodes
        for (TreeNode* child : node->children) {
            printTree(child, depth + 1);
        }
    }

    // Wrapper method to print the entire tree
    void printTree() {
        printTree(root);
    }
};

Tree bulidTree(std::vector<std::unordered_map<std::string, std::string>> instruction) {
    std::vector<TreeNode*> instruction_tree;
    Tree tree;
    for (std::unordered_map<std::string, std::string> instruct : instruction) {
        instruction_tree.push_back(tree.addNode(std::stoi(instruct["inst_numb"]), instruct["instruct"], 0));
    }
       std::cout << "HREHRE" << std::endl;
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
    tree.printTree();
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

    /* TODO : Implement your code here */

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
   std::vector<std::unordered_map<std::string, std::string>> parsed;
//    parsed = parseInstructions(instructions);
    parsed = parseInstructions(hw1_instructions);

   std::vector<std::unordered_map<std::string, std::string>> dependencies;
   dependencies = FindDependency(parsed);

   bulidTree(dependencies);

   /* Schedule instructions */
   scheduledVLIW = scheduleVLIW(instructions, mode);

   /* Print scheduled instructions to file */
   printOutput(vliwSchedulerOutput, scheduledVLIW);
}
