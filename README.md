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
📦 Concurrent-File-Server
┣ 📜 Makefile
┣ 📜 server.c → Handles client connections and task dispatching
┣ 📜 client.c → Simple client to connect and send commands
┣ 📜 queue.c/.h → Thread-safe queue implementation
┣ 📜 threadpool.c/.h → Thread pool for worker threads
┣ 📜 task.c/.h → Task structure and handler functions
┣ 📜 user.c/.h → User management, sessions, and per-file locks
┣ 📜 README.md → Project documentation and run instructions

