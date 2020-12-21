#NAME: Samuel Shen
#EMAIL: sam.shen321@gmail.com
#ID: 405325252

import os
os.system("rm -f lab2b_list.csv")

#graphs 1 and 2
threads = [1,2,4,8,12,16,24]
iterations = [1000]

for thread in threads:
    os.system("./lab2_list --threads="+str(thread)+" --iterations=1000 >> lab2b_list.csv --sync=m")
    os.system("./lab2_list --threads="+str(thread)+" --iterations=1000 >> lab2b_list.csv --sync=s")


#graph 3
threads = [1,4,8,12,16]
iterations = [1,2,4,8,16]
for thread in threads:
    for iteration in iterations:
        os.system("./lab2_list --threads="+str(thread)+" --iterations="+str(iteration)+" --lists=4 --yield=id >> lab2b_list.csv")


iterations = [10,20,40,80]
for thread in threads:
    for iteration in iterations:
        os.system("./lab2_list --threads="+str(thread)+" --iterations="+str(iteration)+" --lists=4 --yield=id --sync=m >> lab2b_list.csv")
        os.system("./lab2_list --threads="+str(thread)+" --iterations="+str(iteration)+" --lists=4 --yield=id --sync=s >> lab2b_list.csv")

# #graphs 4 and 5
threads = [1,2,4,8,12]
#1000 iterations
lists = [1,4,8,16]
for thread in threads:
    for li in lists:
        os.system("./lab2_list --threads="+str(thread)+" --lists="+str(li)+" --iterations=1000 --sync=m >> lab2b_list.csv")
        os.system("./lab2_list --threads="+str(thread)+" --lists="+str(li)+" --iterations=1000 --sync=s >> lab2b_list.csv")
        
