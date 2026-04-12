
# Networking C++ File Server Project

This project is a C++ networked file server and client system, designed for secure file transfer and management over TCP/IP. 

The server handles requests with a thread pool and it's based on the reactor pattern and event driven designs (poll). 

---

## Main Networking Concepts & Stack

- **Transport Layer:** Uses TCP sockets (`AF_INET`, `SOCK_STREAM`, `IPPROTO_TCP`) for reliable, connection-oriented communication.
- **Socket API:** Employs POSIX socket functions (`socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`) for both server and client.
- **Address Resolution:** Uses `getaddrinfo` for DNS and address resolution on the client side.
- **Concurrency:** The server uses a thread pool to handle multiple client connections concurrently, improving scalability and responsiveness.
- **Polling:** The server leverages `poll` for efficient I/O multiplexing, allowing it to manage multiple sockets.
- **Custom Protocol:** Defines custom request and response headers (see `Types.hpp`) for operations like authentication, upload, download, and file listing.
- **Binary Data Transfer:** Files are transferred in binary mode, with explicit file size and chunked reads/writes.
- **Authentication:** Username and password fields are included in the protocol for basic authentication.
- **Error Handling:** Custom error types and status codes are used for robust error reporting.

---

## Architecture

- **Server (`server/`):**
	- Listens for incoming TCP connections.
	- Handles authentication, file upload/download, and file listing requests.
	- Uses a thread pool for concurrent request processing.
	- Stores user files in a dedicated directory per user.
	- Implements custom logging for debugging and monitoring.

- **Client (`client/`):**
	- Connects to the server using TCP sockets.
	- Sends requests for authentication, file upload/download, and file listing.
	- Handles user input and file I/O.
	- Implements robust error handling and status reporting.

- **Common (`basic/`):**
	- Shared types and constants (e.g., buffer sizes, protocol headers, enums for request/response types).
	- Logging utilities.

---

## Key Technologies

- **C++17** (or later)
- **POSIX Sockets API**
- **Threading and Synchronization** (`std::thread`, `std::mutex`, `std::condition_variable`)
- **Filesystem API** (`std::filesystem`)
- **Makefile-based Build System**

