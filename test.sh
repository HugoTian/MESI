echo "running test cases"

for folder in test/*/ ; do
    echo "running test case $folder"
    echo "copy test cases in to sim"
    cp test/folder/.  sim/  -R 
    cd sim
    echo "complile"
    make sim
    ./sim 32 100 0 1
    echo "finish running this case"
    cd ..
done