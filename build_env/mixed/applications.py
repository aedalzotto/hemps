#!/usr/bin/python
import sys
import os
import subprocess
from utils import ApplicationStartTime
from utils import RepoLine
from utils import write_file_into_testcase
from utils import get_app_task_name_list
from utils import check_mem_size
from math import ceil
from testcase import testcase

## @package app_builder
#This scripts compiles the application source code of the testcase and generates the repository and appstart text files

#When defined, a main function must be called in the last line of the script, take a look at the end of file
def main():
	testcase_name = sys.argv[1]
	reader = testcase(testcase_name)

	# riscv_app_names = reader.get_arch_app_names("riscv")
	# mipsi_app_names = reader.get_arch_app_names("mipsi")
	
	# COMES SORTED. EASY TO KEEP TRACK OF STATICALLY MAPPED APPS
	riscv_app_names = reader.get_arch_app_names("riscv")
	mipsi_app_names = reader.get_arch_app_names("mipsi")
	app_list = reader.get_app_start_time_list()

	generate_task_ids(app_list)

	# GENERATE PROPER MAKEFILE
	file_lines = []
	file_lines.append("all:\n")

	if len(riscv_app_names):
		file_lines.append("\tmake -C riscv/\n")

	if len(mipsi_app_names):
		file_lines.append("\tmake -C mipsi/\n")

	write_file_into_testcase("applications/makefile", file_lines)
	
	exit_status = os.system("cd applications/; make")
	
	if exit_status != 0:
		sys.exit("\nError compiling applications' source code\n")
	
	#Generate the repository.txt and repository_debug.txt files, returning a list of tuple {app_name, repo_address}
	apps_repo_addr_list = generate_repository(reader, app_list)
	
	#Generates the appstart.txt and appstart_debug.txt files
	generate_appstart(apps_repo_addr_list, app_list)

def generate_task_ids(app_list):
	for app in app_list:
		source_file = "applications/" + app.arch + "/" + app.name
		task_id_file_path = source_file + "/id_tasks.h"
		
		file_lines = []
		
		task_name_list = get_app_task_name_list(".", app.name, app.arch)
		
		task_id = 0
		
		for task_name in task_name_list:
			file_lines.append("#define\t"+task_name+"\t"+str(task_id)+"\n")
			task_id = task_id + 1
		
		#Use this function to create any file into testcase, it automatically only updates the old file if necessary
		write_file_into_testcase(task_id_file_path, file_lines)

