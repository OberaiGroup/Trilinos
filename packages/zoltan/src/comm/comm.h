#include "mpi.h"

/* data structure for irregular communication */

struct Comm_Obj {
  int nsend;                 /* # of messages to send (not including self) */
  int nrecv;                 /* # of messages to recv (not including self) */
  int nself;                 /* 0 = no data to copy to self, 1 = yes */
  int nsendmax;              /* # of datums in largest send message */
  int *procs_to;             /* list of procs to send to */
  int *lengths_to;           /* # of datums to send to each proc w/ self */
  int *indices_to;           /* which datums to send to each proc w/ self */
  int *procs_from;           /* list of procs to recv from */
  int *lengths_from;         /* # of datums to recv from each proc */
  MPI_Request *request;      /* MPI requests for posted recvs */
  MPI_Status *status;        /* MPI statuses for Waitall */
  MPI_Comm comm;             /* duplicated MPI Comm for all communication */
};

typedef struct Comm_Obj COMM_OBJ;

/* function prototypes */

void comm_do(struct Comm_Obj *, char *, int, char *);
struct Comm_Obj *comm_create(int, int *, MPI_Comm, int *);
void comm_destroy(struct Comm_Obj *);
