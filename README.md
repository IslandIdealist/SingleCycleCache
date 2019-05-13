# SingleCycleCache

## TODO

- [x] Setup cache arrays (2d)
- [x] Replace mem references with cache funtion
- [x] GetCache function
- [ ] PutCache function
- [ ] Writeback function after halt

### Questions

- [x] Is there a max cache size?
- [x] 2D or 3D array (set, way, blk)?
- [x] Always get block from mem b/f sw?
- [x] Allocating for 2D array?
- [ ] ADD FREES
- [x] Should we print anything on writeback?
- [ ] ONLY A LITTLE
- [x] OTHER:
- [ ] MASK ADDRESS ON STOREWORD

### Tests


### Challenges
+ Working wiht 2d arrays
+ Proper eviction with block alignment
+ SW dirty-bit handling
