#NAME: Samuel Shen
#EMAIL: sam.shen321@gmail.com
#ID: 405325252

import os

os.system("rm -f lab2_add.csv")

threads = [1,2,4,8,12]
iterations = [10, 20, 40, 80, 100, 1000, 10000, 100000]

for thread in threads:
    for iteration in iterations:
        os.system("./lab2_add --threads="+str(thread)+" --iterations="+str(iteration)+" >> lab2_add.csv")
        os.system("./lab2_add --yield --threads="+str(thread)+" --iterations="+str(iteration)+" >> lab2_add.csv")

        
iterations = [10, 20, 40, 80, 100, 1000, 10000]
for thread in threads:
    for iteration in iterations:
        os.system("./lab2_add --sync=m --threads="+str(thread)+" --iterations="+str(iteration)+" >> lab2_add.csv")
        os.system("./lab2_add --sync=m --yield --threads="+str(thread)+" --iterations="+str(iteration)+" >> lab2_add.csv")

for thread in threads:
    for iteration in iterations:
        os.system("./lab2_add --sync=c --threads="+str(thread)+" --iterations="+str(iteration)+" >> lab2_add.csv")
        os.system("./lab2_add --sync=c --yield --threads="+str(thread)+" --iterations="+str(iteration)+" >> lab2_add.csv")
        
iterations = [10, 20, 40, 80, 100, 1000, 10000]

for thread in threads:
    for iteration in iterations:
        os.system("./lab2_add --sync=s --threads="+str(thread)+" --iterations="+str(iteration)+" >> lab2_add.csv")
        os.system("./lab2_add --sync=s --yield --threads="+str(thread)+" --iterations="+str(iteration)+" >> lab2_add.csv")

