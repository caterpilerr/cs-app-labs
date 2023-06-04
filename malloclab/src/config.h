#ifndef __CONFIG_H_
#define __CONFIG_H_

/*
 * config.h - malloc lab configuration file
 *
 * Copyright (c) 2002, R. Bryant and D. O'Hallaron, All rights reserved.
 * May not be used, modified, or copied without permission.
 */

/*
 * This is the default path where the driver will look for the
 * default tracefiles. You can override it at runtime with the -t flag.
 */
#define TRACEDIR "/home/azureuser/Desktop/mallocLab-master/traces/"

/*
 * This is the list of default tracefiles in TRACEDIR that the driver
 * will use for testing. Modify this if you want to add or delete
 * traces from the driver's test suite. For example, if you don't want
 * your students to implement realloc, you can delete the last two
 * traces.
 */
#define DEFAULT_TRACEFILES \
  "amptjp-bal.rep",\
  "cccp-bal.rep",\
  "cp-decl-bal.rep",\
  "expr-bal.rep",\
  "coalescing-bal.rep",\
  "random-bal.rep",\
  "random2-bal.rep",\
  "binary-bal.rep",\
  "binary3-bal.rep",\
  "realloc-bal.rep",\
  "realloc2-bal.rep"

/*
 * This constant gives the estimated performance of the libc malloc
 * package using our traces on some reference system, typically the
 * same kind of system the students use. Its purpose is to cap the
 * contribution of throughput to the performance index. Once the
 * students surpass the AVG_LIBC_THRUPUT, they get no further benefit
 * to their score.  This deters students from building extremely fast,
 * but extremely stupid malloc packages.
 */
#define AVG_LIBC_THRUPUT      18718E3  /* 13869 Kops/sec */

 /* 
  * This constant determines the contributions of space utilization
  * (UTIL_WEIGHT) and throughput (1 - UTIL_WEIGHT) to the performance
  * index.  
  */
#define UTIL_WEIGHT .60