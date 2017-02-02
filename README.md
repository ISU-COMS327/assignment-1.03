# Assignment 1.03: Path Finding

## Description

In this assignment, we implemented path finding. Eventually, we will have
monsters in this game that are seeking to the main player. We must create
a way for the monsters to find and reach the player in an efficient manner.
There will be two types of monsters: tunneling and non-tunneling. The
tunneling monsters will be able to dig through walls, and non-tunneling
monsters will just stay in corridors and rooms. Certain rocks may be harder
to break through, so a tunneling monster may be able to more quickly reach
the main player by using corridors instead of digging through the rock walls.

I solved this problem by implementing Dijkstra's algorithm, using a priority
queue. Information on this algorithm can be found
[here](https://en.wikipedia.org/wiki/Dtra's_algorithm#Using_a_priority_queue).

I really enjoyed doing this project. It was fun getting some real-world(ish)
experience using Dijkstra's algorithm.

## Usage

The usage information for this assignment is the same as the last assignment.
Click [here](https://github.com/ISU-COMS327/assignment-1.02#usage) to be
taken to the last assignments usage information.

You may include `--player_x=<x> --player_y=<y>` as parameters to set where
you would like the player to be initialized. Both parameters are required.
If not parameter is given, the player will be randomly placed. You *can* place
the player in an invalid location (outside of a room or corridor). However,
the non-tunneling monsters will not be able to reach him, and he is doomed
by the tunneling monsters. Show him some mercy.

**Note**: when you run this program, three renditions of the board will be
printed. The first rendition is the normal board view; the second rendition
is a map for the non-tunneling monsters; the third rendition is a map for
the tunneling monsters.
