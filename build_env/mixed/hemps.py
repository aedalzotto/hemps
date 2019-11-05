#!/usr/bin/env python
import os
import sys
from builder import build
from subprocess import call
from time import sleep

#1. This script encapsulate the call for testcase_builder.py
#2. Call the makefile (make all) inside the created testcase direcoty. This make call all the scripts to build apps, hw, kernel
#3. Simulate the system, if sc(gcc) it execute ./HeMPS, is scmod or vhdl it open ModelSim/Questa

ENABLE_GUI=1
HEMPS_PATH = os.getenv("HEMPS_PATH", 0)

def main():
	if HEMPS_PATH == 0:
		sys.exit("ERROR: Enviroment variable: HEMPS_PATH not defined")

	try:
		testcase_yaml = sys.argv[1]
		if not os.path.exists(testcase_yaml):
			raise Exception()
	except:
		sys.exit("ERROR: Invalid testcase file path passed as 1st argument, e.g: hemps-run <my_testcase>.yaml <time_in_ms>")

	try:
		simul_time = int(sys.argv[2])
		if simul_time < 0:
			raise Exception()
	except:
		sys.exit("ERROR: Invalid simulation time passed as 2nd argument, e.g: hemps-run <my_testcase>.yaml <time_in_ms>")

	build(testcase_yaml, simul_time, HEMPS_PATH)

	test_dir = os.path.splitext(testcase_yaml)[0]

	exit_code = call("make -C " + test_dir, shell=True)
	if exit_code != 0:
		sys.exit("\n*** Error: hemps-run stopped !!!\n")

	# if system_model == "sc":
	os.system("cd "+test_dir+";./"+test_dir+" -c "+str(simul_time)+" &" )

	# elif system_model == "scmod" or system_model == "vhdl":
	# 	subprocess.call("cd "+testacase_dir+"; vsim -do sim.do", shell=True)
	if ENABLE_GUI == 1:
		# if system_model == "sc":
		call_debugger(test_dir)

def call_debugger(testcase_path):
	traffic_router_path = testcase_path+"/debug/traffic_router.txt"
	platform_path = testcase_path+"/debug/platform.cfg"

	try_number = 0
	not_found = False

	while (not (os.path.exists(platform_path)) or (not os.path.exists(traffic_router_path))):
		if try_number == 10:
			not_found = True
			break
		sleep(0.5)
		try_number = try_number + 1

	if not not_found:
		os.system("java -jar "+HEMPS_PATH+"/build_env/HeMPS_Debugger.jar "+platform_path)

if __name__ == "__main__":
	main()
