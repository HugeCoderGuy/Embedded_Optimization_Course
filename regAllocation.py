# Homework 3 Submission Format
#
# Do not modify any variable names or datastructures, only the resulting values.
# Do not modify the imports
# Leave the filename as hw3.py so the format checker will work.
#
# When you have filled this file out, please run hw3_format_check.py in the same folder as hw3.py
# If there are any formatting issues, the script will provide an output explaining such.
# If there are no issues, the script will only output 'Done'. This means your formatting appears correct.
#``
# Examples of the type of data are provided, please update with your solution.
import json, itertools
from decorators import *

class Allocator:
        def __init__(self, ir = None):
                if ir is not None:
                    self.ir = ir
                else:
                    # NOTE: Below is the default array of the instructions
                    self.ir = ["Main:",  
                                "assign, a, 1.0,",
                                "assign, b, 1.0,",
                                "assign, c, 1.0,",
                                "assign, d, 0,",
                                "assign, e, 0,",
                                "brgt, b, d, label0,",
                                "assign, d, b,",
                                "brneg, b, d, label1,",
                                "label2:",
                                "add, c, a, d,",
                                "assign, a, c,",
                                "assign, d, b,",
                                "brgeq, d, b, label2,",
                                "label3:", 
                                "return, , ,",
                                "label0:",
                                "assign, a, e,",
                                "goto, label2, ,",
                                "label1:", 
                                "assign, b, 2.0,",
                                "add, d, b, 4.0,",
                                "assign, b, a,",
                                "goto, label3, ,"]
        def apply_functions(self):
                # Generate all data
                self.detect_basic_blocks()
                self.control_flow_graph()
                self.find_definitions()
                self.find_uses()
                self.find_live_out()
                self.find_live_in()
                self.find_webs()
                self.interference_graph()
                self.chaitins_graph_coloring()

        # Question 1

        # a) Detect Basic Blocks
        #
        # Determine how many basic blocks there are
        # and which line of code belongs to each.
        # DO NOT MODIFY the first element of each array.
        ##########################################


        # Begin helper functions
        def edge_string(self, item1_points_to, item2):
                return "{} -> {}".format(item1_points_to, item2)
        # End helper functions

        # a)
        @check_detect_basic_blocks
        def detect_basic_blocks(self):
                '''
                Result: Array of Arrays of instructions and their basic block number.
                        Example:
                                [["Main:", "None"],
                                ["assign, a, 1.0,", "1"],
                                ["assign, b, 1.0,", "1"],
                                ["assign, c, 1.0,", "2"],
                                ["assign, d, 0,", "2"],
                                ["brgt, b, d, label0,", "3"],
                                ["assign, d, b,", "3"],
                                ["brneg, b, d, label1,", "4"],
                                ...
                                ]
                Note:
                Datatype: integers, Range 1,10
                Note: To be clear, the exact number of basic blocks
                        is for you to determine and will not require
                        all the numbers 1,2,3,4,5,6,7,8,9,10
                
                Leave Labels as None
                Fill in numbers linearly (top to bottom).
                
                This is a wrong example. Note the 1 comes after the 2. This will result in a deduction. Even if the lines are in the same basic block correctly.
                solution   = [["Main:", "None"],
                                ["assign, a, 1.0,", "2"],
                                ["assign, b, 1.0,", "2"],
                                ["assign, c, 1.0,", "1"],
                                ["assign, d, 0,", "1"],
                                ["brgt, b, d, label0,", "3"],
                                ["assign, d, b,", "3"],
                                ["brneg, b, d, label1,", "4"],

                '''
                solution = [[]]
                # Begin Algorithm
                solution = []  # redefining solution as empty list rather than embedded list
                current_block = 1  # counter for basic block value
                last_line = ""
                for line in self.ir:
                        # ":" defines a 
                        if ":" in line:
                                solution.append([line, "None"])
                                if "label" == line[0:5] and not ("br" in last_line or "goto" in last_line):
                                        current_block += 1
                        elif "br" in line or "goto" in line:
                                solution.append([line, str(current_block)])
                                current_block += 1
                        else:
                                solution.append([line, str(current_block)])

                        last_line = line

                # for i in solution:
                #         print(i)

                # End Algorithm
                # Do not modify below, you can reference self.bb later in your program if need be.
                self.bb = solution

        # Question 1
        # b) Create Control Flow Graph links
        # Datatype: Array of source -> destination strings
        # Use Basic Block Numbers from 1a
        # For example.
        # This is similar to HW1, where we had source/destination
        # q1b = ["1 -> 2",
        #         "2 -> 3",
        #         "3 -> 4",
        #         "4 -> 5"]
        @check_control_flow_graph
        def control_flow_graph(self):
                '''
                Result: Array of strings.
                        Example:
                                ["1 -> 2",
                                "2 -> 3",
                                "3 -> 4",
                                "4 -> 5"]
                '''
                solution = []
                # Begin Algorithm
                
                # first identify what blocks correspond to what labels
                labels_to_blocks = {}
                for i, line_and_block in enumerate(self.bb):
                        if line_and_block[0][0:5] == "label":
                                next_line_and_block = self.bb[i+1]
                                labels_to_blocks[line_and_block[0]] = next_line_and_block[1]
                                
                for i, line_and_block in enumerate(self.bb):
                        current_inst = line_and_block[0]  # current instruction
                        split_curr_inst = current_inst.split(", ")  # ASUMPTION: labeln resides at index [-1] always
                        current_block = line_and_block[1]  # current basic block
                        if current_block == None:
                                continue
                        
                        # branches have fall through. Fallthrough is handled here
                        elif "br" in current_inst:
                                next_inst = self.bb[i+1][0]
                                next_block = self.bb[i+1][1]
                                # first handle edge case that last instruction is branch
                                if len(self.bb) == i + 1:  # technically this won't happen, but just in case
                                        label = split_curr_inst[-1]
                                        if label == ",":  # this if catches to the goto ending with ", ,"
                                                label = split_curr_inst[-2]
                                        if label[-1] == ",":  # this cleans up the label to query the dict for the corresponding block
                                                label = label[0:-1] + ":"  # now has the collen to match the dict val
                                        else:
                                                label += ":"
                                        next_bb = labels_to_blocks[label]
                                        solution.append(self.edge_string(current_block, next_bb))
                                else:
                                        # handle the fall through case for branch
                                        if next_block == "None":  # when next line of code is a "labeln:"
                                                next_bb = labels_to_blocks[next_inst]  # ASUMPTION: labeln lines only consist of labeln:
                                                solution.append(self.edge_string(current_block, next_bb))
                                        else:  # when next line of code is just more operations
                                                solution.append(self.edge_string(current_block, next_block))
                        
                        # "gotos" and branches point to "labeln:" block. This is handled below
                        if "br" in current_inst or "goto" in current_inst:
                                # clean up the "labeln:" so we can query it against labels_to_block to find corresponding bb
                                label = split_curr_inst[-1]
                                if label == ",":  # this if catches to the goto ending with ", ,"
                                        label = split_curr_inst[-2]
                                if label[-1] == ",":  # this cleans up the label to query the dict for the corresponding block
                                        label = label[0:-1] + ":"  # now has the collen to match the dict val
                                else:
                                        label += ":"
                                next_bb = labels_to_blocks[label]
                                solution.append(self.edge_string(current_block, next_bb))
                        
                # for i in self.bb:
                #         print(i)
                # print(solution)
                # for i in self.bb:
                #         print(i)
                for i in solution:
                        print(i)

                # End Algorithm
                # you may use the self.cfg variable later if need be 
                self.cfg = solution


