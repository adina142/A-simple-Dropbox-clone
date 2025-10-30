# A-simple-Dropbox-clone
# Concurrent File Server System 

##  Project Overview
This project implements a **multi-threaded file server system** in C using **thread pools, queues, and synchronized user management**.  
It supports multiple concurrent clients (including multiple sessions per same user), safe concurrent file operations, and robust communication between worker and client threads.

This is the **Phase 2 submission**, focusing on **concurrency, correctness, and robustness**.

---

##  Features Implemented
###  Phase 1
- Basic client-server communication using sockets  
- User sign-up, login, and session management  
- Command parsing and task submission through a thread pool  

###  Phase 2 Enhancements
- Supports **multiple simultaneous clients** and **multiple sessions per user**
- Worker → Client result delivery using **dedicated per-client session channels**
- **Thread-safe per-user and per-file locking** for concurrent access control
- **Atomic metadata updates** for consistency
- **Graceful shutdown** of sockets and worker threads
- **Memory leak and race condition testing** using Valgrind and ThreadSanitizer
- Clean and modular code organization with proper synchronization

---

##  Project Structure
 Concurrent-File-Server
┣  Makefile

┣  server.c → Handles client connections and task dispatching

┣  client.c → Simple client to connect and send commands

┣  queue.c/.h → Thread-safe queue implementation

┣ threadpool.c/.h → Thread pool for worker threads

┣ task.c/.h → Task structure and handler functions

┣  user.c/.h → User management, sessions, and per-file locks

┣  README.md → Project documentation and run instructions


---

## ⚙️ Build Instructions

### 1️⃣ **Clone the Repository**
```bash
git clone https://github.com/adina142/A-simple-Dropbox-clone.git
cd A-simple-Dropbox-clone


2️⃣ Compile the Project
make


This will build both:

server (the main server program)

client (a sample client to test connections)

▶️ Run Instructions
🖥️ Start the Server
./server


The server will start listening on the configured port (default: 8080).

💻 Run Multiple Clients

Open another terminal window for each client:

./client


You can run multiple clients simultaneously to test concurrency.


🧪 Testing and Validation
🔁 Concurrency Testing

Launch multiple clients and perform parallel operations (e.g., upload, download, delete).

The server ensures race-free execution using per-user and per-file locks.

🧼 Memory Leak Testing (Valgrind)
valgrind --leak-check=full ./server

🔒 Data Race Testing (ThreadSanitizer)
gcc -fsanitize=thread -g -o server server.c queue.c threadpool.c task.c user.c -pthread
./server

🧠 Worker → Client Communication Design

Each client session maintains its own dedicated communication channel through its socket and session structure.
When a worker thread completes a task, it writes the result directly to that client’s socket.
This ensures:

Race-free and consistent message delivery

Efficient per-client isolation

Scalability for multiple simultaneous sessions

Refer to the design report for detailed justification.

🧹 Clean Up

To remove all compiled files:

make clean

👩‍💻 Author

Adina Khalid
BSc Computer Science, Information Technology University
📧 Email: adinakhalid99@gmail.com

