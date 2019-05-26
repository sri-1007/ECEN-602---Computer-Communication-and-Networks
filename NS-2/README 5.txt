ECEN - 602 NETWORK SIMULATION ASSIGNMENTS 
TEAM NUMBER: 11
MEMBERS : 	Srividhya Balaji : UIN : 827007169
		Sanjana Srinivasan: UIN: 927008860
Simulator used: NS 2
Usage:
ns <filename.tcl> <TCP flavor (all caps)> <case number>
TCP version: SACK, VEGAS
Case number: 1, 2, 3
Example Usage: ns ns.tcl VEGAS 3
		ns ns.tcl SACK 2
Simulation: 
* Initialize nodes, date flow, agents, links, application
* Proc finish {} writes the Average throughput of both the sources and executes the nam file
* Proc record {} calculates the throughput for every 0.5 second interval and loads the result into file 1 and 2 which is then added to get the total throughput i.e. thruput1 and thruput2 on the terminal window
* Both the sources start and continue sending the data from 0 to 400 seconds and finish is made to run at 400 seconds.





