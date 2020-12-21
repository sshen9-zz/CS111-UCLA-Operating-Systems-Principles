#!/bin/bash

# test regular input output
res=$(echo 'test' | ./lab0)
if [ $res == "test" ] && [ $? -eq 0 ] ; then
    echo "TEST CASE 1: Success"
else
    echo "TEST CASE 1: Fail"
fi


#test input output with specified file options
touch testinput
echo "input text" >> testinput
touch testoutput

./lab0 --input testinput --output testoutput
res=$(cat testoutput)
if [ "$res" == "input text" ] ; then
    echo "TEST CASE 2: Success"
else
    echo "TEST CASE 2: Fail"
fi

rm testinput
rm testoutput


#test segfault
./lab0 --segfault --catch
res=$?
if [ $res -eq 4 ] ; then
    echo "TEST CASE 3: Success"
else
    echo "TEST CASE 3: Fail"
fi

#test unrecognized arg
./lab0 --unrecognizedargument
res=$?
if [ $res -eq 1 ] ; then
    echo "TEST CASE 4: Success"
else
    echo "TEST CASE 4: Fail"
fi

#test unable to open input file

./lab0 --input randomfilename
if [ $? -eq 2 ] ; then
    echo "TEST CASE 5: Success"
else
    echo "TEST CASE 5: Fail"
fi


# #test unable to open output file
touch randomfilename
chmod 000 randomfilename
echo "text" | ./lab0 --output randomfilename

if [ $? -eq 3 ] ; then
    echo "TEST CASE 6: Success"
else
    echo "TEST CASE 6: Fail"
fi

chmod 777 randomfilename
rm randomfilename




