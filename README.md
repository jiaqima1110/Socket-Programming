## name: 
## student ID:  
## What have done for each code file:
### Phase 1: Boot-up
serverM: 
- Clean all message buffers
- Setup UDP socket
- Read and save names from serverA and serverB through UDP socket
- Setup TCP socket
- Read and save input names from client through TCP socket

serverA & serverB:
- Read info from .txt files
- Clean all message buffers
- Setup UDP socket
- Send names to serverM through UDP socket

client:
- Clean all message buffers
- Setup TCP socket
- Readin user's input and send input names to serverM through TCP socket

### Phase 2: Forward
serverM: 
- Check input names from client
- For names that are not in serverA or serverB, reply to client through TCP socket
- For names in serverA, forward these names to serverA through UDP socket
- For names in serverB, forward these names to serverB through UDP socket

serverA & serverB:
- Receive names from serverM

client:
- If there are names neither in serverA or serverB, receive the reply from serverM

### Phase 3: Schedule
serverA & serverB:
- Calculate schedule for all names received in Phase 2

### Phase 4: Reply
serverA & serverB:
- Forward calculated results to serverM

serverM: 
- Receive schedule results from serverA and serverB
- Calculate final schedule from result of A and result of B
- Forward final schedule to client

client:
- Receive final schedule and print it out

### Extra Part: Register and Update
client:
- Promp for user to input final meeting time
- Check whether the input is valid
- Forward valid input to serverM
- Receive the update information from serverM
- Start a new request

serverM: 
- Receive final meeting time from client
- Forward final meeting time to serverA and serverB
- Receive updated information from serverA and serverB
- Forward updated information to client

serverA & serverB:
- Receive final meeting time from serverM
- Update stored time slots information for each involved name
- Send the original time slots information and updated time slots information to serverM

### The format of all exhanchange messages
- Usernames are concatenated and delimited by a comma or a whitespace (depends on different situation)
- Single time slot is delimited by a comma
- Multiple time slots are grouped with "[]" and delimited by a comma

### Idiosyncrasy
- No known idiosyncrasy

### Reused Code
[How to implement UDP sockets in C](https://www.educative.io/answers/how-to-implement-udp-sockets-in-c)
- How to clean message buffers
- How to create UDP socket
- How to set and bind port and IP
- How to send and receive message
- How to close the socket

[How to implement TCP sockets in C](https://www.educative.io/answers/how-to-implement-tcp-sockets-in-c)
- How to clean message buffers
- How to create TCP socket
- How to set and bind port and IP
- How to listen for clients and accept an incoming connection
- How to send and receive message
- How to close the socket

[How do I trim leading/trailing whitespace in a standard way?](https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way)