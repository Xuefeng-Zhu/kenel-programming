#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>

#include "mp1_given.h"
#include "mp1.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP1");

#define DEBUG 1
#define PID_MAX_LENGTH 20
#define GLOBAL_RW_PERM 0666
#define DIR_NAME "mp1"
#define FILE_NAME "status"

/* The proc directory entry */
static struct proc_dir_entry *pdir_mp1;

/* The linked list containing the pids and cpu times 
   of each registered process */
static struct pid_time_list pid_time_list;

/* Timer used for the "top half" calls */
static struct timer_list cpu_timer;

/* Used for creating and traversing the linked list */
static struct list_head *head, *next;
static struct pid_time_list *tmp;

/* Work to get cpu usage */
static struct work_struct *cpu_use_work;

/* Work queue having the work to get cpu usage */
static struct workqueue_struct *cpu_use_wq;

/* Spinlock to protect the linked list */
static spinlock_t list_lock;

/* Available file operations for mp1/status */
struct file_operations proc_fops = {
   read: read_proc,
   write: write_proc
};

/* Helper function to delete the linked list */
void delete_pid_time_list(void) {
   list_for_each_safe(head, next, &pid_time_list.list) {
      tmp = list_entry(head, struct pid_time_list, list);
      list_del(head);
      kfree(tmp);
   }   
}

/* Helper function to create the directory entries for /proc */
void create_mp1_proc_files(void) {
   pdir_mp1 = proc_mkdir(DIR_NAME, NULL);
   proc_create(FILE_NAME, GLOBAL_RW_PERM, pdir_mp1, &proc_fops);
}

/* Helper function to delete the directory entries for /proc */
void delete_mp1_proc_files(void) {
   remove_proc_entry(FILE_NAME, pdir_mp1);
   remove_proc_entry(DIR_NAME, NULL);   
}

/* Occurs when a user runs cat /proc/mp1/status
   Needs to return a list of PID and their CPU time.
   Infinite loops when used with cat.  Probably an issue.
   Only prints out PIDs right now.
*/
ssize_t read_proc(struct file *filp, char *user, size_t count, loff_t *offset)
{
   int pos = 0;
   int len;
   char *pid = (char *)kmalloc(sizeof(count), GFP_KERNEL);

   spin_lock(&list_lock);
   list_for_each(head, &pid_time_list.list) {
      tmp = list_entry(head, struct pid_time_list, list);
      len = sprintf(pid + pos, "PID: %lu, %lu\n", tmp->pid, tmp->cpu_time);
      pos += len;
   }
   spin_unlock(&list_lock);   

   copy_to_user(user, pid, pos);
   kfree((void *)pid);

   return pos;
}

/* Occurs when a user runs ./process > /proc/mp1/status 
   Needs to register the PID into the linked list.
*/
ssize_t write_proc(struct file *filp, const char *user, size_t count, loff_t *offset)
{
   char pid_buf[20];
   tmp = (struct pid_time_list *)kmalloc(sizeof(struct pid_time_list), GFP_KERNEL);
   copy_from_user(pid_buf, user, count);
   sscanf(pid_buf, "%lu", &tmp->pid);

   spin_lock(&list_lock);
   list_add(&(tmp->list), &(pid_time_list.list));
   spin_unlock(&list_lock);   

   #ifdef DEBUG
   printk("PID: %s registered\n", pid_buf);
   #endif
   
   return count;
}

/* Callback for timer expiration "top half" */
void update_cpu_times(unsigned long data) 
{
   #ifdef DEBUG
   printk("Timer called\n");
   #endif
   
   cpu_use_work = (struct work_struct *)kmalloc(sizeof(struct work_struct), GFP_KERNEL);
   INIT_WORK(cpu_use_work, cpu_use_wq_function);
   queue_work(cpu_use_wq, cpu_use_work);

   mod_timer(&cpu_timer, jiffies + msecs_to_jiffies(5000));
}

/* Callback for the work function to process cpu usage */
void cpu_use_wq_function(struct work_struct *work)
{
   #ifdef DEBUG
   printk("Work item performed\n");
   #endif

   //spin_lock(&list_lock);

   list_for_each(head, &pid_time_list.list) {
      tmp = list_entry(head, struct pid_time_list, list);
      get_cpu_use(tmp->pid, &tmp->cpu_time);
      printk("%lu\n", tmp->cpu_time);
   }

   //spin_unlock(&list_lock);

   kfree((void *)work);
   return;
}
/* Called when module is loaded */
int __init mp1_init(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE LOADING\n");
   #endif

   create_mp1_proc_files();

   INIT_LIST_HEAD(&pid_time_list.list);   

   setup_timer(&cpu_timer, update_cpu_times, 0 );
   mod_timer(&cpu_timer, jiffies + msecs_to_jiffies(5000));

   cpu_use_wq = create_workqueue("cpu_use_queue");

   spin_lock_init(&list_lock);

   printk(KERN_ALERT "MP1 MODULE LOADED\n");
   return 0;   
}

/* Called when module is unloaded */
void __exit mp1_exit(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE UNLOADING\n");
   #endif
   // Insert your code here ...

   // Cleans up the file entries in /proc and the data structures
   delete_mp1_proc_files();
   delete_pid_time_list();
   del_timer(&cpu_timer);

   flush_workqueue(cpu_use_wq);
   destroy_workqueue(cpu_use_wq);

   printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}


// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
