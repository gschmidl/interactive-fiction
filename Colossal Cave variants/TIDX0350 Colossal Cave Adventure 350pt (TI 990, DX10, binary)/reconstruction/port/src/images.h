/* images.h - the original program and data (images.c, from src_original) */
#ifndef IMAGES_H
#define IMAGES_H

#define PROC_BASE 0x0000        /* procedure segment: code and constants */
#define PROC_SIZE 0xB294
#define TASK_BASE 0xB2A0        /* task segment: the transfer vector and data */
#define TASK_SIZE 0x2494
#define CAVE_LRL  1728          /* .GAMES.FILES.CAVE */
#define CAVE_RECS 28

extern const unsigned char adven_proc[PROC_SIZE];
extern const unsigned char adven_task[TASK_SIZE];
extern const unsigned char cave_data[CAVE_RECS * CAVE_LRL];

#endif
