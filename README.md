# Reliable Data Transfer Protocol (RDT) Simulation

## Overview

In this laboratory programming assignment, you will be writing the sending and receiving transport-level
code for implementing a simple reliable data transfer protocol. There will be two versions of this
assignment that will take you progressively from easier to difficult level. The two versions are as follows:
(i) alternating-bit protocol and (ii) Go-Back-N. This lab should be fun since your implementation will differ
very little from what would be required in a real-world situation.
Since you probably don't have standalone machines (with an OS that you can modify), your code will
have to execute in a simulated hardware/software environment. However, the programming interface
provided to your routines, i.e., the code that would call your entities from above and from below is very
close to what is done in an actual UNIX environment. (Indeed, the software interfaces described in this
programming assignment are much more realistic that the infinite loop senders and receivers that many
texts describe). Stopping/starting of timers are also simulated, and timer interrupts will cause your timer
handling routine to be activated.
You are given a skeletal code and a network emulator as well in a file called rdt.c. You have to add your
code in this given file. In the last lab you have worked with NS-2. From studying the emulator’s code you
will also learn how to write such simulators using discrete event simulation; though this is not an objective
of this lab.