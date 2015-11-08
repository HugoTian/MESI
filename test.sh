echo "running test cases"

cd test

echo "running test case $1"
echo "copy test cases in to sim"
rm ../sim/proc.cpp
cp $1/proc.cpp  ../sim/
cd ../sim
echo "complile"
make sim
./sim 32 100 0 0 | grep -q "fail"
if [ $? -eq 0 ]  ; then
	echo "Fail for $1"
else		
	echo "Pass the $1"
fi
echo "finish running this case"
echo "====================================================================="
cd ../test
