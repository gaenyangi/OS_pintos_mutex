#include <stdio.h>
#include "threads/thread.h"
#include "threads/synch.h"
#include "projects/crossroads/vehicle.h"
#include "projects/crossroads/map.h"
#include "projects/crossroads/ats.h"

static struct lock step_lock;
static struct condition step_cond;     // Condition variable for synchronizing steps
static int vehicles_moved_count;       // Counter for vehicles that have moved in the current step
static int total_vehicles;             // Total number of vehicles

/* path. A:0 B:1 C:2 D:3 */
const struct position vehicle_path[4][4][12] = {
    /* from A */ {
        /* to A */
        {{4,0},{4,1},{4,2},{4,3},{4,4},{3,4},{2,4},{2,3},{2,2},{2,1},{2,0},{-1,-1},},
        /* to B */
        {{4,0},{4,1},{4,2},{5,2},{6,2},{-1,-1},},
        /* to C */
        {{4,0},{4,1},{4,2},{4,3},{4,4},{4,5},{4,6},{-1,-1},},
        /* to D */
        {{4,0},{4,1},{4,2},{4,3},{4,4},{3,4},{2,4},{1,4},{0,4},{-1,-1},}
    },
    /* from B */ {
        /* to A */
        {{6,4},{5,4},{4,4},{3,4},{2,4},{2,3},{2,2},{2,1},{2,0},{-1,-1},},
        /* to B */
        {{6,4},{5,4},{4,4},{3,4},{2,4},{2,3},{2,2},{3,2},{4,2},{5,2},{6,2},{-1,-1},},
        /* to C */
        {{6,4},{5,4},{4,4},{4,5},{4,6},{-1,-1},},
        /* to D */
        {{6,4},{5,4},{4,4},{3,4},{2,4},{1,4},{0,4},{-1,-1},}
    },
    /* from C */ {
        /* to A */
        {{2,6},{2,5},{2,4},{2,3},{2,2},{2,1},{2,0},{-1,-1},},
        /* to B */
        {{2,6},{2,5},{2,4},{2,3},{2,2},{3,2},{4,2},{5,2},{6,2},{-1,-1},},
        /* to C */
        {{2,6},{2,5},{2,4},{2,3},{2,2},{3,2},{4,2},{4,3},{4,4},{4,5},{4,6},{-1,-1},},
        /* to D */
        {{2,6},{2,5},{2,4},{1,4},{0,4},{-1,-1},}
    },
    /* from D */ {
        /* to A */
        {{0,2},{1,2},{2,2},{2,1},{2,0},{-1,-1},},
        /* to B */
        {{0,2},{1,2},{2,2},{3,2},{4,2},{5,2},{6,2},{-1,-1},},
        /* to C */
        {{0,2},{1,2},{2,2},{3,2},{4,2},{4,3},{4,4},{4,5},{4,6},{-1,-1},},
        /* to D */
        {{0,2},{1,2},{2,2},{3,2},{4,2},{4,3},{4,4},{3,4},{2,4},{1,4},{0,4},{-1,-1},}
    }
};

static int is_position_outside(struct position pos)
{
    return (pos.row == -1 || pos.col == -1);
}


/* return 0:termination, 1:success, -1:fail */
static int try_move(int start, int dest, int step, struct vehicle_info *vi)
{
    struct position pos_cur, pos_next;

    pos_next = vehicle_path[start][dest][step];
    pos_cur = vi->position;

    if (vi->state == VEHICLE_STATUS_RUNNING) {
        /* check termination */
        if (is_position_outside(pos_next)) {
            /* actual move */
            vi->position.row = vi->position.col = -1;
    vi->state = VEHICLE_STATUS_FINISHED;
            /* release previous */
    		if (!is_position_outside(pos_cur)) {
        		lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);
    		}
            return 0;
        }
    }

    /* Try to acquire the next position lock */
    if (lock_try_acquire(&vi->map_locks[pos_next.row][pos_next.col])) {
        if (vi->state == VEHICLE_STATUS_READY) {
            /* start this vehicle */
            vi->state = VEHICLE_STATUS_RUNNING;
        } else {
            /* release current position */
            lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);
        }
        /* update position */
        vi->position = pos_next;

        return 1;
    }
    return -1;
}


void init_on_mainthread(int thread_cnt) {
    lock_init(&step_lock);
    cond_init(&step_cond);
    vehicles_moved_count = 0;
    total_vehicles = thread_cnt;
}

void vehicle_loop(void *_vi) {
    int res;
    int start, dest, step;

    struct vehicle_info *vi = _vi;

    start = vi->start - 'A';
    dest = vi->dest - 'A';

    vi->position.row = vi->position.col = -1;
    vi->state = VEHICLE_STATUS_READY;

    step = 0;
    while (1) {
        /* vehicle main code */
        res = try_move(start, dest, step, vi);
        if (res == 1) {
            step++;
        }

        /* Synchronize step */
        lock_acquire(&step_lock);
        vehicles_moved_count++;
        if (vehicles_moved_count == total_vehicles) {
            /* All vehicles have attempted to move, increment step and notify all */
            crossroads_step++;
            unitstep_changed();
            vehicles_moved_count = 0;  // Reset for the next step
            cond_broadcast(&step_cond, &step_lock);
        } else {
            /* Wait until all vehicles have attempted to move */
            cond_wait(&step_cond, &step_lock);
        }

        /* Check termination condition */
        if (res == 0) {
            total_vehicles--;  // Decrement the total vehicle count when a vehicle finishes
		if (total_vehicles==0) {
                cond_broadcast(&step_cond,&step_lock);  // Wake up any remaining waiting threads
                lock_release(&step_lock);
                break;
            }
        }

        lock_release(&step_lock);

	if (res == 0) {
            break;
        }
    }

    /* status transition must happen before sema_up */
    vi->state = VEHICLE_STATUS_FINISHED;
}

