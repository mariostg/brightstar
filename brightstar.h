/** \file
 */
#ifndef BRIGHTSTAR_H
#define BRIGHTSTAR_H

#define _GNU_SOURCE //!< Mainly for asprintf usage which I like.
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <glib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <curl/curl.h>
#include <openssl/md5.h>
#include <dirent.h>
#include <wordexp.h>

//extern char *optarg; //!< Use by getopt.
//extern int optind; //!< Use by getopt.
//extern int optopt; //!< Use by getopt.
//extern int opterr; //!< Use by getopt.

/*
PACKAGES.TXT;  Wed Jan 16 03:02:49 UTC 2013

This file provides details on the Slackware packages found
in the ./patches/ directory.

Total size of all packages (compressed):  78 MB
Total size of all packages (uncompressed):  263 MB


PACKAGE NAME:  bind-9.9.2_P1-i486-1_slack14.0.txz
PACKAGE LOCATION:  ./patches/packages
PACKAGE SIZE (compressed):  1940 K
PACKAGE SIZE (uncompressed):  7490 K
PACKAGE DESCRIPTION:
bind: bind (DNS server and utilities)
bind:
bind: The named daemon and support utilities such as dig, host, and
bind: nslookup.  Sample configuration files for running a simple caching
bind: nameserver are included.  Documentation for advanced name server
*/

/*!< The Slackware installed packages database, used to find uout if a package 
 * is installed.  It contains original packages and third party pckages (SBo).
 */
#define SB_DB "/var/log/packages/"     
                                        
//SLACKWARE configuration section
#define PKG_NAME "PACKAGE NAME"
#define PKG_LOCATION "PACKAGE LOCATION"
#define PKG_SIZEC "PACKAGE SIZE (compressed)"
#define PKG_SIZEU "PACKAGE SIZE (uncompressed)"
#define PKG_DESCRIPTION "PACKAGE DESCRIPTION"

#define SK_DB "/var/lib/slackpkg/" //!<The DB folder of all slackware packages.
#define SK_TXT "PACKAGES.TXT"      //!<The file that contains Slackware package information.
#define SK_CHNG "ChangeLog.txt"    //!<The file that contains Slackware package changelog.
#define SK_LIST "pkglist"          //list repo, name, arch, release, version, fullname, location, extension
#define SK_LIST_PATH SK_DB SK_LIST
#define SK_PACKAGES SK_DB SK_TXT
#define SK_CHANGELOG SK_DB SK_CHNG

//SLACKBUILS configuration section
#define LINE_NAME 1       //!< Line 1 for SLACKBUILD NAME: EMBASSY
#define LINE_LOCATION 2   //!< Line 2 for SLACKBUILD LOCATION: ./academic/EMBASSY
#define LINE_FILES 3      //!< Line 3 for SLACKBUILD FILES: CONTENTS EMBASSY.SlackBuild EMBASSY.info README slack-desc
#define LINE_VERSION 4    //!< Line 4 for SLACKBUILD VERSION: 6.5.7
#define LINE_DOWNLOAD 5   //!< Line 5 for SLACKBUILD DOWNLOAD: ftp://emboss.open-bio.org/pub/EMBOSS/CBSTOOLS-1.0.0.650.tar.gz
#define LINE_DOWNLOAD64 6 //!< Line 6 for SLACKBUILD DOWNLOAD_x86_64: 
#define LINE_MD5SUM 7     //!< Line 7 for SLACKBUILD MD5SUM: 4913776ee5ff93ae839762107f8d8bc8 
#define LINE_MD5SUM64 8   //!< Line 8 for SLACKBUILD MD5SUM_x86_64: 
#define LINE_SHORTDESCR 9 //!< Line 9 for SLACKBUILD SHORT DESCRIPTION:  EMBASSY (EMBOSS associated software)

#define VAR_NAME       "SLACKBUILD NAME"                //!< Identifier to get the package name in SLACKBUILD.TXT
#define VAR_VERSION    "SLACKBUILD VERSION"             //!< Identifier to get the package version in SLACKBUILD.TXT
#define VAR_DOWNLOAD   "SLACKBUILD DOWNLOAD"            //!< Identifier to get the package source files 32 bits
#define VAR_DOWNLOAD64 "SLACKBUILD DOWNLOAD_x86_64"     //!< Identifier to get the package source files 64 bits
#define VAR_MD5SUM     "SLACKBUILD MD5SUM"              //!< Identifier to get the package md5sum source files 32 bits
#define VAR_MD5SUM64   "SLACKBUILD MD5SUM_x86_64"       //!< Identifier to get the package md5sum source files 64 bits
#define VAR_SHORTDESCR "SLACKBUILD SHORT DESCRIPTION"   //!< Identifier to get the package short description
#define VAR_LOCATION   "SLACKBUILD LOCATION"            //!< Identifier to get the package directory location
#define VAR_FILES      "SLACKBUILD FILE"                //!< Identifier to get the package slackbuild files

