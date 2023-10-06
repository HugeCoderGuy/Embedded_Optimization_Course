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
Gets the syllable from the file line
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

    // iterate through all instructions
    for (std::string instruction : instructions) {
        // create the encoded instruction every ;; in original file
        std::string firstTwoChars = instruction.substr(0, 2);
        if (firstTwoChars == ";;") {
            // first add instruction delim
            std::string encoded_inst = "c0\t";
            encoded_inst += mask;  // add that mask before syllables
            encoded_inst += "\t";
            // append each of the syllables found
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

        // processing instructions to make mask
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

    // First check how wide the VLIW is by iterating until end of first instruct
    int vliw_width = 0;
    for (std::string instruction : instructions) {
        std::string firstTwoChars = instruction.substr(0, 2);
        if (firstTwoChars == ";;") {
            break;
        }
        else {
            vliw_width += 1;
        }
    }  // vliw width determines the number of bits needed for mask

    // Now begin going through instructions and building templates
    std::vector<std::string> syllables; //list of syllables for next instruction
    std::string mask = "";  // mask for each instruction
    int instruction_index = 0;
    int last_instruction_index = 0;

    for (std::string instruction : instructions) {
        // Check if it is ";;" for end of inst
        std::string firstTwoChars = instruction.substr(0, 2);
        if (firstTwoChars == ";;") {
            // Handle the all NOP case for the mask
            if (mask == "") {
                if (vliw_width == 4) {
                    mask = "00";
                }
                else if (vliw_width == 8) {
                    mask = "000";
                }
                else {
                    std::cout << "Unexpected VLIW width " << vliw_width;
                }
            }

            // Begin making the encoded instruction
            std::string encoded_inst = "c0\t";
            encoded_inst += mask;
            encoded_inst += "\t";
            // add the syllables
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
            instruction_index = 0;
            last_instruction_index = 0;
        }
        // This is where we handle building the inst from the syllables
        else {
            std::string inst =  parseIndividualInst(instruction);

            if (inst != "NOP") {
                syllables.push_back(inst);
                int diff_in_index = instruction_index - last_instruction_index;
                last_instruction_index = instruction_index;

                // create the mask depending on the width
                if (vliw_width == 4) {
                    switch (diff_in_index) {
                        case 0:
                            mask += "00";
                            break;
                        case 1:
                            mask += "01";
                            break;
                        case 2:
                            mask += "10";
                            break;
                        case 3:
                            mask += "11";
                            break;
                    }
                }
                else if (vliw_width == 8) {
                    switch (diff_in_index) {
                        case 0:
                            mask += "000";
                            break;
                        case 1:
                            mask += "001";
                            break;
                        case 2:
                            mask += "010";
                            break;
                        case 3:
                            mask += "011";
                            break;
                        case 4:
                            mask += "100";
                            break;
                        case 5:
                            mask += "101";
                            break;
                        case 6:
                            mask += "110";
                            break;
                        case 7:
                            mask += "111";
                            break;
                    }
                }
                else {
                    std::cout << "Unexpected VLIW width " << vliw_width;
                }
            }

            instruction_index += 1;
        }

    }
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
