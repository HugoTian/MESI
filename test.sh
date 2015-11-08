echo "running test cases"

cd test
for folder in * ; do
    echo "running test case $folder"
    echo "copy test cases in to sim"
    rm ../sim/proc.cpp
    cp $folder/proc.cpp  ../sim/
    cd ../sim
    echo "complile"
    make sim
    ./sim 32 100 0 0 | grep -q "fail"
    if [ $? -eq 0 ]  ; then
			echo "Fail for $folder"
	else		
			echo "Pass the $folder"
	fi
    echo "finish running this case"
    echo "====================================================================="
    cd ../test
done