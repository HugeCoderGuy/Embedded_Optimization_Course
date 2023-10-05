#include <iostream>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>

#define MAX_INSTR_WIDTH 100

/*
Read instructions from an input file and return a char array 
*/
std::vector<std::string> readInput(const char* fname)
{
    std::string str;
    std::vector<std::string> instructions;
    
    std::ifstream filestream(fname);
    
    while (std::getline(filestream, str))
    {
        int current_position = 0;
        std::size_t pos = str.find("\tc0    ");
        if (pos == 0)
        {
            instructions.push_back(str);
        } else {
            std::size_t f = str.find(";;");
            if (f == 0)
                instructions.push_back(str);
        }
    }
   
   return instructions;
}

/*
Print an encoded VLIW instruction to a file. The encoded instruction should be a character array and terminated by '\0' character
*/
void printOutput(const char* fname, std::vector<std::string> encodedVLIW)
{
    std::ofstream outfile;
    outfile.open(fname);
  
    for(int i = 0; i < encodedVLIW.size(); i++)
        outfile << encodedVLIW[i] << "\n";

    outfile.close();
}

/*
TODO : Write any helper functions that you may need here. 

*/

std::string parseIndividualInst(std::string line) {
    size_t endPos = line.find("##");
    size_t startPos = 6;

    std::string first_strip = line.substr(startPos + 1, endPos - startPos - 1);
    // Find the position of the first non-whitespace character from the beginning
    size_t start = 0;

    // Find the position of the last non-whitespace character from the end
    size_t end = first_strip.find_last_not_of(" \t\n\r");

    std::string trimmedString = first_strip.substr(start, end - start + 1);
    
    std::cout << trimmedString << "b" << std::endl;

    return trimmedString;
}

/*
Input : std::vector<std::string> instructions. The input is a vector of strings. Each string in the vector is one line in the vex code. A line can be a instruction or a ';;'

Returns : std::vector<std::string>

The function should return a vector of strings. Each string should be a line of VLIW encoded instruction with masked encoding scheme
*/
std::vector<std::string>  maskedVLIW(std::vector<std::string> instructions)
{
    std::vector<std::string> encodedVLIW;
    std::vector<std::string> syllables; //list of syllables for next instruction
    std::string mask = "";  // mask for each instruction

    /* TODO : Implement your code here */
    for (std::string instruction : instructions) {
        // Check if it is ";;" for end of inst
        std::string firstTwoChars = instruction.substr(0, 2);
        if (firstTwoChars == ";;") {
            std::string encoded_inst = "c0\t";
            encoded_inst += mask;
            encoded_inst += "\t";
            for (std::string non_nop_inst : syllables) {
                encoded_inst += non_nop_inst;
                encoded_inst += "\t";
            }
            encoded_inst.pop_back();
            encoded_inst += "\n;;";
            encodedVLIW.push_back(encoded_inst);
            // reset the mask and syllables for next inst
            syllables.clear();
            mask = "";
        }

        else {
            std::string inst =  parseIndividualInst(instruction);

            if (inst != "NOP") {
                syllables.push_back(inst);
                mask += "1";
            }
            else {
                mask += "0";
            }
        }

    }

    for (std::string i : encodedVLIW) {
        std::cout << i << std::endl;
    }

    return encodedVLIW;
}

/*
Input : std::vector<std::string> instructions. The input is a vector of strings. Each string in the vector is one line in the vex code. A line can be a instruction or a ';;'

Returns : std::vector<std::string>

The function should return a vector of strings. Each string should be a line of VLIW encoded instruction with template encoding scheme
*/
std::vector<std::string>  templateVLIW(std::vector<std::string> instructions)
{
    std::vector<std::string> encodedVLIW;

    /* TODO : Implement your code here */

    return encodedVLIW;
}


int main(int argc, char *argv[])
{

   if(argc != 2) {
       std::cout << "Invalid parameters \n";
       std::cout << "Expected use ./vliwEncoder <input file name>\n";
   }
 
   const char* inputFile = argv[1];
   const char* maskedOutput = "maskedEncoding.txt";
   const char* templateOutput = "templateEncoding.txt";

   std::vector<std::string> instructions;
   std::vector<std::string> maskedEncoding;
   std::vector<std::string> templateEncoding;
 
   /* Read instructions from the file */
   instructions = readInput(inputFile);

   /* Encode instructions using masked and template encoding */
   maskedEncoding = maskedVLIW(instructions);
   templateEncoding = templateVLIW(instructions);

   /* Print encoded instructions to file */
   printOutput(maskedOutput,maskedEncoding);
   printOutput(templateOutput,templateEncoding);
}
