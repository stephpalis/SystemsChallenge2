# Challenge 2 - Optmized Allocator

## Goal

Create a thread-safe, free-list based memory allocator without the use of malloc or free that is quicker than the system allocator.

## Strategy

The strategy for creating a fast allocator was to implement the Bucket system. I created a bucket system that had 8 buckets of sizes: 32, 64, 128,256, 512, 1028, 2046, and 4096. Upon initialization of the buckets, I allocated a chunk greater than a page for each bucket. Next, that chunk is split up into free list cells of that bucket's size. So, the bucket of size 32 consists of a free list where each cell is of size 32. So when memory is requested, the requested size is rounded up to the next bucket, and then this is removed from the free list of that bucket and returned to the user. If the requested size is greater than a page, this allocation is handled by our hmalloc function from hw7. For freeing the memory, first the total size of the item is found, and then this item is added to the bucket's list.  

## Discussion

In both cases, the hw7 allocator was the slowest by a significant amount, the par malloc was the second fastest, and system malloc was still the fastest. However, the par malloc function was very close to system time, and was even able to beat system time on certain occasions. Because these two were so close in time, it varied from test to test if par malloc as able to beat the system time. The utilization of the threads and the implementation of the bucket system in par malloc dramatically improved the results.