# Question 1 - Parts C
# Fill out in report

        # Question 1 - Parts D & E
        # Liveness Analysis
        # Create a program to perform the liveness analysis on the CFG and show the live ranges found. The Excel spreadsheet has been provided as a way to visualize your liveness analysis and check your program output.
        #
        # Please fill out your algorithm between "Begin algorithm" and "End algorithm" in each template function.

        '''
        OPTIONAL: create any helper functions here that you might use in your algorithms.
        Hint: You may use outputs from detect_basic_blocks and q1b (basic block mappings and CFG) to help with your
        liveness analysis.
        '''
        # Begin helper functions
        def is_number(self, s):
                """Checks if a string is a float or int. Used with "use" var function

                Args:
                    s (string): string to check

                Returns:
                    bool: true if is number, false if not
                """
                try:
                        float(s)
                        return True
                except ValueError:
                        return False
                
        def find_next_bb_indexs(self, current_basic_block):
                """Takes the block you're in and returns blocks that point to it from cfg
                
                ex. if 1 -> 2, and 2 is the input, function returns 1

                Args:
                    current_basic_block (str): string of basic block num

                Returns:
                    list: list of basic blocks before
                """
                blocks_before = []
                for edge in self.cfg:
                        if current_basic_block == edge[0]:
                                blocks_before.append(edge[-1])
                                
                indexes_to_return = [self.find_first_line_of_bb(i) for i in blocks_before]
                return indexes_to_return
        
        def find_first_line_of_bb(self, basic_block):
                """Finds the line number in the IR corresponding to the edge of a bb

                Args:
                    basic_block (str): string of basic block number

                Returns:
                    int: line index of last line of bb
                """
                last_bb = None
                for i, line in enumerate(self.bb):
                        if basic_block == line[1]:
                                return i
                        
        def find_last_line_of_bb(self, basic_block):
                """Finds the last line number in the IR corresponding to the edge of a bb

                Args:
                    basic_block (str): string of basic block number

                Returns:
                    int: line index of last line of bb
                """
                for i, line in enumerate(self.bb):
                        if basic_block == line[1]:
                                last_bb_line = i
                return last_bb_line
                                
        # End helper functions

        # d1) Determine the variable definitions at each line of code.
        @check_find_definitions
        def find_definitions(self):
                '''
                Result: Array of instructions corresponding to their variable definitions.
                        Example:
                        [["0", ""],
                        ["1", "x"],
                        ["2", "y"],
                        ["3", "z"],
                        ["4", "w"],
                        ["5", "z"],
                        ["6", ""],
                        ["7", ""]]
                '''
                solution = [[]]
                # Begin Algorithm
                solution = []
                for i, line in enumerate(self.ir):
                        if "add" in line or "assign" in line:
                                split_line = line.split(", ")
                                solution.append([str(i), split_line[1]])
                        else:
                                solution.append([str(i), ""])
                                
                # for i in solution:
                #         print(i)

                # End Algorithm
                self.defs = solution

        # d2) Determine the variable uses at each line of code.
        @check_find_uses
        def find_uses(self):
                '''
                Result: Array of instructions corresponding to an array of their variable
                        definitions.
                        Example:
                        [["0", []],
                        ["1", []],
                        ["2", ["x"]],
                        ["3", ["x","y"]],
                        ["4", ["y","z"]],
                        ["5", ["y","w"]],
                        ["6", ["y","z"]],
                        ["7", []]
                        ]
                '''
                solution = [[]]
                # Begin Algorithm
                solution = []
                for i, line in enumerate(self.ir):
                        use_list = []
                        split_line = line.split(", ")
                        split_line[-1] = split_line[-1][0:-1]
                        filtered_list = [x for x in split_line if x != ""]
                        if "add" in filtered_list or "assign" in filtered_list:
                                filtered_list.pop(0)  # remove the opcode
                                filtered_list.pop(0)  # remove the def
                                use_list = [j for j in filtered_list if not self.is_number(j)]
                        elif filtered_list[0] == "Main:" or filtered_list[0][0:5] == "label" or filtered_list[0] == "goto":
                                pass
                        else:
                                filtered_list.pop(0)  # remove the opcode
                                use_list = [j for j in filtered_list if j[0:5] != "label" and not self.is_number(j)]
                        solution.append([str(i), use_list])
                        
                for i in solution:
                        print(i)
                        

                # End Algorithm
                self.uses = solution

        # d3) Determine the variables that are Live-Out at each line of code.
        @check_find_live_out
        def find_live_out(self):
                '''
                Result: Array of instructions corresponding to their Live[Out] variables.
                        Example:
                        [["0", []],
                        ["1", ["x"]],
                        ["2", ["x"]],
                        ["3", ["x","y"]],
                        ["4", ["y","z"]],
                        ["5", ["y","w"]],
                        ["6", ["y","z"]],
                        ["7", []]
                        ]
                '''
                solution = [[]]
                # Begin Algorithm
                solution = []
                num_instructions = len(self.ir)
                # initialize a live in and live out set matching each index of instruct
                live_out = [set() for _ in range(num_instructions)]  
                live_in = [set() for _ in range(num_instructions)]

                changed = True
                while changed:  # iterate until liveness has settled
                        changed = False
                        
                        # work backwards from the last instruction in the series 
                        for i in range(num_instructions-1, 0, -1):
                                in_before = live_in[i].copy()  # this is the check for changed variable later
                                
                                # unpack the instruciton we're dealing with
                                basic_block = self.bb[i][1]
                                defs = set(self.defs[i][1])
                                uses = set(self.uses[i][1])
                                
                                if basic_block == "None":  # ignore the labels
                                        continue
                                # if True, this means that the liveout is union of following basic blocks
                                if i == self.find_last_line_of_bb(basic_block):  # aka is this instruction the end of a bb?
                                        next_bbs_indexes = self.find_next_bb_indexs(basic_block)
                                        for index in next_bbs_indexes:  # add those live-ins from the last basic blocks
                                                live_out[i] |= live_in[index]
                                else:
                                        # live out of this instruct is the live in of the next instruct
                                        live_out[i] |= live_in[i+1]
                                                
                                # update the in set for this instruction
                                in_set = live_out[i].copy()
                                in_set = in_set.difference(defs) | uses

                                if in_set != in_before:  # did we see change?
                                        changed = True
                                        live_in[i] = in_set
                                        # Set the instruction before this one to have it's out match this in
                                        if (self.ir[i-1][0:5] != "label"):
                                                live_out[i-1] |= in_set
                                                
                # validation
                # for i, st in enumerate(live_in):
                #         print(self.ir[i], "\t\t\t", st, "\t\t\t", live_out[i])
                                
                # format the deliverable
                for i in range(num_instructions):
                        solution.append([str(i), list(live_out[i])])  # live out answer

                # End Algorithm
                self.live_out = solution

        # d4) Determine the variables that are Live-In at each line of code.
        @check_find_live_in
        def find_live_in(self):
                '''
                Result: Array of instruction numbers corresponding to their Live[In] variables.
                        Example:
                        [["0", []],
                        ["1", []],
                        ["2", ["x"]],
                        ["3", ["x","y"]],
                        ["4", ["y","z"]],
                        ["5", ["y","w"]],
                        ["6", ["y","z"]],
                        ["7", []]
                        ]
                '''
                solution = [[]]
                # Begin Algorithm
                """THIS IS THE SAME EXACT CODE AS THE PROBLEM ABOVE (D3)"""
                solution = []
                num_instructions = len(self.ir)
                # initialize a live in and live out set matching each index of instruct
                live_out = [set() for _ in range(num_instructions)]  
                live_in = [set() for _ in range(num_instructions)]

                changed = True
                while changed:  # iterate until liveness has settled
                        changed = False
                        
                        # work backwards from the last instruction in the series 
                        for i in range(num_instructions-1, 0, -1):
                                in_before = live_in[i].copy()  # this is the check for changed variable later
                                
                                # unpack the instruciton we're dealing with
                                basic_block = self.bb[i][1]
                                defs = set(self.defs[i][1])
                                uses = set(self.uses[i][1])
                                
                                if basic_block == "None":  # ignore the labels
                                        continue
                                # if True, this means that the liveout is union of following basic blocks
                                if i == self.find_last_line_of_bb(basic_block):  # aka is this instruction the end of a bb?
                                        next_bbs_indexes = self.find_next_bb_indexs(basic_block)
                                        for index in next_bbs_indexes:  # add those live-ins from the last basic blocks
                                                live_out[i] |= live_in[index]
                                else:
                                        # live out of this instruct is the live in of the next instruct
                                        live_out[i] |= live_in[i+1]
                                                
                                # update the in set for this instruction
                                in_set = live_out[i].copy()
                                in_set = in_set.difference(defs) | uses

                                if in_set != in_before:  # did we see change?
                                        changed = True
                                        live_in[i] = in_set
                                        # Set the instruction before this one to have it's out match this in
                                        if (self.ir[i-1][0:5] != "label"):
                                                live_out[i-1] |= in_set
                                                
                # validation
                # for i, st in enumerate(live_in):
                #         print(self.ir[i], "\t\t\t", st, "\t\t\t", live_out[i])
                                
                # format the deliverable
                for i in range(num_instructions):
                        """THIS IS THE ONLY LINE THAT IS CHANGED FROM ABOVE"""
                        solution.append([str(i), list(live_in[i])])  # live out answer

                # End Algorithm
                self.live_in = solution

        # Helper for web
        def find_all_use_points(self, variable, defined=False):
                if defined:
                        filter_for = self.defs
                else:
                        filter_for = self.uses
                used_at = []
                for i, used in enumerate(filter_for):
                        if variable in used[1]:
                                bb = self.bb[i][1]
                                used_at.append((i, bb))
                return used_at
        
        def merge_sets_with_common_element(self, sets_list):
                result = []
                
                while sets_list:
                        current_set = sets_list.pop(0)
                        combined = {element for element in current_set}
                        
                        i = 0
                        while i < len(sets_list):
                                if any(element in combined for element in sets_list[i]):
                                        combined.update(sets_list.pop(i))
                                        i = 0  # Restart the check from the beginning
                                else:
                                        i += 1

                        result.append(combined)

                return result
        
        def find_continuous_streaks(self, int_str_list):
                if not int_str_list:
                        return []

                int_str_list.sort()
                start, end = int(int_str_list[0]), int(int_str_list[0])
                streaks = []

                for i in range(1, len(int_str_list)):
                        current = int(int_str_list[i])
                        if current - end == 1:
                                end = current
                        else:
                                streaks.append((start, end))
                                start = end = current

                streaks.append((start, end))
                return streaks

        # e) Show all webs for each variable
        @check_find_webs
        def find_webs(self):
                '''
                Result: Dictionary of variables corresponding to all their webs. Each web is
                        represented by an array of instruction numbers. Some variables may have more
                        than 1 web.
                        Example:
                        {
                        "x1" : ["1",
                                "2",
                                "3"],
                        "y1" : ["7",
                                "9",
                                "10",
                                "11",
                                "15"],
                        "z1" : ["1",
                                "3"],
                        "z2" : ["2",
                                "6"],
                        "w1" : ["5",
                                "10"],
                        }

                NOTE: DO NOT include labels in your answer.
                '''
                solution = {}
                # Begin algorithm
                
                # lets first find all of the variables
                variables = set()
                for liveins in self.live_in:
                        variables.update(liveins[1])
                variables = list(variables)
                variables.sort()
                
                # first find the "overall" web the variable is live for. AKA all live points
                initial_solution = {}
                for i in range(len(self.ir)):
                        var_added = []
                        # live out varaible is alive if it is in the live_out
                        for variable in self.live_out[i][1]:
                                if variable not in initial_solution:
                                        initial_solution[variable] = []
                                initial_solution[variable].append(i)
                                var_added.append(variable)
                        # now handle the last line that the variable is killed at
                        for variable in self.live_in[i][1]:
                                # aka is the variable only in the live on (if statement below)
                                if variable not in self.live_out[i] and variable not in var_added:
                                        initial_solution[variable].append(i)
                
                # now try to split the initial web up if there isn't continuity
                for var in variables:
                        live_points = initial_solution[var] # all the live points of this var
                        var_live_in = set()  # a set of the blocks the variable is live in
                        for point in live_points:
                                var_live_in.update(self.bb[point][1])
                        
                        # initialize the web_blocks (a set with bb of that var) with the first bb instance
                        int_set = {int(s) for s in var_live_in}
                        first_bb = min(int_set)
                        web_blocks = [set(str(first_bb))]
                        # iteratively add the other connected blocks to this web block set
                        for edge in self.cfg:
                                # unpack the edge
                                from_block = edge[0]
                                last_line_of_from_block = self.find_last_line_of_bb(from_block)
                                to_block = edge[-1]
                                first_line_of_to_block = self.find_first_line_of_bb(to_block)
                                # only deal with edges in cfg with live vars in both blocks
                                if from_block in var_live_in and to_block in var_live_in:
                                        # then check if the varaiable is passed between the two blocks
                                        if var in self.live_out[last_line_of_from_block][1] and var in self.live_in[first_line_of_to_block][1]:
                                                # if it is, add it to the edge block sets
                                                current_blocks = set([to_block, from_block])
                                                web_blocks.append(current_blocks)
                        
                        # now we have a series of sets of edges between bb with the var flowing between
                        merged_web_blocks = self.merge_sets_with_common_element(web_blocks)
                        # ^ combine all the blocks in the edge bb sets that are connected. Unecessary for this homework, but redundant
                        
                        # There is an off chance that a variable has an isolated web in the middle of a bb
                        isolated_webs = []
                        # check the start and end points of a continous live range
                        for start_index, end_index in self.find_continuous_streaks(live_points):
                                start_bb = self.bb[start_index][1]
                                end_bb = self.bb[end_index][1]
                                # starts of blocks are acounted for
                                if start_index != self.find_first_line_of_bb(start_bb):
                                        # so are ends of blocks
                                        if end_index != self.find_last_line_of_bb(end_bb):
                                                # however isolated live ranges in middle of blocks is not 
                                                if start_bb == end_bb:
                                                        # account for it here
                                                        isolated_webs.append(range(start_index, end_index))
                        
                        # now we have a list of basic block sets with the live var. Iterate over each indipended web
                        for count, block in enumerate(merged_web_blocks):
                                for point in live_points:
                                        current_bb = self.bb[point][1]
                                        # check if this point is in the web being processed
                                        if current_bb in block:
                                                # if it is, add it to the solution
                                                var_string = var + str(count+1)
                                                if var_string not in solution:
                                                        solution[var_string] = []
                                                solution[var + str(count+1)].append(str(point))
                                        # If the block is not apart of any identified webs, add it to it's own web
                                        if all([current_bb not in b for b in merged_web_blocks]):
                                                merged_web_blocks.append([current_bb])
                
                        # now handle those islolated webs in the middle of other webs
                        # note that this is overly redundant since it doesn't happen in our homework
                        for key in solution.keys():
                                # find the webs for this variable
                                if key[0] == var:
                                        for isolated_web in isolated_webs:
                                                # if the isolated web is inside of a different solution
                                                if all([int(point) in isolated_web for point in solution[key]]):
                                                        # remove that isolated web from that solution entry
                                                        solution[key] = [item for item in solution[key] if int(item) not in isolated_web]
                                                        
                                                        # then go through and find the next solution dictionary web
                                                        largest_web_numb = 0
                                                        for k in solution.keys():
                                                                if k[0] == var and int(k[1]) > largest_web_numb:
                                                                        largest_web_numb = int(k[1])
                                                        var_string = var + str(largest_web_numb)
                                                        # finally add that isolated web as it's on web
                                                        solution[var_string] = [str(i) for i in isolated_web]
                
                # End algorithm
                self.webs = solution

        # Question 1
        # fa) Create Interference Graph
        # Datatype: Array of source -- destination strings
        # Use web labels from find_webs function
        # For example.
        # This is similar to HW1, where we had source/destination however since it's undirected it's just two dashes instead of an arrow
        @check_interference_graph
        def interference_graph(self):
                '''
                Result: List of strings of the following example format.
                        ["a1 -- b",
                        "c -- d",
                        "e -- f",
                        "g -- h"]
                NOTE: DO NOT include labels in your answer.
                '''
                solution = []
                # Begin algorithm
                
                # check all of the values for each web
                for key, values in self.webs.items():
                        for value in values:
                                for key2, values2 in self.webs.items():
                                        # to see if they match values in other webs (and aren't same web)
                                        if value in values2 and key != key2:
                                                # format the string in both permutations
                                                interference_str = "{} -- {}".format(key, key2)
                                                interference_str_backwards = "{} -- {}".format(key2, key)
                                                # add the edge if it hasn't already been added
                                                if interference_str not in solution and interference_str_backwards not in solution:
                                                        solution.append(interference_str)

                # End algorithm
                self.ig = solution

        # Begin helpers for chaitin's algorithm  
        def remove_edges_between(self, web1, web2, graph):
                for i in range(len(graph)):
                        # first go through and delete web2 from web1 edges
                        if graph[i][0] == web1:  # 0 index is web
                                graph[i][1].remove(web2)  # index 1 is edges
                        # Then repeat for web2
                        if graph[i][0] == web2:  # 0 index is web
                                # graph[i][1] = graph[i][1].remove(web1)
                                graph[i][1].remove(web1)
                                
                        # reset index 1 to list of all items have been removed
                        if graph[i][1] == None:
                                graph[i][1] = []
                                
                return graph
        
                
        def get_basic_block_instruction_indexes(self):
                num_blocks = int(self.bb[-1][1])  # index last block to get total num blocks
                blocks_dict = {}
                # initialize empty dict values
                for i in range(1, num_blocks+1):
                        blocks_dict[str(i)] = []
                # make dict of keys for bb and values of line number
                for i, line in enumerate(self.bb):
                        if line[1] != 'None':
                                blocks_dict[line[1]].append(i)
                
                return blocks_dict
        
        def get_basic_block_of_label(self, label):
                for i, line in enumerate(self.bb):
                        print(line[0][0:-1])
                        if line[0][0:-1] == label:  # indexing is to remove the ':' at the end of str
                                return self.bb[i+1][1]

        # Chaitin's Graph Coloring
        # fb)
        # Here we will note what the resulting node and color is, similar to 1a use a number to indicate which color each node will be. 
        # For example 1,2,3 and spill
        @check_chaitins_graph_coloring
        def chaitins_graph_coloring(self):
                '''
                Result: Dict of instructions corresponding to their coloring definitions.
                        Example:
                        {'a1': '1', 
                        'b1': '2', 
                        'c1': '3', 
                        'd1': "spill"}
                '''
                import copy
                solution = {}
                # Begin Algorithm
                
                # first go through and create a dictionary of nodes with values list as edges
                nodes_to_other_nodes = {}
                for interference in self.ig:
                        # add destination edge to key at index 0
                        if interference[:2] not in nodes_to_other_nodes:
                                # create the list for the dict entry if not exists
                                nodes_to_other_nodes[interference[:2]] = []
                        nodes_to_other_nodes[interference[:2]].append(interference[-2:])
                        # repeat above for the target node in the self.ig since it's non directional
                        if interference[-2:] not in nodes_to_other_nodes:
                                nodes_to_other_nodes[interference[-2:]] = []
                        nodes_to_other_nodes[interference[-2:]].append(interference[:2])
                
                graph = []
                print(nodes_to_other_nodes)
                print(self.webs)
                print('graph')
                
                # now find the loops to help compute the spill cost later
                bb_instructs_at = self.get_basic_block_instruction_indexes()
                loops_encountered = []
                line_points_that_are_loops = set()
                # iterate over lines to find which basic blocks are looped
                for i in range(len(self.bb)):
                        # get the instruciton and clean it up for processing
                        instruct = self.bb[i][0]
                        instruct = instruct.split(', ')
                        if instruct[-1][-1] == ',':
                                instruct[-1] = instruct[-1][:-1]  # shave off that la
                        
                        # if it's a label, add it to the stack
                        if instruct[0][0:5] == "label":
                                loops_encountered.append(instruct[0][0:-1])  # get rid of the ':'
                        # if its a pointer to a label, and that label is in the stack, its a loop!
                        elif "label" in instruct[-1] and instruct[-1] in loops_encountered:
                                basic_block_of_label = self.get_basic_block_of_label(instruct[-1])
                                line_points_that_are_loops.update(bb_instructs_at[basic_block_of_label])
                                        
                
                # now convert that dictionary of nodes and edges to a list entry to include spill cost
                for key, value in nodes_to_other_nodes.items():
                        spill_cost = 0  # want to tag spill cost as metadata
                        for live_point in self.webs[key]:
                                # spill is defined as a sum of all uses and defs
                                if key[0] in self.defs[int(live_point)]:
                                        # if def or use in loop, it's 10 times more spill
                                        if int(live_point) in line_points_that_are_loops:
                                                spill_cost += 10
                                        else:
                                                spill_cost += 1
                                if key[0] in self.uses[int(live_point)][1]:
                                        if int(live_point) in line_points_that_are_loops:
                                                spill_cost += 10
                                        else:
                                                spill_cost += 1
                        # important values will be indexed out as shown below
                        graph.append([key, value, spill_cost])
                
                # sort lists from highest spill cost to lowest
                sorted_graph = sorted(graph, key=lambda x: x[-1], reverse=True)
                print(sorted_graph)
                
                colors = ['1', '2', '3']  # the three registers we have
                g_not_r_colored = True
                # begin Chaitin's!
                while g_not_r_colored:
                        temp_graph = copy.deepcopy(sorted_graph)
                        color_stack = []
                        potential_spill_index = 0
                        while len(temp_graph) != 0:
                                potential_spill_index += 1
                                # want to remove the web with least interference
                                temp_graph = sorted(temp_graph, key=lambda x: x[-1], reverse=True)
                                web_details = temp_graph.pop()
                                if len(web_details[1]) < 3:  # index one is edges
                                        # web details has edges trimmed, so graph the original web with all values still there
                                        color_stack.append([i for i in sorted_graph if i[0] == web_details[0]])
                                        for connection in web_details[1]:
                                                temp_graph = self.remove_edges_between(web1=web_details[0],
                                                                                        web2=connection,
                                                                                        graph=temp_graph)
                                                        
                                else:
                                        # this else triggers if graph can't be colored
                                        spill_this_web = sorted_graph.pop(len(sorted_graph) - potential_spill_index)
                                        # take the lowest spill cost node and spill it
                                        for connection in spill_this_web[1]:
                                                self.remove_edges_between(web1=spill_this_web[0],
                                                                        web2=connection,
                                                                        graph=sorted_graph)
                                        solution[spill_this_web[0]] = 'spill'
                                        potential_spill_index = 0
                                        temp_graph = copy.deepcopy(sorted_graph)
                                        color_stack = []
                                        continue
                        
                        temp_solution = {}
                        for key, value in self.webs.items():
                                temp_solution[key] = None
                        potential_color_i = 0
                        while len(color_stack) != 0:
                                web_details = color_stack.pop()[0]
                                for neighbor in web_details[1]:
                                        if temp_solution[neighbor] == colors[potential_color_i]:
                                                potential_color_i += 1
                                                potential_color_i = potential_color_i % 3
                                                if potential_color_i > len(colors):
                                                        raise AttributeError("SOMETHING IS WRONG")
                                temp_solution[web_details[0]] = colors[potential_color_i]
                                
                        g_not_r_colored = False
                                
                for key, value in temp_solution.items():
                        if key not in solution: 
                                solution[key] = value
                        
                print(solution)

                # End Algorithm
                self.coloring = solution       
        
