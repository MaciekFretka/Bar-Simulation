**<p align="center"> Operating Systems, Bar Simulation  </p>**
_________________________________
**<p align="center"> Wrocław University of Science and Technology </p>**
<p align="center"> Maciej Jaroński </p>


<a name="desc"></a>
# General info
Program for visualize simulation lifecycle of bar.

## Description
In bar we have barmans and clients. 

Every client have states : is sitting, is drinking, in queue, is in waiting (when he is waiting for beer), in queue to toilet, in toilet.
Every barman have states: is free, is pouring, is cleanning glass, is waiting for glass.
When client is thirsty, he go to the bar. If there is queue, he must wait. When he gets a beer, he drink it and back to table.
Barman can pour beer only if there are a clean mugs. If isn't he must clean all dirty mugs

Clients also have needs of going to toilet sometimes. Every beer is increase progress of that need.

## Visualisation
<a name="pre"></a>

# Prerequisites
- LINUX
- GCC
- C++
-ncurses
