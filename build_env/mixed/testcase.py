#!/usr/bin/python
import yaml
import sys
from utils import ApplicationStartTime
from operator import attrgetter

## @package yaml_intf
#IMPORTANT: This file encapsulates the yaml reading process, abstracting this process to another modules
#For this reason, modification in the yaml sintax are only reflected here
#USE THIS FILE TO PROVIDE FUNCIONS TO ALL MODULES
#If you need to modify the yaml syntax, modify only this file, in a way to make
#the yaml reading process transparent to the other modules. This process is done by creating new functions in this module
#that can be called in the other modules
class testcase:
	def __init__(self, yaml_path):
		try:
			file = open(yaml_path, 'r')
		except:
			sys.exit('ERROR: No such following testcase file or directory (%s)!' % yaml_path)
		try:
			yaml_reader = yaml.load(file)
		except:
			sys.exit('ERROR: Incorrent YAML sintax!!\nThe YAML file does not support the character tab (\\t) (%s)!\n' % yaml_path)
        
		self.reader = yaml_reader

	def get_page_size_KB(self):
		return self.reader["hw"]["page_size_KB"]

	def get_tasks_per_PE(self):
		return self.reader["hw"]["tasks_per_PE"]

	def get_memory_size_KB(self):
		#page size number plus one (kernel page) x page_size_KB
		memory_size_KB = (self.get_tasks_per_PE() + 1) * self.get_page_size_KB()
		return memory_size_KB

	def get_clusters(self):
		cluster_list = self.reader["clusters"]
		# cluster_list = sorted(cluster_list, key=attrgetter("id"))
		return cluster_list

	def get_arch_app_names(self, arch):
		clusters = self.get_clusters()

		app_list = []

		for cluster in clusters:
			if cluster["arch"] == arch:
				for app in cluster["apps"]:
					app_list.append(app["name"])

		return app_list
	
	def get_mpsoc_x_dim(self):
		return self.reader["hw"]["mpsoc_dimension"][0]

	def get_mpsoc_y_dim(self):
		return self.reader["hw"]["mpsoc_dimension"][1]

	def get_cluster_x_dim(self):
		return self.reader["hw"]["cluster_dimension"][0]

	def get_cluster_y_dim(self):
		return self.reader["hw"]["cluster_dimension"][1]

	#Returns a list of objects ApplicationStartTime containing the app name, start time, and repo address
	#However repo address return -1, it is filled inside app_builder during the repositoriy generation
	def get_app_start_time_list(self):
		app_start_time_list = []
		
		clusters = self.get_clusters()

		for cluster in clusters:
			for app in cluster["apps"]:
				name = app["name"]
				arch = cluster["arch"]
				cid = cluster["id"]

				#If the time is not configured - default is zero
				try:
					start_time_ms = "%x" % int(app["start_time_ms"])
				except:
					start_time_ms = "%x" % 0

				repo_address = "-1"

				app_start_time_list.append(ApplicationStartTime(name, start_time_ms.zfill(8), repo_address, arch, cid))
				
		#Sort objects by start time. See more in : https://wiki.python.org/moin/HowTo/Sorting
		sorted_list = sorted(app_start_time_list, key=attrgetter('start_time_ms'))
		
		return sorted_list
		
	def get_repository_size_MB(self):
		return self.reader["hw"]["repository_size_MB"]

	def get_repository_size_BYTES(self):
		repo = self.get_repository_size_MB() * 1024 * 1024
		return repo

	def get_app_number(self):
		number = len(self.get_arch_app_names("riscv")) + len(self.get_arch_app_names("mipsi"))
		return number
	