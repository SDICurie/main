define restart
	monitor targets 0
	monitor reset halt

	# Setting an access watchpoint in the
	# Sensor Subsystem Configuration (SS_CFG) register
	# This one is going to be read by Quark, prior to set the bit
	# that will start the ARC.
	monitor wp 0xb0800600 4 a
	monitor resume
	# waiting to hit watchpoint
	monitor wait_halt 20000
	# removing watchpoint
	monitor rwp 0xb0800600

	# starting arc is done with a read / modify / write of the register
	# step will execute the instruction that |= ARC_RUN_REQUEST
	# clear the eax register to remove the ARC_RUN_REQUEST from the command
	# continue qrk execution ( that will wait for arc to start )
	monitor step
	monitor reg eax 0
	monitor resume

	#setting ARC program counter to reset vector address
	#and a hard breakpoint on main procedure.
	monitor targets 1
	monitor halt
	delete
	set $pc=__reset
	stepi
	br main
	c
	#waiting to hit bp.
	monitor wait_halt 20000
end

