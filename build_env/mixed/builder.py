#!/usr/bin/env python
import os
import filecmp
import sys
from testcase import testcase
from shutil import copyfile, rmtree
from deloream import generate_deloream_env
from utils import delete_if_exists
from utils import create_ifn_exists

def build(yaml_path, sim_time, hemps_path):
	# #HEMPS_PATH - must to point to the root directory of hemps
	# HEMPS_PATH = os.getenv("HEMPS_PATH", 0)

	# #Test if testcase file HEMPS_PATH is valid
	# if HEMPS_PATH == 0:
	# 	sys.exit("ENV PATH ERROR: HEMPS_PATH not defined")

	#Gets the testcase name:
	path_list = yaml_path.split("/") #The testcase path can have severals '/' into its composition
	yaml_name = path_list[len(path_list)-1] #Gets the last element of the split list
	test_name = yaml_name.split(".")[0]

	#Test if there are some differences between the input testcase file and the testcase file into testcase directory 
	#If there are differences, them deleted the testcase directory to rebuild a new testcase
	test_file = test_name+"/"+test_name+".yaml"
	if os.path.exists(test_file):
		test_changes = not filecmp.cmp(yaml_path, test_file)
		if test_changes:
			delete_if_exists(test_name)

	#Clean all undesired simulation files traces
	delete_if_exists(test_name+"/log")
	if os.path.exists(test_name+"/log"):
		os.system("cd "+test_name+"/log; rm -rf *")

	#Create the testcase path if not exist
	create_ifn_exists(test_name)

	#Reads some importats parameters from testcase
	reader = testcase(yaml_path)
	page_size_KB = reader.get_page_size_KB()
	memory_size_KB = reader.get_memory_size_KB()
	riscv_app_names = reader.get_arch_app_names("riscv")
	mipsi_app_names = reader.get_arch_app_names("mipsi")
	clusters = reader.get_clusters()

	# #Testcase generation: updates source files...
	copy_scripts(hemps_path, test_name)
	copy_kernel(hemps_path, test_name)
	copy_hardware(hemps_path, test_name)
	copy_testcase_file(test_name, yaml_path)
	copy_apps(hemps_path, test_name, riscv_app_names, mipsi_app_names)
	copy_makefiles(hemps_path, test_name, page_size_KB, memory_size_KB, riscv_app_names, mipsi_app_names, sim_time, clusters)

	#Create other importatants dirs
	create_ifn_exists(test_name+"/include")
	create_ifn_exists(test_name+"/log")
    
    # #Calls the deloream_env.py to generate all necessary debugging dir and files
	generate_deloream_env(test_name, reader)

# This fucntion copies the source files in source_dir to target_dir
#If you desire to add especific copies test, for example, ignore some specific files names or extensions, 
#please includes those file name or extension into the 3rd argument (ignored_names_list), the name can be the file name or its extension
def generic_copy(source_dir, target_dir, ignored_extension_list):
	exclude_extensions = " --exclude=".join(ignored_extension_list)
	command_string = "rsync -u -r --exclude="+exclude_extensions+" "+source_dir+"/ "+target_dir+"/"
	os.system(command_string)

def copy_scripts(hemps_path, testcase_path):
	source_script_path = hemps_path+"/build_env/mixed"
	testcase_script_path = testcase_path+"/build"
	generic_copy(source_script_path, testcase_script_path, [".pyc"])

#This funtion copies the software source files to the testcase/software path. The copied files are kernel and apps
def copy_kernel(hemps_path, testcase_path):
	source_sw_path = hemps_path+"/software"
	testcase_sw_path = testcase_path+"/software"

	create_ifn_exists(testcase_sw_path)
	create_ifn_exists(testcase_sw_path+"/riscv")
	create_ifn_exists(testcase_sw_path+"/mipsi")

	#--------------  COPIES ALL THE FILES .H AND .C FROM SOFTWARE DIRECTORY ----------------
	generic_copy(source_sw_path, testcase_sw_path, [".svn"])

# This fucntion copies the source files in hardware dir according with the system model description
#For example, to a SytemC description, all files in the hardware dir with the extension .ccp and .h will be copied
#If you desire to add especific copies test, for example, ignore some specific vhd files, please include those file name or extension
#into the 3rd argument (ignored_names_list), the name can be the file name or its extension
def copy_hardware(hemps_path, testcase_path):
	
	source_hw_path = hemps_path+"/hardware"
	testcase_hw_path = testcase_path+"/hardware"
	
	#Creates the direcoty into testcase path
	create_ifn_exists(testcase_hw_path)
		
	delete_if_exists(testcase_hw_path+"/vhdl")
	source_hw_path = source_hw_path+"/sc"
	testcase_hw_path = testcase_hw_path+"/sc"
	ignored_names_list = [".svn" , ".vhd"]
	
	generic_copy(source_hw_path, testcase_hw_path, ignored_names_list)

#This function copies the input testcase file to the testcase path.
#If the input testcase file is equal to the current testcase file, the copy is suspended. Further, the package generation is not fired 
#The function return if the testcase are equals = True or not equals = False
def copy_testcase_file(testcase_name, input_testcase_file_path):	
	current_testcase_file_path = testcase_name+"/"+testcase_name+".yaml"
	
	if os.path.isfile(current_testcase_file_path):
		#If the file are equals then do nothing
		if filecmp.cmp(input_testcase_file_path, current_testcase_file_path) == True:
			return True
		
	copyfile(input_testcase_file_path, current_testcase_file_path)
	return False

