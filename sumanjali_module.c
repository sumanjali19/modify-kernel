// sumanjali_module.c — CHARACTER DRIVER THAT PRINTS EXACTLY 10 MESSAGES PER OPEN
// ==========================================================================
// ALL-CAPS COMMENTS EXPLAIN WHAT I CHANGED TO MEET TASK 3:
//
// (1) I USE THE FILE POSITION (*OFFSET) TO ENFORCE "EXACTLY 10 FULL MESSAGES PER OPEN":
//     THE VIRTUAL CONTENT = MESSAGE REPEATED 10 TIMES; WHEN *OFFSET REACHES 10*LEN, RETURN EOF.
// (2) THIS HANDLES PARTIAL READS CORRECTLY (NO OFF-BY-ONE IF USER BUFFERS ARE SMALL).
// (3) A NEW OPEN STARTS AT OFFSET=0, SO EACH OPEN GETS A FRESH 10 MESSAGES.
// (4) I KEEP SINGLE-OPEN POLICY (LIKE THE EXAMPLE) TO DEMO 'MODULE IN USE' FOR 2.4.
// (5) FIXED HEADERS, __user POINTERS, .owner=THIS_MODULE, GLOBAL major_num FOR UNREGISTER.
// (6) WRITE IS UNSUPPORTED BY DESIGN: LOG AND RETURN -EINVAL (USED IN 2.2/2.3).
// ==========================================================================

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME     "sumanjali_module"
#define EXAMPLE_MSG     "Hello from the sumanjali_module!\n"
#define MSG_BUFFER_LEN  64

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ORIGINAL: ROBERT W. OLIVER II — MODIFIED BY SUMANJALI");
MODULE_DESCRIPTION("CHAR DRIVER BY SUMANJALI: EXACTLY 10 MESSAGES PER OPEN, RESET ON REOPEN");
MODULE_VERSION("0.03");

static int     device_open   (struct inode *, struct file *);
static int     device_release(struct inode *, struct file *);
static ssize_t device_read   (struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write  (struct file *, const char __user *, size_t, loff_t *);

static int  major_num = -1;
static int  device_open_count = 0;
static char   msg_buffer[MSG_BUFFER_LEN];
static size_t msg_len;

static const struct file_operations file_ops = {
    .owner   = THIS_MODULE,
    .open    = device_open,
    .release = device_release,
    .read    = device_read,
    .write   = device_write,
};

static ssize_t device_read(struct file *filp, char __user *buffer, size_t len, loff_t *offset)
{
    size_t total_len = msg_len * 10;   // TEN COMPLETE MESSAGES PER OPEN
    size_t remaining, to_copy;

    if (*offset >= total_len)          // AFTER 10*LEN → EOF
        return 0;

    remaining = total_len - *offset;
    to_copy   = (len < remaining) ? len : remaining;

    
    {
        size_t copied = 0;
        while (copied < to_copy) {
            size_t pos_in_stream = *offset + copied;
            size_t pos_in_msg    = pos_in_stream % msg_len;
            size_t chunk         = msg_len - pos_in_msg;
            size_t step          = (to_copy - copied < chunk) ? (to_copy - copied) : chunk;

            if (copy_to_user(buffer + copied, msg_buffer + pos_in_msg, step))
                return -EFAULT;

            copied += step;
        }
        *offset += copied;             // ADVANCE FILE POSITION
        return copied;
    }
}

static ssize_t device_write(struct file *filp, const char __user *buffer, size_t len, loff_t *offset)
{
    printk(KERN_ALERT "sumanjali_module: WRITE NOT SUPPORTED\n");
    return -EINVAL;
}

static int device_open(struct inode *inode, struct file *file)
{
    if (device_open_count)             // SINGLE-OPEN POLICY
        return -EBUSY;

    device_open_count++;
    try_module_get(THIS_MODULE);       // MODULE REFCOUNT++
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    device_open_count--;
    module_put(THIS_MODULE);           // MODULE REFCOUNT--
    return 0;
}

static int __init sumanjali_example_init(void)
{
    strscpy(msg_buffer, EXAMPLE_MSG, MSG_BUFFER_LEN);
    msg_len = strnlen(msg_buffer, MSG_BUFFER_LEN);

    major_num = register_chrdev(0, DEVICE_NAME, &file_ops);
    if (major_num < 0) {
        printk(KERN_ALERT "sumanjali_module: COULD NOT REGISTER DEVICE: %d\n", major_num);
        return major_num;
    }
    printk(KERN_ALERT "sumanjali_module: LOADED WITH MAJOR NUMBER %d\n", major_num);
    return 0;
}

static void __exit sumanjali_example_exit(void)
{
    if (major_num >= 0)
        unregister_chrdev(major_num, DEVICE_NAME);
    printk(KERN_ALERT "sumanjali_module: UNLOADED\n");
}

module_init(sumanjali_example_init);
module_exit(sumanjali_example_exit);
