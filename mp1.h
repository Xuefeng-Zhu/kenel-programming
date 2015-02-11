#ifndef __MP1__
#define __MP1__

/* The linked list structure */
struct pid_time_list {
   struct list_head list; /* Kernel's list structure */
   unsigned long pid;
   unsigned long cpu_time;
};

/* Callback for the work function to process cpu usage */
void cpu_use_wq_function(struct work_struct *work);

/* Callback for the kernel timer */
void update_cpu_times(unsigned long data);

/* Helper function to delete the linked list */
void delete_pid_time_list(void);

/* Helper function to create the directory entries for /proc */
void create_mp1_proc_files(void);

/* /proc file read op */
ssize_t read_proc(struct file *filp, char *user, size_t count, loff_t *offset);

/* /proc file write op */
ssize_t write_proc(struct file *filp, const char *user, size_t count, loff_t *offset);

/* Called when module is loaded */
int __init mp1_init(void);

/* Called when module is unloaded */
void __exit mp1_exit(void);

#endif