def generate_repository(reader, app_list):
	TASK_DESCRIPTOR_SIZE = 26 #27 is number of lines to represent a task description 
	MAX_DEPENDENCES = 10

	#list(set(apps_name_list)) remove duplicates app name. Usefull to generate only one instance of the same app into repository
	app_list = list(set(app_list))

	#Returned list, containing a tuple {app_name, app_repo_address}
	apps_repo_addr_list = []
	
	#repolines is a list of RepoLine objetcts, the ideia is keep a list generic to be easily converted to any systax
	repo_lines = []
	
	#Used for point the next free address to fill with a given task code (task.txt file)
	initial_address = 0

	print("\n***************** task page size report ***********************")
	
	#Walk for all apps into /applications dir
	for app in app_list:
		#Stores the repository address of the application, this list is used to generate appstart
		apps_repo_addr_list.append([app.arch+"/"+app.name, toX(initial_address)])
		
		task_name_list = get_app_task_name_list(".", app.name, app.arch)
		
		app_tasks_number = len(task_name_list)
		
		#First line of app descriptor is the app task number
		repo_lines.append(RepoLine(toX(app_tasks_number), "task number to application "+app.arch+"/"+app.name+"")) 
		
		#This variable point to the end of the application header descriptor, this address represent where the 
		#app tasks code can be inserted
		initial_address = initial_address + ((TASK_DESCRIPTOR_SIZE * app_tasks_number) * 4) + 4 #plus 4 because the first word is the app task number
		
		task_id = 0
		
		#Walk for all app tasks into /applications/arch/app dir to fills the task headers
		for task_name in task_name_list:
			
			txt_size = get_task_txt_size(app.arch, app.name, task_name)
			
			#Points next_free_address to a next free repo space after considering the task code of <task_name>.txt
			repo_lines.append(RepoLine(toX(task_id), task_name+".c"))
			repo_lines.append(RepoLine(toX(txt_size), "txt size"))
			repo_lines.append(RepoLine(toX(get_task_DATA_size(app.arch, app.name, task_name)), "data size") )
			repo_lines.append(RepoLine(toX(get_task_BSS_size(app.arch, app.name, task_name)), "bss size") )
			repo_lines.append(RepoLine(toX(initial_address), "initial_address") )
			
			initial_address = initial_address + (txt_size * 4)
			
			repo_lines.append(RepoLine(toX(0), "computational load")) # TODO
			
			dependenc_list = []	# TODO
			
			#Fills the task dependences according with MAX_DEPENDENCES size
			comment = "communication load {task id, comm load in flits}"
			for i in range(0, MAX_DEPENDENCES):
				
				if ((i+1) < len(dependenc_list)):
					repo_lines.append(RepoLine(toX(dependenc_list[i]), comment))
					comment = ""
					repo_lines.append(RepoLine(toX(dependenc_list[i+1]), comment))
				else:
					repo_lines.append(RepoLine("ffffffff", comment))
					comment = ""
					repo_lines.append(RepoLine("ffffffff", comment))
					
			task_id = task_id + 1
			
		#After fills the task header, starts to insert the tasks code
		for task_name in task_name_list:
			txt_source_file = "applications/" + app.arch + "/" + app.name + "/" + task_name + ".txt"
			elf_source_file = "applications/" + app.arch + "/" + app.name + "/" + task_name + ".elf"
			
			check_mem_size(app.arch, elf_source_file, reader.get_page_size_KB())
			
			comment = task_name+".c"
			
			task_txt_file = open(txt_source_file, "r")
			
			for line in task_txt_file:
				file_line = line[0:len(line)-1] # removes the \n from end of file
				repo_lines.append(RepoLine(file_line, comment))
				comment = ""

			task_txt_file.close()
	
	################Finally, generates the repository file (main and debug files) ##########################
	print("***************** end task page size report *********************\n")

	generate_repository_file(repo_lines)

	print("\n***************** repository size report ***********************")
	check_repo_size(reader.get_repository_size_MB(), "repository.txt")
	print("***************** end repository size report ***********************\n")
	
	return apps_repo_addr_list

#Receives a int, convert to string and fills to a 32 bits word
def toX(input):
	hex_string = "%x" % input
	#http://stackoverflow.com/questions/339007/nicest-way-to-pad-zeroes-to-string
	return hex_string.zfill(8) # 8 is the lenght of chars to represent 32 bits (repo word) in hexa

def get_task_txt_size(arch, app_name, task_name):
	source_file = "applications/" + arch + "/" + app_name + "/" + task_name + ".txt"
	#http://stackoverflow.com/questions/845058/how-to-get-line-count-cheaply-in-python/1019572#1019572
	num_lines = sum(1 for line in open(source_file))
	return num_lines

def get_task_DATA_size(arch, app_name, task_name):
	source_file = "applications/" + arch + "/" + app_name + "/" + task_name + ".elf"
	tool_prefix = "mips-elf-"
	if arch == "riscv":
		tool_prefix = "riscv-unknown-elf-"

	#https://www.quora.com/What-is-a-convenient-way-to-execute-a-shell-command-in-Python-and-retrieve-its-output
	data_size = int (subprocess.getoutput(tool_prefix+"size "+source_file+" | tail -1 | sed 's/ //g' | sed 's/\t/:/g' | cut -d':' -f2"))

	while data_size % 4 != 0:
		data_size = data_size + 1
		
	data_size = data_size / 4

	return data_size
	