def copy_apps(hemps_path, testcase_path, riscv_app_names, mipsi_app_names):
	#--------------  COPIES ALL APP SOURCE FILES RELATED INTO TESTCASE FILE ----------------
	source_app_path = hemps_path+"/applications/"
	testcase_app_path = testcase_path+"/applications/"
	
	create_ifn_exists(testcase_app_path)
	create_ifn_exists(testcase_app_path+"riscv/")
	create_ifn_exists(testcase_app_path+"mipsi/")
		
	#for each app described into testcase file
	for app_name in riscv_app_names:
		source_app_dir = source_app_path + app_name
		target_app_dir = testcase_app_path + "riscv/" + app_name
		generic_copy(source_app_dir, target_app_dir, [".svn"])

	for app_name in mipsi_app_names:
		source_app_dir = source_app_path + app_name
		target_app_dir = testcase_app_path + "mipsi/" + app_name
		generic_copy(source_app_dir, target_app_dir, [".svn"])
		
	tc_riscv_apps = []
	tc_mipsi_apps = []
	
	#List as directories from applications directory
	for tc_app in os.listdir(testcase_app_path+"riscv/"):
		if os.path.isdir(testcase_app_path+"riscv/"+tc_app):
			tc_riscv_apps.append(tc_app)

	for tc_app in os.listdir(testcase_app_path+"mipsi/"):
		if os.path.isdir(testcase_app_path+"mipsi/"+tc_app):
			tc_mipsi_apps.append(tc_app)
		
	#Remove the apps already present into testcase 
	to_remove_riscv_apps = list ( set(tc_riscv_apps) - set(riscv_app_names) )
	to_remove_mipsi_apps = list ( set(tc_mipsi_apps) - set(mipsi_app_names) )
	
	for to_remove_app in to_remove_riscv_apps:
		delete_if_exists(testcase_app_path + "riscv/" + to_remove_app)

	for to_remove_app in to_remove_mipsi_apps:
		delete_if_exists(testcase_app_path + "mipsi/" + to_remove_app)

def copy_makefiles(hemps_path, testcase_path, page_size_KB, memory_size_KB, riscv_app_names, mipsi_app_names, simul_time, clusters):
	#--------------  COPIES THE MAKEFILE TO SOFTWARE DIR ----------------------------------
	makes_dir = hemps_path+"/build_env/recipes/"

	for cluster in clusters:
		if cluster["arch"] == "mipsi":
			copyfile(makes_dir + "software/mipsi.mk", testcase_path+"/software/mipsi/makefile")
			break

	for cluster in clusters:
		if cluster["arch"] == "riscv":
			copyfile(makes_dir + "software/riscv.mk", testcase_path+"/software/riscv/makefile")
			break

	copyfile(makes_dir + "hardware.mk", testcase_path+"/hardware/makefile")
	copyfile(makes_dir + "testcase.mk", testcase_path + "/makefile")

	# Open the file (closing after scope) to append the PAGE_SP_INIT and MEM_SP_INIT value
	software_path = testcase_path + "/software/"

	if len(riscv_app_names):
		lines = []
		lines.append("PAGE_SP_INIT = " + str((page_size_KB   * 1024)) + "\n")
		lines.append("MEM_SP_INIT  = " + str((memory_size_KB * 1024)) + "\n")
		copyfile(makes_dir + "applications.mk", testcase_path + "/applications/riscv/makefile")
		append_lines_at_end_of_file(software_path+"riscv/makefile", lines)

	if len(mipsi_app_names):
		lines = []
		lines.append("PAGE_SP_INIT = " + str((page_size_KB   * 1024) - 1) + "\n")
		lines.append("MEM_SP_INIT  = " + str((memory_size_KB * 1024) - 1) + "\n")
		copyfile(makes_dir + "applications.mk", testcase_path + "/applications/mipsi/makefile")
		append_lines_at_end_of_file(software_path+"mipsi/makefile", lines)

	for app_name in riscv_app_names:
		make_app_path = testcase_path + "/applications/riscv/" + app_name + "/makefile"
		
		copyfile(makes_dir + "application/riscv.mk", make_app_path)
		
		line = "PAGE_SP_INIT = "+ str((page_size_KB  *  1024)) + "\n"
		
		#Append the PAGE_SP_INIT value
		append_lines_at_end_of_file(make_app_path, line)

	for app_name in mipsi_app_names:
		make_app_path = testcase_path + "/applications/mipsi/" + app_name + "/makefile"
		
		copyfile(makes_dir + "application/mipsi.mk", make_app_path)
		
		line = "PAGE_SP_INIT = "+ str((page_size_KB  *  1024) - 1) + "\n"
		
		#Append the PAGE_SP_INIT value
		append_lines_at_end_of_file(make_app_path, line)

def append_lines_at_end_of_file(file_path, lines):
	f = open(file_path, "a")
	f.writelines(lines)
	f.close()
