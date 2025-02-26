

```markdown
# Assignment 2
## CIS\*3110/Operating Systems
## W25

----------------------------------------------------------------

# Modelling a Token Ring using Semaphores

This exercise will give you some experience working with semaphores,
and designing systems to avoid deadlock.

# Objective

Write a C program that uses multiple Unix processes communicating via
a shared memory region to simulate a Token Ring network.

A Token Ring is one of the two most common types of Local Area Network.

# Program Design

## Overview

The program simulates a Token Ring LAN by forking off a process for each LAN node, that communicate via shared memory. Each node in the ring can send and receive data packets. The nodes use semaphores to synchronize access to the shared memory and avoid race conditions.

## Files

- `tokenRing.h`: Header file containing definitions and structures used in the program.
- `tokenRing_main.c`: Main program that sets up the system, runs the simulation, and cleans up resources.
- `tokenRing_setup.c`: Contains functions to set up and clean up the shared memory and semaphores.
- `tokenRing_simulate.c`: Contains the functions that simulate the behavior of each node in the Token Ring.
- `makefile`: Makefile to compile the program.

## Structures

### `data_pkt`

Represents a data packet in the Token Ring.

```c
struct data_pkt {
    unsigned char token_flag;
    unsigned char to;
    unsigned char from;
    unsigned char length;
    unsigned char data[MAX_DATA];
};
```

### `node_data`

Represents the data associated with each node in the Token Ring.

```c
struct node_data {
    struct data_pkt to_send;
    unsigned char data_xfer;
    int terminate;
    int sent;
    int received;
};
```

### `shared_data`

Represents the shared memory region containing data for all nodes.

```c
struct shared_data {
    struct node_data node[N_NODES];
};
```

## Functions

### `setupSystem`

Sets up the shared memory and semaphores.

### `cleanupSystem`

Cleans up the shared memory and semaphores.

### `token_node`

Simulates the behavior of a node in the Token Ring. Each node waits for a token, sends data if it has any, and passes the token to the next node.

### `send_pkt`

Sends a data packet followed by the token.

### `send_byte`

Sends a byte to the next node on the ring.

### `rcv_byte`

Receives a byte for the current node.

## Synchronization

The program uses semaphores to synchronize access to the shared memory. Each node has a pair of semaphores: one for indicating that the shared memory is empty and another for indicating that it is filled. The semaphores ensure that only one node accesses the shared memory at a time.

## Termination

The `terminate` flag is used to signal the nodes to terminate. The main program sets this flag when it is time to shut down the simulation. Each node checks this flag frequently and exits its loop when the flag is set.

## Compilation

Use the provided makefile to compile the program:

```sh
make
```

## Running the Simulation

Run the compiled program with the number of packets to be sent as an argument:

```sh
./tokensim <number_of_packets>
```

## Example

```sh
./tokensim 10
```

This command runs the simulation with 10 packets to be sent.

## Conclusion

This program simulates a Token Ring network using multiple Unix processes, shared memory, and semaphores. It demonstrates the use of synchronization mechanisms to avoid race conditions and ensure correct communication between processes.
```

