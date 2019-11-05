#!/usr/bin/python
import testcase
import os
from utils import delete_if_exists
from utils import write_file_into_testcase
from utils import get_app_task_name_list

def generate_deloream_env(test_name, reader):
	debug_path = test_name + "/debug"
	delete_if_exists(debug_path)
	os.mkdir(debug_path)
	os.mkdir(debug_path + "/pipe")
	os.mkdir(debug_path + "/request")
	services_cfg(test_name, debug_path, reader)
	platform_cfg(test_name, debug_path, reader)
	cpu_cfg(debug_path)

def cpu_cfg(debug_path):
	cpu_cfg_lines = []

	cpu_cfg_lines.append("Interruption\t65536\n")
	cpu_cfg_lines.append("Scheduler\t262144\n")
	cpu_cfg_lines.append("Idle\t524288\n")

	write_file_into_testcase(debug_path + "/cpu.cfg", cpu_cfg_lines)
    
def platform_cfg(testcase_path, debug_path, reader):
	mpsoc_x = reader.get_mpsoc_x_dim()
	mpsoc_y = reader.get_mpsoc_y_dim()
	cluster_x = reader.get_cluster_x_dim()
	cluster_y = reader.get_cluster_y_dim()
	manager_position_x = "0"
	manager_position_y = "0"
	global_manager_cluster = "0"
	flit_size = "32"
	clock_period_ns = "10"
	
	platform_cfg_lines = []
	
	platform_cfg_lines.append("router_addressing XY\n")
	platform_cfg_lines.append("channel_number 1\n")
	platform_cfg_lines.append("mpsoc_x "+str(mpsoc_x)+"\n")
	platform_cfg_lines.append("mpsoc_y "+str(mpsoc_y)+"\n")
	platform_cfg_lines.append("cluster_x "+str(cluster_x)+"\n")
	platform_cfg_lines.append("cluster_y "+str(cluster_y)+"\n")
	platform_cfg_lines.append("manager_position_x "+manager_position_x+"\n")
	platform_cfg_lines.append("manager_position_y "+manager_position_y+"\n")
	platform_cfg_lines.append("global_manager_cluster "+global_manager_cluster+"\n")
	platform_cfg_lines.append("flit_size "+flit_size+"\n")
	platform_cfg_lines.append("clock_period_ns "+clock_period_ns+"\n")
	
	platform_cfg_lines.append("BEGIN_task_name_relation\n")
	
	apps_start_list = reader.get_app_start_time_list()
	
	apps_name_relation = []
	
	apps_name_relation.append("BEGIN_app_name_relation\n")
	
	app_id = 0
	
	for app_obj in apps_start_list:
		
		app_name = app_obj.name
		
		task_name_list = get_app_task_name_list(testcase_path, app_name, app_obj.arch)
		
		task_id = 0
		
		apps_name_relation.append(app_name+"\t"+str(app_id)+"\n")
		
		for task_name in task_name_list:
			
			relative_id =  app_id << 8 | task_id
			
			platform_cfg_lines.append(task_name+" "+str(relative_id)+"\n")
			
			task_id = task_id + 1
		
		app_id = app_id + 1
	
	apps_name_relation.append("END_app_name_relation\n")
	
	platform_cfg_lines.append("END_task_name_relation\n")
	
	platform_cfg_lines = platform_cfg_lines + apps_name_relation
	
	write_file_into_testcase(debug_path+"/platform.cfg", platform_cfg_lines)

def services_cfg(testcase_path, debug_path, yaml_r):
	fp = open(testcase_path+"/software/include/services.h", "r")
	
	service_cfg_lines = []
	
	for line in fp:
		split_line = line.split()
		#Test if the string is in split_line and split line leght > 2
		if "#define" in split_line and len(split_line) > 2:
			value = int(split_line[2], 0)
			#value_hex = format(value, 'x') #Incompatible below version 2.6
			value_hex = "%x" % value
			service_cfg_lines.append(split_line[1]+" "+value_hex+"\n")
	fp.close()
	
	service_cfg_lines.append("\n")
	service_cfg_lines.append("$TASK_ALLOCATION_SERVICE 40 221\n")
	service_cfg_lines.append("$TASK_TERMINATED_SERVICE 70 221\n")
	
	write_file_into_testcase(debug_path+"/services.cfg", service_cfg_lines)
	