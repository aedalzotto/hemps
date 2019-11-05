#!/usr/bin/python
import os
import filecmp
import subprocess
import sys
from math import ceil
from shutil import copyfile
from shutil import rmtree

#------------ Python classes. See more in: http://www.tutorialspoint.com/python/python_classes_objects.htm
#This class is used to store the start time information of applications, usefull into repository generation process
class ApplicationStartTime:
	def __init__(self, name, start_time_ms, repo_address, arch, cid):
		self.name = name
		self.start_time_ms = start_time_ms #Stores the application descriptor repository address
		self.repo_address = repo_address
		self.arch = arch
		self.cid = cid

	def __hash__(self):
		return hash((self.name, self.arch))

	def __eq__(self, other):
		return self.name == other.name and self.arch == other.arch

#This class stores the repository lines in a generic way, this class as well as StaticTask are used in the app_builder module
class RepoLine:
    def __init__(self, hex_string, commentary):
        self.hex_string = hex_string
        self.commentary = commentary

#Python class used for store Cluste's information. 
class Cluster:
	def __init__(self, master_x, master_y, leftbottom_x, leftbottom_y, topright_x, topright_y, arch):
		self.master_x = master_x
		self.master_y = master_y
		self.leftbottom_x = leftbottom_x
		self.leftbottom_y = leftbottom_y
		self.topright_x = topright_x
		self.topright_y = topright_y
		self.arch = arch

def delete_if_exists(path_dir):
	if os.path.exists(path_dir):
		rmtree(path_dir, False, None)

def create_ifn_exists(path_dir):
	if not os.path.exists(path_dir):
		os.mkdir(path_dir)

def write_file_into_testcase(file_path, file_lines):
	tmp_file_path = file_path + "tmp"

	file = open(tmp_file_path, 'w+')
	file.writelines(file_lines)
	file.close()

	if os.path.isfile(file_path):
		#If the file are equals remove the new file tmp and then do nothing
		if filecmp.cmp(file_path, tmp_file_path) == True:
			os.remove(tmp_file_path)
			return
		
	copyfile(tmp_file_path, file_path)
	os.remove(tmp_file_path)

def get_app_task_name_list(testcase_path, app_name, arch):
	source_file = testcase_path + "/applications/" + arch + "/" + app_name
	
	task_name_list = []
	
	for file in os.listdir(source_file):
		if file.endswith(".c"):
			file_name = file.split(".")[0] #Splits the file name by '.' then gets the first element - [filename][.c]
			task_name_list.append(file_name)
			
	return task_name_list

def check_mem_size(arch, file_path, mem_size_KB):
	tool_prefix = "mips-elf-"
	if arch == "riscv":
		tool_prefix = "riscv64-unknown-elf-"

	program_memory_size = int(subprocess.getoutput(tool_prefix+"size "+file_path+" | tail -1 | sed 's/ //g' | sed 's/\t/:/g' | cut -d':' -f4"))

	file_size_KB = program_memory_size / 1024.0

	file_size_KB = ceil( file_size_KB + (file_size_KB * 0.2) )

	if file_size_KB >= mem_size_KB:
		sys.exit("ERROR: Insuficient memory size ("+str(mem_size_KB)+"KB) for file <"+file_path+"> size ("+str(file_size_KB)+"KB)")

	#This print needs more for: http://www.python-course.eu/python3_formatted_output.php
	#But currently I am very very very busy - so, if I is reading this, please...do it!!
	print ("Memory size ("+str(mem_size_KB)+"KB) OK for file <"+file_path+">\t with size ("+str(file_size_KB)+"KB)")

def create_cluster_list(x_mpsoc_dim, y_mpsoc_dim, x_cluster_dim, y_cluster_dim, clusters):
	if (x_mpsoc_dim % x_cluster_dim) or (y_mpsoc_dim % y_cluster_dim ):
		sys.exit('Error in YAML noc_dimension OR cluster_dimension - you must provide a compatible dimension')
		
	x_clusters_number = x_mpsoc_dim / x_cluster_dim
	y_clusters_number = y_mpsoc_dim / y_cluster_dim

	cluster_list = []

	idx = 0
	for y in range(0, int(y_clusters_number)):
		for x in range(0, int(x_clusters_number)):
			leftbottom_x =   x  * x_cluster_dim
			leftbottom_y =   y  * y_cluster_dim
			topright_x   =   ( (x+1) *  x_cluster_dim ) - 1
			topright_y   =   ( (y+1) *  y_cluster_dim ) - 1
			master_x =   x * x_cluster_dim
			master_y =   y * y_cluster_dim
			cluster_list.append(Cluster(master_x, master_y, leftbottom_x, leftbottom_y, topright_x, topright_y, clusters[idx]["arch"]))
			idx = idx + 1

	return cluster_list
