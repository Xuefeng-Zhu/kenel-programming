# kenel_programming

## Implementation & Design Decisions
Step1: We Created a directory entry “/proc/mp1” within the Proc filesystem and created a file entry “/proc/mp1/status/” inside the directory
```
/* Helper function to create the directory entries for /proc */
void​create_mp1_proc_files(v​oid)​;
```

Step 2: Declared and initialized a linked list “pid_time_list” that contains PIDs and CPU times
of each registered process.
```
/* The linked list structure */
struct​pid_time_list {
s​truct​list_head list; /​* Kernel's list structure */ u​nsigned​l​ong​pid;
u​nsigned​l​ong​cpu_time;
};
/* Initialized the linked list */
INIT_LIST_HEAD(&pid_time_list.list);
```

Step 3: Callback functions for read and write in the entry of the proc filesystem. The write function allows processes to register their PIDs into the pid_time_list. In order to avoid two processes modify the linked list at the same time, we use spin_lock. The read function allows user to read the PIDs and CPU time value of each registered process. We increase the offset after first read, so it will only print once when we cat.
```
/* /proc file read op
occurs when a user runs cat /proc/mp1/status returns a list of PIDs and their CPU time.
*/
s s i z e _ t r e a d _ p r o c ( s​t r u c t ​f i l e * f i l p , c​h a r ​* u s e r , s i z e _ t c o u n t , l o f f _ t * o f f s e t ) ;
/* /proc file write op
occurs when a user runs ./process &
need to register the PID into the linked list
*/
s s i z e _ t w r i t e _ p r o c ( s​t r u c t ​f i l e * f i l p , c​o n s t ​c​h a r ​* u s e r , s i z e _ t c o u n t , l o f f _ t * o f f s e t ) ;
```
Step 4: Initialized a Linux Kernel Timer and set the expiration time to 5 seconds.
```
setup_timer(&cpu_timer, update_cpu_times, 0​​); mod_timer(&cpu_timer, jiffies + msecs_to_jiffies(5​000)​);
```

￼Step 5: Implemented the Top­Half of the Timer Interrupt Handler. Every 5 seconds, it initializes the work and enqueue the work to a work queue.
```
/* Callback for the kernel timer */
void​update_cpu_times(u​nsigned​l​ong​data);
```

Step 6: Implemented the Bottom­Half work function “cpu_use_wq_function”. It will traverse the link list and update CPU Time values of each registered process. In order to avoid two processes modify the linked list at the same time, we used spin_lock.
```
/* Callback for the work function to process cpu usage */
void​cpu_use_wq_function(s​truct​work_struct *work);
```

Step 7: Wrote a user space application “userapp” that implements the factorial calculation

Step 8: Memory leak checks: Our kernel module ensures that the resources that were allocated during its execution are freed which includes freeing allocated memory, stopping pending work function, destroying timers, workqueue and proc filesystem entry.
## Details of how to run the program
```
run “make”
sudo insmod mp1.ko
­ install the module
Step 2:
./userapp &
­ run the user space program “userapp” that registers itself into the module
Step 3:
cat /proc/mp1/status
­ read the pid & cpu time from the kernel space
Step 4:
./userapp & ./userapp &
­ run two user programs concurrently
Step 5:
cat /proc/mp1/status
­read the pid & cpu time from the kernel space
