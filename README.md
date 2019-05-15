# SingleCycleCache

## TODO

- [x] Setup cache arrays (2d)
- [x] Replace mem references with cache funtion
- [x] GetCache function
- [x] PutCache function
- [x] Writeback function after halt

### Questions

- [x] Is there a max cache size?
- [x] 2D or 3D array (set, way, blk)?
- [x] Always get block from mem b/f sw?
- [x] Allocating for 2D array?
- [x] ADD FREES
- [x] Should we print anything on writeback?
- [x] ONLY A LITTLE
- [x] OTHER:
- [x] MASK ADDRESS ON STOREWORD

### Tests
+ -b 4 -s 3 -a 1

### Challenges
+ Working with 2d arrays
+ Proper eviction with block alignment
+ SW dirty-bit handling
