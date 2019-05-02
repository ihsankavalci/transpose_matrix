#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define BUFSIZE  1000

MODULE_LICENSE("GPL");              ///< The license type -- this affects runtime behavior
MODULE_AUTHOR("ihsan kavalci");      ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("Transpose Matrix");  ///< The description -- see modinfo
MODULE_VERSION("1.0");              ///< The version of the module

static struct proc_dir_entry *ent;

static int rows = 4;
module_param(rows, int, S_IRUGO);

static int cols = 3;
module_param(cols, int, S_IRUGO);

int matrixSize;
int *Matrix;
int is_empty = 1;

char *strtok_r(char *s, const char *delim, char **save_ptr)
{
	char *end;
	if (s == NULL)
	s = *save_ptr;
	if (*s == '\0') {
		*save_ptr = s;
		return NULL;
	}
	s += strspn (s, delim);
	if (*s == '\0') {
		*save_ptr = s;
		return NULL;
	}
	end = s + strcspn (s, delim);
	if (*end == '\0') {
		*save_ptr = end;
		return s;
	}
	*end = '\0';
	*save_ptr = end + 1;
	return s;
}

int parse_col(char *str, int r, int *tmpMatrix) {
	int c = 0;
     char *rest;
	char *token;

	token = strtok_r(str, " ", &rest);
	while (token != 0) {
		sscanf(token, "%d", (tmpMatrix + r * cols + c++));
		token = strtok_r(rest, " ", &rest);
	}
	kfree(str);
	return c;
}

int parse_rows(const char *buf, int len) {
	int rowCount = 0, colCount = 0, count = 0;
	int *tmpMatrix;
	char *str;
	char *rest, *strRow, *strCol;
	tmpMatrix = (int *)kmalloc(matrixSize, GFP_KERNEL);

	str = kmalloc(sizeof(char) * len, GFP_KERNEL);
	memcpy(str, buf, len);

	strRow = strtok_r(str, "-", &rest);
	while (strRow != 0) {
		strCol = kmalloc(sizeof(char) * strlen(strRow) + 1, GFP_KERNEL);
		memcpy(strCol, strRow, strlen(strRow) + 1);
          colCount = parse_col(strCol, rowCount++, tmpMatrix);
          if (colCount != cols) {
               count = -1;
               break;
          }
		count += colCount;
		strRow = strtok_r(rest, "-", &rest);
	}
	kfree(str);
	if (count == rows * cols) {
		memcpy(Matrix, tmpMatrix, matrixSize);
	}
	kfree(tmpMatrix);
	return count;
}

int *get_transpose(void) {
     int i, j;
	int *transpose;
	transpose = (int *)kmalloc(matrixSize, GFP_KERNEL);

	for (i = 0; i < rows; i++) {
		for (j = 0; j < cols; j++) {
			*(transpose + j * rows + i) = *(Matrix + i * cols + j);
		}
	}
	return transpose;
}

char *get_transpose_str(void) {
     int i, j;
	int len = 0, size, *transpose;
	char *buf;
	size = sizeof(char) * BUFSIZE;
	buf = kmalloc(size, GFP_KERNEL);
	transpose = get_transpose();
	for (i = 0; i < cols; i++) {
		for (j = 0; j < rows; j++) {
			len += sprintf(buf + len, "%5d ", *(transpose + i * rows + j));
		}
		len += sprintf(buf + len, "\n");
	}
	kfree(transpose);
	return buf;
}

void printk_transpose(void) {
	char *str;
	str = get_transpose_str();
	printk(KERN_INFO "Transpose of matrix:\n%s", str);
	kfree(str);
}

void printk_matrix(void) {
	int i, j;
	printk(KERN_INFO "Matrix:\n");
	for (i = 0; i < rows; i++) {
		for (j = 0; j < cols; j++) {
			printk(KERN_INFO "%d ", *(Matrix + i * cols + j));
		}
		printk(KERN_INFO " \n");
	}
}

static ssize_t mywrite(struct file *file, const char __user *ubuf,size_t count, loff_t *ppos) {
	int c, l;
	char buf[BUFSIZE];

	if(*ppos > 0 || count > BUFSIZE)
		return -EFAULT;
	if(copy_from_user(buf, ubuf, count))
		return -EFAULT;

	l = strlen(buf);
     c = parse_rows(buf, l);

     if (c != rows * cols) {
          printk(KERN_INFO "Matrix data is invalid\n");
          *ppos = l;
     	return l;
     }
     printk_matrix();
     printk_transpose();
     is_empty = 0;
	*ppos = l;
	return l;
}

static ssize_t myread(struct file *file, char __user *ubuf,size_t count, loff_t *ppos){
	char *buf;
	int len = 0;
	if(*ppos > 0 || count < BUFSIZE)
		return 0;
     if (!is_empty) {
          buf = get_transpose_str();
     	len = strlen(buf);
     	if(copy_to_user(ubuf,buf,len))
     		return -EFAULT;
     	kfree(buf);
     	*ppos = len;
     	return len;
     }
     else {
          return 0;
     }
}

static struct file_operations myops = {
     .owner = THIS_MODULE,
     .read = myread,
     .write = mywrite,
};

static int __init transpose_init(void){
     matrixSize = rows * cols * sizeof(int);
     Matrix = (int *) kmalloc(matrixSize, GFP_KERNEL);
     ent = proc_create("transpose", 0660, NULL, &myops);
     printk(KERN_INFO "transpose_init Rows = %d, Cols = %d\n", rows, cols);
     return 0;
}

static void __exit transpose_exit(void){
     printk(KERN_INFO "transpose_exit, Bye\n");
	proc_remove(ent);
}

module_init(transpose_init);
module_exit(transpose_exit);
