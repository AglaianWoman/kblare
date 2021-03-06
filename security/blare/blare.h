/*
 * Blare Linux Security Module
 *
 * Authors: Christophe Hauser <christophe@cs.ucsb.edu>
 *          Guillaume Brogi <guillaume.brogi@akheros.com>
 *          Laurent Georget <laurent.georget@supelec.fr>
 *
 * Copyright (C) 2010-2016 CentraleSupelec
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 */
#ifndef _BLARE_H
#define _BLARE_H

#include <linux/xattr.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/bitops.h>

struct msg_msg;

#define BLARE_XATTR_TAG_SUFFIX "blare.tag"
#define BLARE_XATTR_TAG XATTR_SECURITY_PREFIX BLARE_XATTR_TAG_SUFFIX
#define BLARE_XATTR_TAG_LEN (sizeof(BLARE_XATTR_TAG) - 1)

#define BLARE_TAGS_NUMBER CONFIG_SECURITY_BLARE_TAGS_SIZE

#define BLARE_FILE_TYPE 0
#define BLARE_MM_TYPE 1
#define BLARE_MSG_TYPE 2

/* defined in securityfs */
extern int blare_enabled;

/**
 * struct info_tags - Information tags (itags) of a container of information
 * @tags: 	a bit-field representing the itag, bit i corresponds to tag i
 * 		(in the local endianess)
 */
struct info_tags {
	__u32 tags[BLARE_TAGS_NUMBER];
};

/**
 * struct blare_inode_sec - Security structure for the inodes
 * @info: 	Contains the information tags of the inode.
 */
struct blare_inode_sec {
	struct info_tags info;
};

/**
 * struct blare_mm_sec - Security struct for the memory spaces (mm_struct)
 * @info: 	Contains the information tags of the memory space.
 * @users: 	Stores the numbers of processes sharing the same structure.
 * 		Two processes share their blare_mm_sec structure when one is
 * 		ptracing the other.
 */
struct blare_mm_sec {
	struct info_tags info;
	atomic_t users;
};

/**
 * struct blare_msg_sec - Security structure for the messages of message queues
 * @info:	Contains the information tags of the message.
 */
struct blare_msg_sec {
	struct info_tags info;
};

/* open_flows.c */
int register_flow_file_to_mm(struct file *file, struct mm_struct *mm);
int register_flow_mm_to_file(struct mm_struct *mm, struct file *file);
int register_flow_msg_to_mm(struct msg_msg *msg, struct mm_struct *mm);
int register_new_tags_for_mm(const struct info_tags *tags, struct mm_struct *mm);
int register_read(struct file *file);
int register_write(struct file *file);
int register_msg_reception(struct msg_msg *msg);
int register_ptrace_attach(struct task_struct *tracer,
			   struct task_struct *child);
void unregister_current_flow(void);
void unregister_dying_task_flow(struct task_struct *p);
void unregister_ptrace(struct task_struct *child);
struct blare_mm_sec *dup_msec(struct blare_mm_sec *old_msec);
void msec_get(struct blare_mm_sec *msec);
void msec_put(struct blare_mm_sec *msec);

/**
 * tags_count() - Counts the number of tags in an info_tags structure.
 * @tags:	The itags to count.
 *
 * Return: The number of tags in the set of itags represented by tags.
 */
static inline int tags_count(const struct info_tags *tags) {
	int count = 0;
	int i;
	for (i=0 ; i<BLARE_TAGS_NUMBER ; i++)
		count += hweight32(tags->tags[i]);
	return count;
}

/**
 * initialize_tags() - Zeroes an info_tags structure.
 * @tags:	The itags to initialize.
 */
static inline void initialize_tags(struct info_tags *tags) {
	int i;
	for (i=0 ; i<BLARE_TAGS_NUMBER ; i++)
		tags->tags[i] = 0;
}

/**
 * copy_tags() - Copies itags from one info_tags structure to another.
 * @dest:	The destination info_tags structure.
 * @src:	The source info_tags structure.
 *
 * The destination structure is erased and replaced by the content of the
 * source. This functions is not typically useful for tags propagation but
 * rather for initialization.
 */
static inline void copy_tags(struct info_tags *dest, const struct info_tags *src) {
	int i;
	for (i=0 ; i<BLARE_TAGS_NUMBER ; i++)
		dest->tags[i] = src->tags[i];
}

/* securityfs.c */
int blare_init_fs(void);
bool blare_is_traced(const struct info_tags* tags_added);
int blare_trace_all(const struct info_tags* tags_added, void* src, int src_type,
		void* dest, int dest_type);
int blare_tags_to_string(const __u32 *tag, char** buffer);
int blare_tags_from_string(const char* buf, size_t length, __u32 *tag);

/**
 * blare_stop_propagate() - Checks whether an info_tags is marked as preventing
 * tag propagation.
 * @tags:	The info_tags structure.
 *
 * Tag 0 has a special semantics and stops the tag propagation when set. It
 * itself doesn't propagate.
 *
 * Returns: True if, and only if, no tags can be propagated to or from the
 * info_tags structure.
 */
static inline bool blare_stop_propagate(const struct info_tags* tags) {
	return (tags->tags[0] & 1);
}
#endif // _BLARE_H
