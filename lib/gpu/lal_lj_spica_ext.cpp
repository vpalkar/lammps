/***************************************************************************
                                  lj_spica.h
                             -------------------
                            W. Michael Brown (ORNL)

  Functions for LAMMPS access to lj/spica pair acceleration routines

 __________________________________________________________________________
    This file is part of the LAMMPS Accelerator Library (LAMMPS_AL)
 __________________________________________________________________________

    begin                :
    email                : brownw@ornl.gov
 ***************************************************************************/

#include <iostream>
#include <cassert>
#include <cmath>

#include "lal_lj_spica.h"

using namespace std;
using namespace LAMMPS_AL;

static CGCMM<PRECISION,ACC_PRECISION> CMMMF;

// ---------------------------------------------------------------------------
// Allocate memory on host and device and copy constants to device
// ---------------------------------------------------------------------------
int spica_gpu_init(const int ntypes, double **cutsq, int **cg_types,
                 double **host_lj1, double **host_lj2, double **host_lj3,
                 double **host_lj4, double **offset, double *special_lj,
                 const int inum, const int nall, const int max_nbors,
                 const int maxspecial, const double cell_size, int &gpu_mode,
                 FILE *screen) {
  CMMMF.clear();
  gpu_mode=CMMMF.device->gpu_mode();
  double gpu_split=CMMMF.device->particle_split();
  int first_gpu=CMMMF.device->first_device();
  int last_gpu=CMMMF.device->last_device();
  int world_me=CMMMF.device->world_me();
  int gpu_rank=CMMMF.device->gpu_rank();
  int procs_per_gpu=CMMMF.device->procs_per_gpu();

  CMMMF.device->init_message(screen,"lj/spica",first_gpu,last_gpu);

  bool message=false;
  if (CMMMF.device->replica_me()==0 && screen)
    message=true;

  if (message) {
    fprintf(screen,"Initializing Device and compiling on process 0...");
    fflush(screen);
  }

  int init_ok=0;
  if (world_me==0)
    init_ok=CMMMF.init(ntypes,cutsq,cg_types,host_lj1,host_lj2,host_lj3,
                       host_lj4, offset, special_lj, inum, nall, max_nbors,
                       maxspecial, cell_size, gpu_split, screen);

  CMMMF.device->world_barrier();
  if (message)
    fprintf(screen,"Done.\n");

  for (int i=0; i<procs_per_gpu; i++) {
    if (message) {
      if (last_gpu-first_gpu==0)
        fprintf(screen,"Initializing Device %d on core %d...",first_gpu,i);
      else
        fprintf(screen,"Initializing Devices %d-%d on core %d...",first_gpu,
                last_gpu,i);
      fflush(screen);
    }
    if (gpu_rank==i && world_me!=0)
      init_ok=CMMMF.init(ntypes,cutsq,cg_types,host_lj1,host_lj2,host_lj3,
                         host_lj4, offset, special_lj, inum, nall, max_nbors,
                         maxspecial, cell_size, gpu_split, screen);

    CMMMF.device->gpu_barrier();
    if (message)
      fprintf(screen,"Done.\n");
  }
  if (message)
    fprintf(screen,"\n");

  if (init_ok==0)
    CMMMF.estimate_gpu_overhead();
  return init_ok;
}

void spica_gpu_clear() {
  CMMMF.clear();
}

int** spica_gpu_compute_n(const int ago, const int inum_full,
                        const int nall, double **host_x, int *host_type,
                        double *sublo, double *subhi, tagint *tag, int **nspecial,
                        tagint **special, const bool eflag, const bool vflag,
                        const bool eatom, const bool vatom, int &host_start,
                        int **ilist, int **jnum, const double cpu_time,
                        bool &success) {
  return CMMMF.compute(ago, inum_full, nall, host_x, host_type, sublo,
                       subhi, tag, nspecial, special, eflag, vflag, eatom,
                       vatom, host_start, ilist, jnum, cpu_time, success);
}

void spica_gpu_compute(const int ago, const int inum_full, const int nall,
                     double **host_x, int *host_type, int *ilist, int *numj,
                     int **firstneigh, const bool eflag, const bool vflag,
                     const bool eatom, const bool vatom, int &host_start,
                     const double cpu_time, bool &success) {
  CMMMF.compute(ago,inum_full,nall,host_x,host_type,ilist,numj,
                firstneigh,eflag,vflag,eatom,vatom,host_start,cpu_time,success);
}

double spica_gpu_bytes() {
  return CMMMF.host_memory_usage();
}


