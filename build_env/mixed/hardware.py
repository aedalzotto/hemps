#!/usr/bin/python
import os
import sys
from testcase import testcase
from utils import create_cluster_list
from utils import write_file_into_testcase

## @package hw_builder
#This script compiles the hardware part inside the testcase dir
#It also generate the hardware includes hemps.h or hemps.vhd in the include directory inside the testcase
def main():
	testcase_name = sys.argv[1]
	
	reader = testcase(testcase_name)

	#Variables from yaml used into this function
	x_mpsoc_dim =       reader.get_mpsoc_x_dim()
	y_mpsoc_dim =       reader.get_mpsoc_y_dim()
	x_cluster_dim =     reader.get_cluster_x_dim()
	y_cluster_dim =     reader.get_cluster_y_dim()
	clusters = reader.get_clusters()
	cluster_list = create_cluster_list(x_mpsoc_dim, y_mpsoc_dim, x_cluster_dim, y_cluster_dim, clusters)

	page_size_KB = reader.get_page_size_KB()
	memory_size_KB = reader.get_memory_size_KB()
	repo_size_bytes = reader.get_repository_size_BYTES()
	app_number = reader.get_app_number()

	generate_hw_pkg(x_mpsoc_dim, y_mpsoc_dim, cluster_list, page_size_KB, memory_size_KB, repo_size_bytes, app_number)
	
	#compile_hw
	exit_status = os.system("cd hardware/; make")
	
	if exit_status != 0:
		sys.exit("\nError compiling hardware source code\n")

def generate_hw_pkg(x_mpsoc_dim, y_mpsoc_dim, cluster_list, page_size_KB, memory_size_KB, repo_size_bytes, app_number):
	pe_arch = []
	#----------- Generates PEs types ----------------
	is_master_list = []
	#Walk over x and y address
	for y in range(0, y_mpsoc_dim):
		for x in range(0, x_mpsoc_dim):
			#By default is a slave PE
			arch = ""
			master_pe = False
			for cluster in cluster_list:
				# First check from which cluster this PE is
				if x >= cluster.leftbottom_x and x <= cluster.topright_x and y >= cluster.leftbottom_y and y <= cluster.topright_y:
					# PE is inside the found cluster
					arch = cluster.arch
					if x == cluster.master_x and y == cluster.master_y:
						master_pe = True
					break
	
			pe_arch.append(arch)
			if master_pe == True:
				is_master_list.append(1) # master
			else:
				is_master_list.append(0) # slave
				
	#------------  Updates the include path -----------
	include_dir_path = "include"
	
	#Creates the include path if not exists
	if os.path.exists(include_dir_path) == False:
		os.mkdir(include_dir_path)
	
	generate_to_systemc(is_master_list, pe_arch, page_size_KB, memory_size_KB, repo_size_bytes, app_number, x_mpsoc_dim, y_mpsoc_dim)

def generate_to_systemc(is_master_list, pe_arch_list, page_size_KB, memory_size_KB, repo_size_bytes, app_number, x_mpsoc_dim, y_mpsoc_dim):
	string_pe_type_sc = ""
	string_pe_arch = ""

	#Walk over is master list
	for pe_type in is_master_list:
		string_pe_type_sc = string_pe_type_sc + str(pe_type) + ", "

	#Remove the lost ',' in the string_pe_type_vhdl
	string_pe_type_sc = string_pe_type_sc[0:len(string_pe_type_sc)-2] # 2 because the space, otherelse will be 1

	# Walk over arch list
	for pe_arch in pe_arch_list:
		string_pe_arch = string_pe_arch + str(1 if pe_arch == "riscv" else 0) + ", "

	#Remove the lost ',' in the string_pe_type_vhdl
	string_pe_arch = string_pe_arch[0:len(string_pe_arch)-2] # 2 because the space, otherelse will be 1

	file_lines = []
	#---------------- SYSTEMC SINTAX ------------------
	file_lines.append("#ifndef _HEMPS_PKG_\n")
	file_lines.append("#define _HEMPS_PKG_\n\n")

	file_lines.append("#define PAGE_SIZE_BYTES           "+str(page_size_KB*1024)+"\n")
	file_lines.append("#define MEMORY_SIZE_BYTES      "+str(memory_size_KB*1024)+"\n")
	file_lines.append("#define TOTAL_REPO_SIZE_BYTES   "+str(repo_size_bytes)+"\n")
	file_lines.append("#define APP_NUMBER           "+str(app_number)+"\n")
	file_lines.append("#define N_PE_X              "+str(x_mpsoc_dim)+"\n")
	file_lines.append("#define N_PE_Y              "+str(y_mpsoc_dim)+"\n")
	file_lines.append("#define N_PE                "+str(x_mpsoc_dim*y_mpsoc_dim)+"\n\n")

	file_lines.append("const int pe_type[N_PE] = {"+string_pe_type_sc+"};\n\n")
	file_lines.append("const int pe_arch[N_PE] = {"+string_pe_arch+"};\n\n")
	file_lines.append("#endif\n")

	#Use this function to create any file into testcase, it automatically only updates the old file if necessary
	write_file_into_testcase("include/hemps_pkg.h", file_lines)

if __name__ == "__main__":
	main()