#define RSYNC_URL "rsync://rsync.slackbuilds.org/slackbuilds/14.0/"  //!< Where to rsync from.  With slackware version.
#define RSYNC_ARGS "-rvz --delete"                                   //!< The params to pass on to rsync
#define RSYNC "/usr/bin/rsync"                                       //!< The path of rsync
#define SB_REPODIR "/var/lib/sbopkg/SBo/14.0/"                       //!< The local directory of Slackbuild files
#define SB_REPONET "slackbuilds.org/slackbuilds/14.0/"               //!< The remote location of Slackbuild files
#define SB_TXT "SLACKBUILDS.TXT"                                     //!< What can I say... :)
#define SB_BUILDS_LIST SB_REPODIR SB_TXT                             //!< The local full path of Slackbuilds
#define SAVESOURCEPATH "/tmp/"                                       //!< Path where source files and Slackbuilds are download
#define MAXLEN 2048                                                  //!< An array size sometime usefule...

/**The elements used to describe a package from Slackware
 */
typedef struct{
    char repo[30];
    char name[80];
    char version[20];
    char arch[10];
    char release[3];
    char fullname[50];
    char location[150];
    char extension[5];
    char sizec[30];
    char sizeu[30];
    char *descr[13];
    int descr_count;
    char patch[75];
} slackware_s;

/**The elements used to describe a package as it exists in Slackbuilds
 */
typedef struct {
    char name[80];               //!< The name of the package as per SLACKBUILDS.TXT.
    char files[300];             //!< The file included in the package (README, slackbuild,etc) as per SLACKBUILDS.TXT.
    char *download[45];          //!< The url of the 32 bits source files version as per SLACKBUILDS.TXT.
    int download_count;          //!< The number of source files, 32 bits version.
    char *download_64[45];       //!< The url of the 64 bits source files version as per SLACKBUILDS.TXT.
    int download_64_count;       //!< The number of source files, 64 bits version.
    char *md5sum[45];            //!< The md5sum of the corresponding url source file 32 bits as per SLACKBUILDS.TXT.
    int md5sum_count;            //!< The number of md5sum values associated to url source file 32 bits.
    char *md5sum_64[45];         //!< The md5sum of the corresponding url source file 64 bits as per SLACKBUILDS.TXT.
    int md5sum_64_count;         //!< The number of md5sum values associated to url source file 64 bits.
    char shortdescr[900];        //!< The short description of the package as per SLACKBUILDS.TXT.
    char *longdescr[10];         //!< The long description of the package as per slack-desc file.
    int longdescr_count;         //!< The number of lines making the long description.
    char version[30];            //!< The version of the package as per SLACKBUILDS.TXT.
    char version_installed[30];  //!< The installed version of the package.
    char location[150];          //!< The directory location of the package as per SLACKBUILDS.TXT.
    char homepage[150];          //!< The website of the package as per .info file.
    char maintainer[150];        //!< The maintainer of the slackbuild as per .info file.
    char email[150];             //!< The email address of the slackbuild maintainer as per .info file.
    char requires[300];          //!< The package(s) that are dependencies to the package as per .info file
} package_s;

extern package_s *pkg;
/**The sections of the .info file
 */
enum {NONE=0, HOMEPAGE=1, DOWNLOAD=2, MD5SUM=3, DOWNLOAD_x86_64=4, 
    MD5SUM_x86_64=5, REQUIRES=6, MAINTAINER=7, EMAIL=8} section;

FILE * file_open(const char *filename, const char *mode);
void chomp(char *s);
int set_section_flag(char *line, int current);
int search_name(const char *name);
void split2array(package_s *a, char *s, int section);
package_s describe_package(const char *name);
void get_package_info(package_s *pkg);
void free_pkg(package_s *pkg);
void request_download(package_s *pkg);
int  do_download(char *url, char *saveto);
void do_md5(char md[33], char *filename);
int md5_compare(const char *md5_1, const char *md5_2);
void display_help(void);
void get_installed_version(package_s *pkg);
int YesOrNo(const char *question);
int is_package_installed(char *package_name);
int init_parse(int argc, char *argv[]);
#endif /* BRIGHTSTAR_H */
