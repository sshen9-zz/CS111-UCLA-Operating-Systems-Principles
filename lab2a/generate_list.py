#NAME: Samuel Shen
#EMAIL: sam.shen321@gmail.com
#ID: 405325252

import os
os.system("rm -f lab2_list.csv")

iterations = [10, 100, 1000, 10000, 20000]
for iteration in iterations:
    os.system("./lab2_list --threads=1 --iterations="+str(iteration)+" >> lab2_list.csv")

threads = [2,4,8,12]
iterations = [1, 10, 100, 1000]

for thread in threads:
    for iteration in iterations:
        os.system("./lab2_list --threads="+str(thread)+" --iterations="+str(iteration)+" >> lab2_list.csv")

iterations = [1,2,4,8,16,32]
yieldopts= ['i', 'd', 'l', 'id', 'il', 'dl', 'idl']

for thread in threads:
    for iteration in iterations:
        for yieldopt in yieldopts:
            os.system("./lab2_list --threads="+str(thread)+" --iterations="+str(iteration)+" --yield="+str(yieldopt)+" >> lab2_list.csv")
            os.system("./lab2_list --threads="+str(thread)+" --iterations="+str(iteration)+" --yield="+str(yieldopt)+" --sync=m"+" >> lab2_list.csv")
            os.system("./lab2_list --threads="+str(thread)+" --iterations="+str(iteration)+" --yield="+str(yieldopt)+" --sync=s"+" >> lab2_list.csv")

threads = [1,2,4,8,12,16,24]
iterations = [1, 10, 100, 500, 1000]
for thread in threads:
    for iteration in iterations:
        os.system("./lab2_list --threads="+str(thread)+" --iterations="+str(iteration)+" --sync=m"+" >> lab2_list.csv")
        os.system("./lab2_list --threads="+str(thread)+" --iterations="+str(iteration)+" --sync=s"+" >> lab2_list.csv")