# Question 1 - Part G
# Provide Answer in Report PDF

# Question 2
# Provide Answer in Report PDF

# Question 3
# Provide Answer in Report PDF

# Question 4
# a) Create Adjacency Graph
# Fill out the following;
# First element is a src->dst element
# The second is the weight
# Be sure to use the letters as defined in the problem
# For example:
# q4a = [["x->y", "3"],
#        ["y->z", "5"],
#
#

q4a = [["", ""],
       ["", ""],
       ["", ""],
       ["", ""],
       ["", ""]
       ]


# Question 4
# b) Generate Adjacency Graph
#
if q4a[0][0] != "":
    print ("Q4 Overwrite the text on the left window of this site https://dreampuf.github.io/GraphvizOnline/ with the following. Put the resulting graph in your report.")
    print ("Question 4 Start Digraph")
    print ("digraph G{")
    for i in q4a:
        print ("\t", i[0], "[ label=" + str(i[1]) + "];")
    print ("}")
    print ("Question 4 End Digraph")
    print ("")

# Question 4b
# Carry out Differential Remapping
# Iteration 1
# Mapping using the letters as keys and the word "cost" as a key, below is an EXAMPLE ONLY and not correct for this problem... you will have several more rows
# NOTE: The 1st row should be your starting vector (i.e., the mapping of your adjaceny graph).
q4b_a1 = [{"x":0, "z":0, "y":0, "u":0, "w":0, "v":0, "cost":0},
          {"x":0, "z":0, "y":0, "u":0, "w":0, "v":0, "cost":0},
          {"x":0, "z":0, "y":0, "u":0, "w":0, "v":0, "cost":0},
          {"x":0, "z":0, "y":0, "u":0, "w":0, "v":0, "cost":0},
          {"x":0, "z":0, "y":0, "u":0, "w":0, "v":0, "cost":0}]
          
