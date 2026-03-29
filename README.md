# coincidence-multiplication
This project explores a class of **non-standard multiplication rules** where numerical products are connected instead of using carry sums, providing a fast solver for this problem.
## Problem Description
Given two integers:  
A = x · 10^m + y  
B = b  
Instead of standard multiplication, we define a "fake product":  
- Multiply corresponding digits  
- Concatenate results directly (no carry handling, no zero padding)
### Example
164 × 94  
Digit-wise:  
- 6 × 9 = 54  
- 4 × 4 = 16
Constructed result:  
1 | 54 | 16 = 15416  
Interestingly:  
164 × 94 = 15416  
This is a **coincidence** where the fake product equals the true product.
## Goal
Find all integer pairs (A, B) such that:  
A × B = digit-wise concatenation result  
## Method
The algorithm avoids brute force by:  
- Digit-by-digit recursive construction  
- Carry consistency constraints  
- Pruning invalid branches early  
- Final algebraic verification
Parallelization is used to accelerate the search.
## Performance
- Supports up to 8-digit × 7-digit search  
- Typically completes within **< 1 second** (multi-threaded)
## Example Output
```
Starting to compute all coincidence multiplication solutions for n=4, m=3

Parallel searching for primitive solutions where n=4, m=3 (using 16 threads)...
Found 27 sets of primitive solutions.

Parallel searching for primitive solutions where n=3, m=2 (using 16 threads)...
Found 27 sets of primitive solutions.

Parallel searching for primitive solutions where n=2, m=1 (using 16 threads)...
Found 0 sets of primitive solutions.

Displaying partial solutions
A = 1021, B = 981
……
A = 9190, B = 990

Solutions that satisfy the no-zero-padding rule:
    A = 2544, B = 964
    A = 4524, B = 984
    A = 1376, B = 924
    A = 1668, B = 924
    A = 1655, B = 897
    A = 1673, B = 888
    A = 2654, B = 958
    A = 6343, B = 989
Verification completed. Found a total of 8 special solutions.


Search completed.
Total time: 0.0090884 seconds.
Found a total of 54 solution groups.
```
