#NAME: Samuel Shen, Chandrasekhar Suresh
#EMAIL: sam.shen321@gmail.com, chandra.b.suresh@gmail.com
#ID: 405325252, 605304814

# 0 ... successful execution, no inconsistencies found.
# 1 ... unsuccessful execution, bad parameters or system call failure.
# 2 ... successful execution, inconsistencies found.

import sys
import csv
from collections import defaultdict

class Block:
    def __init__(self, blocknum, inodenum, indirection, offset):
        self.blocknum = blocknum
        self.inodenum = inodenum
        self.indirection = indirection
        self.offset = offset
        
arglist = sys.argv
if len(arglist) != 2:
    print("Bad arguments")
    sys.exit(1)
    
in_file = arglist[1]

# NOTE: If you make these, sets, then wouldn't it ignore the error of two duplicate lines?
freeblocks = set()
freeinodes = set()
allocated_inodes = defaultdict(lambda: 0)
allocated_blocks = set()
direct_references = defaultdict(lambda: 0)
maxblocks = 0
maxinodes = 0
first_legal_block = 0
inode_size = 0
blocksize = 0
invalid_inodes = []
error_flag = False
#track all blocks
blocklist = []
#track block freq
blockdict = defaultdict(lambda: 0)
try:
    csv_file = open(in_file)
except:
    sys.stderr.write("Error opening file")
    exit(1)
    
csv_reader = csv.reader(csv_file, delimiter=',')

# NOTE: We don't know if the superblock and group lines will always be first
# NOTE: Its kind of pointless to be doing error checking while populating these data structures so we should probably only do error checking at the end after filling up our data structures
for row in csv_reader:
    #print(f"row:{row}")
    if row[0] == "SUPERBLOCK":
        maxblocks = row[1]
        maxinodes = row[2]
        blocksize = row[3]
        inode_size = row[4]
    if row[0] == "GROUP":
        total_group_inodes = row[3]
        inode_table_size = int(total_group_inodes) * int(inode_size) / int(blocksize)
        # NOTE: Isn't it just total_group_inodes? cause each inode is stored in a block i think
        first_legal_block = int(int(row[8]) + inode_table_size)
    if row[0] == "BFREE":
        freeblocks.add(row[1])
    if row[0] == "IFREE":
        freeinodes.add(row[1])
    if row[0] == "INODE":
        inode_num = row[1]
        filetype = row[2]
        imode = row[3]
        filesize = row[10]
        allocated_inodes[inode_num] = row[6]
        #if int(inode_num) < 1 or int(inode_num) > int(total_group_inodes):
        #    invalid_inodes.append(inode_num)
        if imode!=0 and inode_num in freeinodes:
            print("ALLOCATED INODE "+inode_num+" ON FREELIST")
            error_flag = True
        elif imode == 0 and inode_num not in freeinodes:
            print("UNALLOCATED INODE "+inode_num+" ON FREELIST")
            error_flag = True
        offset = [0, 12, 256 + 12, 256*256 + 256 + 12]
        if filetype == "f" or filetype == "d" or (filetype == "s" and int(filesize) > 60):
            #starts at row[12], goes until row[26]
            for i in range(12, 27):
                blocknum = row[i]
                if blocknum != "0":
                    allocated_blocks.add(blocknum)
                    indirection = max(i-23,0)
                    curblock = Block(blocknum, inode_num, indirection, str(offset[indirection]))
                    blocklist.append(curblock)
                    blockdict[blocknum]+=1
        
    if row[0] == "INDIRECT":
        blocknum = row[5]
        if blocknum != "0":
            allocated_blocks.add(blocknum)
            inode_num = row[1]
            offset = row[3]
            indirection_level = row[2]
            indirection = ""
            curblock = Block(blocknum, inode_num, indirection_level, offset)
            blocklist.append(curblock)
            blockdict[blocknum]+=1
            
    if row[0] == "DIRENT":
        direct_references[row[3]] += 1
        if row[6] == "'.'":
            if row[3] != row[1]:
                print(f"DIRECTORY INODE {row[1]} NAME '.' LINK TO INODE {row[3]} SHOULD BE {row[1]}")
                error_flag = True
        elif row[6] == "'..'" and row[1] == "2" and row[1] != row[3]:
            print(f"DIRECTORY INODE {row[1]} NAME '..' LINK TO INODE {row[3]} SHOULD BE {row[1]}")
        if row[3] in freeinodes and row[6] != "'.'" and row[6] != "'..'":
            print(f"DIRECTORY INODE {row[1]} NAME {row[6]} UNALLOCATED INODE {row[3]}")
            error_flag = True
        if (int(row[3]) < 1 or int(row[3]) > int(total_group_inodes)) and row[6] != "'.'" and row[6] != "'..'":

            print(f"DIRECTORY INODE {row[1]} NAME {row[6]} INVALID INODE {row[3]}")
            error_flag = True
            
#loop through blocks
indirection = [" ", " INDIRECT ", " DOUBLE INDIRECT ", " TRIPLE INDIRECT "]
for block in blocklist:
    
    # NOTE: Changed int(block.blocknum) < 0 to int(block.blocknum) < first_legal_block
    if int(block.blocknum) < 0 or int(block.blocknum) > int(maxblocks):
        print("INVALID"+indirection[block.indirection]+"BLOCK "+block.blocknum+" IN INODE "+block.inodenum+" AT OFFSET " + block.offset)
        error_flag = True
        
    # if block.blocknum == "0" and block.blocknum not in freeblocks:
    #     print("UNREFERENCED BLOCK "+block.blocknum)
    #     error_flag = True
        
    if block.blocknum in freeblocks:
        print("ALLOCATED BLOCK "+block.blocknum+" ON FREELIST")
        error_flag = True
        
    if int(block.blocknum) < first_legal_block and int(block.blocknum) >= 0:
        print("RESERVED"+indirection[block.indirection]+"BLOCK "+block.blocknum+" IN INODE "+block.inodenum+" AT OFFSET "+block.offset)
        error_flag = True
        
    if blockdict[block.blocknum] > 1:
        print("DUPLICATE"+indirection[block.indirection]+"BLOCK "+block.blocknum+" IN INODE "+block.inodenum+" AT OFFSET "+block.offset)
        error_flag = True



#block unallocated and in use
for i in range(first_legal_block, int(maxblocks)):
    blocknum = str(i)
    if blocknum not in allocated_blocks and blocknum not in freeblocks:
        print("UNREFERENCED BLOCK "+blocknum)
        error_flag = True 
    
#inode unallocated and in use
for i in range(11, int(maxinodes)):
    num = str(i)
    if num not in allocated_inodes and num not in freeinodes:
        print("UNALLOCATED INODE "+num+" NOT ON FREELIST")
        error_flag = True
        
for inode_num in allocated_inodes:
    if direct_references[inode_num] != int(allocated_inodes[inode_num]):
        print(f"INODE {inode_num} HAS {direct_references[inode_num]} LINKS BUT LINKCOUNT IS {allocated_inodes[inode_num]}")
        error_flag = True

if error_flag == True:
    sys.exit(2)
