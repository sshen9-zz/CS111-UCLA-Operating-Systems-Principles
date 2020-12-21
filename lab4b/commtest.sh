#!/bin/bash
#Basic communication test script

(echo "START"; sleep 5; echo "OFF") | ./lab4b
if [ $? -ne 0 ]
then
	echo "Communication test error"
else
	echo "Communication test passed"
fi
