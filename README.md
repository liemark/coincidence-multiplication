# coincidence-multiplication
This project explores a class of **non-standard multiplication rules** where digit-wise products are concatenated instead of summed with carries.
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
→ 1 | 54 | 16 = 15416  
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
Starting to compute all coincidence multiplication solutions for n=5, m=3

Parallel searching for primitive solutions where n=5, m=3 (using 16 threads)...
Found 21 sets of primitive solutions.

Parallel searching for primitive solutions where n=4, m=2 (using 16 threads)...
Found 0 sets of primitive solutions.

Parallel searching for primitive solutions where n=3, m=1 (using 16 threads)...
Found 0 sets of primitive solutions.

Displaying partial solutions
A = 10099, B = 991
A = 30724, B = 997
A = 20155, B = 997
A = 50879, B = 997
A = 10514, B = 994
A = 15278, B = 994
A = 18815, B = 995
A = 12500, B = 996
A = 10697, B = 986
A = 90455, B = 999
A = 12686, B = 989
A = 72346, B = 999
A = 54237, B = 999
A = 36128, B = 999
A = 18019, B = 999
A = 57750, B = 998
A = 10764, B = 988
A = 45634, B = 998
A = 53384, B = 998
A = 33518, B = 998
A = 41268, B = 998

Solutions that satisfy the no-zero-padding rule:
    A = 30724, B = 997
    A = 50879, B = 997
    A = 15278, B = 994
    A = 10697, B = 986
    A = 90455, B = 999
    A = 12686, B = 989
    A = 72346, B = 999
    A = 54237, B = 999
    A = 10764, B = 988
    A = 45634, B = 998
    A = 53384, B = 998
    A = 41268, B = 998
Verification completed. Found a total of 12 special solutions.

Search completed.
Total time: 0.0097645 seconds.
Found a total of 21 solution groups.
```
