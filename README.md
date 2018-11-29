# Asst-3-Simple-Network-File-System

## Project Details
1. Run `./clientSNFS` and `./serverSNFS`, for example with port `8080`, client directory `/tmp/client_snfs/`, and server directory `/tmp/server_snfs` (the client and server directories are allowed to be the same name).
2. In `clientSNFS.c`  
  + A bunch of functions, one for each operation we have to support (i.e. open, close, read, write, getattr, readdir, etc.)
  + Each function does a few things:
    1. Sends a message to the server telling it what operation to perform on the file.
    2. Gets a message back from the server with the result of the operation. If the operation fails, then we should not proceed any further and instead return an error message to the user.
    3. Now that the server operation has succeeded, the client performs the same operation on its local copy of the file (in whatever the mounted directory is), and does a check to make sure the result of the client and server side operations are consistent. If they are the same, then returns the value to the user. If not, returns an error.
3. In `serverSNFS.c`
  + Waits for a message from the client (in a main thread).
  + When a message is received, a new worker thread is spawned to handle the message request.
  + In the worker thread:
    + Parses the request to determine what kind of operation to perform and gets any arguments that may be needed.
    + Performs the request (we don't have to use FUSE for this since we're not adding anything on top of these operations).
    + Returns the result of the request back to the user.
4. We are going to need some sort of clean up function that we can run when we want to stop using the port (can be specified as a command line argument)