def get_task_BSS_size(arch, app_name, task_name):
	source_file = "applications/" + arch + "/" + app_name + "/" + task_name + ".elf"
	tool_prefix = "mips-elf-"
	if arch == "riscv":
		tool_prefix = "riscv-unknown-elf-"

	#https://www.quora.com/What-is-a-convenient-way-to-execute-a-shell-command-in-Python-and-retrieve-its-output
	bss_size = int(subprocess.getoutput(tool_prefix+"size "+source_file+" | tail -1 | sed 's/ //g' | sed 's/\t/:/g' | cut -d':' -f3"))

	while bss_size % 4 != 0:
		bss_size = bss_size + 1
		
	bss_size = bss_size / 4

	return bss_size

#This fucntion receives the repo_lines filled above and generates two files: the repository itself and a repository.lst
def generate_repository_file(repo_lines):
	repo_file_path = "repository.txt"
	repo_debug_file_path = "repository_debug.txt"

	file_lines = []
	file_debug_lines = []

	address = 0

	for repo_obj in repo_lines:
		file_lines.append(repo_obj.hex_string+"\n")
		debug_line = hex(address)+"\t\t0x"+repo_obj.hex_string+"\t"+repo_obj.commentary+"\n"
		address = address + 4
		file_debug_lines.append(debug_line)
		
	write_file_into_testcase(repo_file_path, file_lines)
	write_file_into_testcase(repo_debug_file_path, file_debug_lines)

def check_repo_size(yaml_repo_size, repo_path):
	f = open(repo_path)

	file_size_KB = (len(f.readlines()) * 4) #Each line has 4 bytes

	f.close()

	file_size_KB = ceil(file_size_KB / 1024.0)

	yaml_repo_size_KB = yaml_repo_size * 1024

	if file_size_KB >= yaml_repo_size_KB:
		sys.exit("ERROR: Insuficient repository size informed within yaml file. Current repository size is ("+str(file_size_KB)+"KB) for file <"+repo_path+">, informed size is ("+str(yaml_repo_size_KB)+"KB)")

	#This print needs more for: http://www.python-course.eu/python3_formatted_output.php
	#But currently I am very very very busy - so, if I is reading this, please...do it!!
	print ("Repository size ("+str(file_size_KB)+"KB) OK for size informed within yaml file ("+str(yaml_repo_size_KB)+"KB)")

#------- Repository Generation Scope ------------------- 
#apps_repo_addr_list is a list of tuples = {app_name, repo_address}
def generate_appstart(apps_repo_addr_list, app_list): 
	#For each apps_start_onj into apps_start_obj_list 
	for app in app_list:
		app_address = ""

		#Search for it respective app name into apps_repo_addr_list and extract the repo address
		for app_name_repo in apps_repo_addr_list:
			if app_name_repo[0] == app.arch+"/"+app.name:
				app_address = app_name_repo[1]
				break
		
		if app_address == "":
			sys.exit("ERROR: App name "+app.arch+"/"+app.name +" is out of the repository")
		
		
		app.repo_address = app_address
		
	generate_appstart_file(app_list)

def generate_appstart_file(app_list):
	appstart_file_path = "appstart.txt"

	appstart_debug_file_path = "appstart_debug.txt"

	file_lines = []
	file_debug_lines = []

	address = 0

	for app in app_list:
		file_lines.append( str(app.repo_address) +"\n")
		file_lines.append( str(app.start_time_ms) +"\n")
		
		file_debug_lines.append(hex(address)+"\t\t0x"+str(app.repo_address)+"\trepo address\t\t["+app.arch+"/"+app.name+"]\n")
		address = address + 4
		file_debug_lines.append(hex(address)+"\t\t0x"+str(app.start_time_ms)+"\tstart time in ms\t["+app.arch+"/"+app.name+"]\n")
		address = address + 4

	file_lines.append( "deadc0de\n")
	file_debug_lines.append(hex(address)+"\t\t0xdeadc0de\tend of file indicator\n")
		
	write_file_into_testcase(appstart_file_path, file_lines)
	write_file_into_testcase(appstart_debug_file_path, file_debug_lines)

if __name__ == "__main__":
	main()