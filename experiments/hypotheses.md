## H1: The Difference between Open Addressing Map and Scalar Swiss Map will be minimal for primitive data types and great for larger data types like Strings
The only benefit that the scalar swiss map has over the normal open addressing map, is instead of having to compare keys whenever a full slot is found, we can first compare control 
bytes. There is only a 1/128 chance of a matching control byte having different keys, given a well-distributed hashing function, hence the average number of key comparisons goes down
significantly, as it is likely that on the first matching control byte, the keys will be the same. However, since comparing keys for primitives is very cheap, it is unlikely for there to be a significant performance improvement, and the extra memory allocated for the control vector may worsen memory usage.

## H2: The Swiss Map will perform the best at low load factors
Since 16 slots are checked simultanously, and the likelihood of false hits are low, then if the element isn't the map, it is very likely that an empty control byte will be read in the first 16 bytes, giving a very early return. At high load factors, it may be the case that there are many deleted slots, causing there to be no empty slot in the first few iterations.

## H3: The Open Addressing Map will perform very badly at high load factors for misses.
According to Knuth's analysis, the expected number of slots examined for a hit is (1 + 1/(1 − α)) and miss is ½ (1 + 1/(1 − α)²), where α is the load factor.
Substituting different values of α
α	    Hit	  Miss
0.5	    1.5	  2.5
0.75	2.5	  8.5
0.875	4.5	  32.5
0.9375	8.5	  128.5

We see that the expected number of slots needing to be examined for misses grows very large as load factors increase, since the (1 - α) term is squared.

## H4: Open Addressing, Scalar Swiss, and Swiss Map perform vastly better compared to std::unordered_map at high memory levels
Since unordered_map uses pointers to store its next element instead of storing its values in continguous arrays, the chance of cache misses grows as memory gets to large for lower level cache lines. 

## H5: Boost's Flat Map will perform the best overall
Boost's flat map has several features left out of my implementation that will likely increase lookup time and memory efficiency. First, instead of linear probing, Boost's flat map uses quadratic probing, which minimises the occurence and size of clusters. Moreover, Boost's flat map uses an overflow byte to state whether a possible key has been inserted, but the group of slots was full. This means that when the corresponding bit in the overflow byte is 0, Boost's map can immediately terminate even with the prescence of large clusters. Moreover, this approach removes the need for tombstones, minimising the need to rehash, as well as keeping the underlying arrays minimal.


