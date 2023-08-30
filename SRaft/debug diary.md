2023/08/12
- each server and the raft node relate to it need to have different port
- when a follower recieve a RequestVote RPC, if the candidate's term is higher, the follower should change its own term to match the candidate's
- add a mutex to Skvserver. Because its Get and Set method could be called by multiple threads

2023/08/13
- add destructor to KvServer ans SkvServer
- use smart pointer to replace raw pointer

2023/08/14
- in SRaft, create a toDestroy variable. use this variable to join the timer thread. All thread must be joined

2023/08/15
- circular dependency of header files
- adding some checking, like grpc connection fail, index out of bound, pointer is null

2023/08/17
- the get method of the client should be blocked
- any number relates to log index should be int rather than uint
- (bug) raft doesn't stop: a thread of Skvserver doesn't join
- (bug) when put failed, it should do something
- note: gdb can't stop at lambda

2023/08/18
- working on persister
