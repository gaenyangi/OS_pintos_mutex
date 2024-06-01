#include <stdio.h>
#include "threads/thread.h"
#include "threads/synch.h"
#include "projects/crossroads/vehicle.h"
#include "projects/crossroads/map.h"
#include "projects/crossroads/ats.h"

/* Barrier to synchronize all vehicles */
static struct condition step_cond;
static struct lock step_lock;
static int vehicles_ready;
static int total_vehicles; 
static int active_vehicles; // currently active vehicles (decresed from total vehicles if some of them reaches destination)

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
static int try_move(int start, int dest, int step, struct vehicle_info *vi) {
    struct position pos_cur, pos_next;

    pos_next = vehicle_path[start][dest][step];
    pos_cur = vi->position;

    if (vi->state == VEHICLE_STATUS_RUNNING) {
        /* check termination */
        if (is_position_outside(pos_next)) {
            /* actual move */
            vi->position.row = vi->position.col = -1;
            /* release previous */
            lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);
            return 0;
        }
    }

    /* lock next position */
    lock_acquire(&vi->map_locks[pos_next.row][pos_next.col]);
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

void init_on_mainthread(int thread_cnt) {
    cond_init(&step_cond);
    lock_init(&step_lock);
    vehicles_ready = 0;
    total_vehicles = thread_cnt;
	active_vehicles = total_vehicles;
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

        /* termination condition. */
        if (res == 0) {
            lock_acquire(&step_lock);
            active_vehicles--; // decrease number of active vehicles if they arrive at destination
            vehicles_ready--;  // Ensure vehicles_ready is correct if a vehicle finishes
            if (active_vehicles == 0) {
                cond_broadcast(&step_cond, &step_lock); // Wake up all waiting threads if no active vehicles left
            }
            lock_release(&step_lock);
            break;
        }

        /* synchronization step */
        lock_acquire(&step_lock);
        vehicles_ready++;
        if (vehicles_ready == active_vehicles) {
            vehicles_ready = 0;
            crossroads_step++;
            unitstep_changed();
            cond_broadcast(&step_cond, &step_lock);
        } else {
		cond_wait(&step_cond, &step_lock);
        }
        lock_release(&step_lock);
    }

    /* status transition must happen before sema_up */
    vi->state = VEHICLE_STATUS_FINISHED;
}