# Optimal Mapping and cost for Iteration 1
q4b_b1 = {"x":0, "z":0, "y":0, "u":0, "w":0, "v":0, "cost":0}

# Iteration 2
# NOTE: The 1st row should be your starting vector (i.e., the mapping chosen from iteration 1).
q4b_a2 = [{"x":0, "z":0, "y":0, "u":0, "w":0, "v":0, "cost":0},
          {"x":0, "z":0, "y":0, "u":0, "w":0, "v":0, "cost":0},
          {"x":0, "z":0, "y":0, "u":0, "w":0, "v":0, "cost":0},
          {"x":0, "z":0, "y":0, "u":0, "w":0, "v":0, "cost":0},
          {"x":0, "z":0, "y":0, "u":0, "w":0, "v":0, "cost":0}]
# Optimal Mapping and cost
q4b_b2 = {"x":0, "z":0, "y":0, "u":0, "w":0, "v":0, "cost":0}

# DO NOT MODIFY BELOW THIS LINE
if __name__ == "__main__":
        allocator = Allocator()
        allocator.apply_functions()
        answers = {"q1a"  : allocator.bb,
                "q1b"  : allocator.cfg,
                "q1d1" : allocator.defs,
                "q1d2" : allocator.uses,
                "q1d3" : allocator.live_out,
                "q1d4" : allocator.live_in,
                "q1e"  : allocator.webs,
                "q1fa" : allocator.ig,
                "q1fb" : allocator.coloring,
                "q4a"  : q4a,
                "q4b_a1" : q4b_a1,
                "q4b_b1" : q4b_b1,
                "q4b_a2" : q4b_a2,
                "q4b_b2" : q4b_b2 }
        f = open('hw3_solution.json', 'w')
        f.write(json.dumps(answers, sort_keys = True, indent=4))
        f.close()
