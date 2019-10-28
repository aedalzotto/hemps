/*!\file resource_manager.h
 * HEMPS VERSION - 8.0 - support for RT applications
 *
 * Distribution:  June 2016
 *
 * Created by: Marcelo Ruaro - contact: marcelo.ruaro@acad.pucrs.br
 *
 * Research group: GAPH-PUCRS   -  contact:  fernando.moraes@pucrs.br
 *
 * \brief This module defines the function relative to mapping a new app and a new task into a given slave processor.
 * This module is only used by manager kernel
 */

#ifndef RESOURCE_MANAGER_H_
#define RESOURCE_MANAGER_H_

#include "../../include/kernel_pkg.h"

/** Allocate resources to a Cluster by decrementing the number of free resources. If the number of resources
 * is higher than free_resources, then free_resourcers receives zero, and the remaining of resources are allocated
 * by reclustering
 * \param cluster_index Index of cluster to allocate the resources
 * \param nro_resources Number of resource to allocated. Normally is the number of task of an application
 */
static inline void allocate_cluster_resource(int cluster_index, int nro_resources)
{

	//puts(" Cluster address "); puts(itoh(cluster_info[cluster_index].master_x << 8 | cluster_info[cluster_index].master_y)); puts(" resources "); puts(itoa(cluster_info[cluster_index].free_resources));

	if (cluster_info[cluster_index].free_resources > nro_resources){
		cluster_info[cluster_index].free_resources -= nro_resources;
	} else {
		cluster_info[cluster_index].free_resources = 0;
	}

	//putsv(" ALLOCATE - nro resources : ", cluster_info[cluster_index].free_resources);
}

/** Release resources of a Cluster by incrementing the number of free resources according to the nro of resources
 * by reclustering
 * \param cluster_index Index of cluster to allocate the resources
 * \param nro_resources Number of resource to release. Normally is the number of task of an application
 */
static inline void release_cluster_resources(int cluster_index, int nro_resources){

	//puts(" Cluster address "); puts(itoh(cluster_info[cluster_index].master_x << 8 | cluster_info[cluster_index].master_y)); puts(" resources "); puts(itoa(cluster_info[cluster_index].free_resources));

	cluster_info[cluster_index].free_resources += nro_resources;

   // putsv(" RELEASE - nro resources : ", cluster_info[cluster_index].free_resources);
}

void page_used(int, int, int);

void page_released(int, int, int);

int map_task(int);

int application_mapping(int, int);

int SearchCluster(int, int);

#endif /* RESOURCE_MANAGER_H_ */
