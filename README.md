# MESI
Implement MESI protocol using c++


Test cases
We have the following test cases: 
/***************************************/
Test 1: Single Processor Read-Write Test
In this test, a single processor loads from a location “A”.
The cache miss initiates a “Read” and the cache state for this particular line changes
from Invalid to Shared. The Processor now issues a store to the same location. 
This results in a cache hit and cache state now changes from shared to modified. 
This test is just to ensure the basic cache states. 

/***************************************/
Test 2: Sequencial consistency test of 2 processor
initially A = B = 0
processor 1 store 200 to A, then store 1 B
processor 2 while(B == 0) ; load A
Error cases : A not equals to 200 in processor 2


/***************************************/
Test 3: Another sequencial test of  3 processors
 initially, A = B = 0
p1 : A = 1      p2 : if(A==1) B = 1    p3 : if(B==1) load A
Error case A = 0 in processor 3


/***************************************/
Test 4:

Every  one store a balue in range R to A, then every one load A, see functionality of bit vector
Error case, if some processor load wrong value from A, which is not in range R



/***************************************/

Test 5:  Expand test 2, sequencial consistence for 32 processors

/***************************************/
Test 6:  Expand test 3, sequencial consistence for 32 processors


/***************************************/
Test 7: Check directory similar to test 4 

Every processor store to random location, and it is sure that every processor store to different location
Then every processor load from that location and see whether the value is correct







