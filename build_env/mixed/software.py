#!/usr/bin/python
import sys
from utils import get_app_task_name_list
from testcase import testcase

def main():
	testcase_name = sys.argv[1]

	reader = testcase(testcase_name)

	page_size_KB =      reader.get_page_size_KB()
	memory_size_KB =    reader.get_memory_size_KB()
	max_local_tasks =   reader.get_tasks_per_PE()
	x_mpsoc_dim =       reader.get_mpsoc_x_dim()
	y_mpsoc_dim =       reader.get_mpsoc_y_dim()
	x_cluster_dim =     reader.get_cluster_x_dim()
	y_cluster_dim =     reader.get_cluster_y_dim()
	riscv_app_names = reader.get_arch_app_names("riscv")
	mipsi_app_names = reader.get_arch_app_names("mipsi")
	app_list = reader.get_app_start_time_list()

	generate_sw_pkg(x_mpsoc_dim, y_mpsoc_dim, x_cluster_dim, y_cluster_dim, app_list, max_local_tasks, page_size_KB)

def generate_sw_pkg(x_mpsoc_dim, y_mpsoc_dim, x_cluster_dim, y_cluster_dim, app_list, max_local_tasks, page_size_KB):
	cluster_list = create_cluster_list(x_mpsoc_dim, y_mpsoc_dim, x_cluster_dim, y_cluster_dim)

	master_number = len(cluster_list)
	max_slave_processors = (x_mpsoc_dim*y_mpsoc_dim) - master_number
	max_cluster_slave = (x_cluster_dim * y_cluster_dim) - 1
	apps_number = len(app_list)
	static_lenght = apps_number

	#Computes the max of tasks into an app
	max_task_per_app = 0

	for app in app_list:		
		task_count = len(get_app_task_name_list(".", app.name, app.arch))
		if task_count > max_task_per_app:
			max_task_per_app = task_count		

	file_lines = []
	#---------------- C SINTAX ------------------
	file_lines.append("#ifndef _KERNEL_PKG_\n")
	file_lines.append("#define _KERNEL_PKG_\n\n")

	file_lines.append("#define MAX_LOCAL_TASKS             "+str(max_local_tasks)+"  //max task allowed to execute into a single slave processor \n")
	file_lines.append("#define PAGE_SIZE                   "+str(page_size_KB*1024)+"//bytes\n")
	file_lines.append("#define MAX_TASKS_APP               "+str(max_task_per_app)+" //max number of tasks for the APPs described into testcase file\n")

	file_lines.append("\n//Only used by kernel master\n")
	file_lines.append("#define MAX_SLAVE_PROCESSORS        "+str(max_slave_processors)+"    //total of slave processors for the whole mpsoc\n")
	file_lines.append("#define MAX_CLUSTER_SLAVES          "+str(max_cluster_slave)+"    //total of slave processors for each cluster\n")
	file_lines.append("#define MAX_CLUSTER_APP             "+str(max_cluster_slave*max_local_tasks)+"    //max of app running simultaneously into each cluster\n")
	file_lines.append("#define XDIMENSION                  "+str(x_mpsoc_dim)+"     //mpsoc  x dimension\n")
	file_lines.append("#define YDIMENSION                  "+str(y_mpsoc_dim)+"     //mpsoc  y dimension\n")
	file_lines.append("#define XCLUSTER                    "+str(x_cluster_dim)+"     //cluster x dimension\n") 
	file_lines.append("#define YCLUSTER                    "+str(y_cluster_dim)+"     //cluster y dimension\n")
	file_lines.append("#define CLUSTER_NUMBER              "+str(master_number)+"     //total number of cluster\n")
	file_lines.append("#define APP_NUMBER                  "+str(apps_number)+"     //max number of APPs described into testcase file\n")
	file_lines.append("#define MAX_STATIC_APPS             "+static_lenght+"      //max number of tasks mapped statically \n\n")



	#file_lines.append("#ifdef IS_MASTER\n")
	file_lines.append("//This struct stores the cluster information\n")
	file_lines.append("typedef struct {\n")
	file_lines.append("    int master_x;		//master x address\n")
	file_lines.append("    int master_y;		//master y address\n")
	file_lines.append("    int xi;				//initial x address of the cluster perimeter\n")
	file_lines.append("    int yi;				//initial y address of the cluster perimeter\n")
	file_lines.append("    int xf;				//final x address of the cluster perimeter\n")
	file_lines.append("    int yf;				//final y address of the cluster perimeter\n")
	file_lines.append("    int free_resources;	//number of free pages available into the cluster's slave processors\n")
	file_lines.append("} ClusterInfo;\n\n")


	file_lines.append("extern ClusterInfo cluster_info[CLUSTER_NUMBER];\n\n")

	#Check if the list is not emply
	if static_mapping_list:
		file_lines.append("//Stores the app static mapping. It is a list of tuple = {app_id, cluster}\n")
		file_lines.append("extern unsigned int static_map[MAX_STATIC_APPS][2];\n\n")


	file_lines.append("#endif\n")

	#Use this function to create any file into testcase, it automatically only updates the old file if necessary
	writes_file_into_testcase("include/kernel_pkg.h", file_lines)


	file_lines = []

	file_lines.append("#include \"kernel_pkg.h\"\n\n")

	file_lines.append("ClusterInfo cluster_info[CLUSTER_NUMBER] = {")

	for cluster_obj in cluster_list:
		file_lines.append( "\n\t{"+str(cluster_obj.master_x)   +
							", "+ str(cluster_obj.master_y) +
							", "+ str(cluster_obj.leftbottom_x) +
							", "+ str(cluster_obj.leftbottom_y) +
							", "+ str(cluster_obj.topright_x) +
							", "+ str(cluster_obj.topright_y) +
							", "+ str(max_cluster_slave*max_local_tasks)+"},")
	file_lines.append("\n};\n\n")

	if static_mapping_list:
		
		static_table_string = ""
		
		#static_mapping_list is a list of tuple={app_id, cluster} and static_tuple is one instance (one tuple) of this list
		for static_tuple in static_mapping_list:
			
			app_id_str   = str ( static_tuple[0] )
			app_cluster_str = str ( static_tuple[1] )
			
			static_table_string = static_table_string + "{"+app_id_str+", "+app_cluster_str+"}, "
		
		#Removes the lost ',' at the end of string
		static_table_string = static_table_string[0:len(static_table_string)-2] # 2 because the space, other else will be 1
		
		file_lines.append("//Stores the app static mapping. It is a list of tuple = {app_id, cluster}\n")
		file_lines.append("unsigned int static_map[MAX_STATIC_APPS][2] = {"+static_table_string+"};\n\n")
			
			
	#Use this function to create any file into testcase, it automatically only updates the old file if necessary
	writes_file_into_testcase("include/kernel_pkg.c", file_lines)

if __name__ == "__main__":
	main()
