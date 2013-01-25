/** \file

An early version of an application like sbopkg written in C.

\mainpage
This program allows Slackware user to query their own system about packages.
It will query for native Slackware packages and Slackbuilds provided that the Slackbuild
database has been downloaded.
By querying a given package, if it is installed, the installed version number will be
displayed along the one of the database.

To synchronize the Slackbuild database, you must become root.

Certain system parameters like Slackware version of interes are defined in the header file.

There is two type of operation, System and Display.  System operation require root privilege.
Display operation perform miscellaneous queries.

S (System)
 r rsync the Slackbuild DB
 d download a package from Slackbuild repo

D (Display)
 a all package names
 p package description
 r readme
 c changelog
 m matching package string

h help


Compile using 
\code 
gcc -g -Wall -std=gnu99 `pkg-config --cflags glib-2.0` `curl-config --cflags` \
`pkg-config --libs glib-2.0 ` `curl-config --libs` -lssl -lcrypto sbob.c -o sbob

\endcode
or better, use make command.
*/

#include "brightstar.h"
#include "bright_parse.h"

/** A simple wrapper to fopen that provides a more friendly message if
 * file cannot be open.
 * \param filename the full path of the file to open
 * \param mode like r, w
 */
FILE * file_open(const char *filename, const char *mode)
{
    FILE * fp;
    if ((fp = fopen(filename, mode))==NULL)
    {
        printf("Cannot open file %s for mode %s\n",filename, mode);
        printf("%s\n","Cannot proceed further");
        exit(1);
    }
    return fp;
}

/**Strip newline character at end of string.
 * \param s the string to strip of newline character.
 */
void chomp(char *s)
{
    s[strcspn ( s, "\n" )] = '\0';
}

/**Calculate md5sum of filename.
 *  \param md5 Store the md5 to be calculated from filename.
 *  \param filename The filename to calculate the md5sum.
 */
void do_md5(char md5[33], char *filename)
{
    unsigned char c[MD5_DIGEST_LENGTH];
    FILE *inFile = file_open(filename, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];
    char md[2]={};

    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, 1024, inFile)) != 0)
        MD5_Update (&mdContext, data, bytes);
    MD5_Final (c,&mdContext);
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        sprintf(md,"%02x",c[i]);
        strncat(md5, md,2);
    }
    fclose (inFile);
}

/**Compare the value of two md5 and return 0 if they match or -1 if they don't.
 * Return -1 if length of either md5 string is not 32.
 * \param md5_1
 * \param md5_2
 */
int md5_compare(const char *md5_1, const char *md5_2)
{
    if(strlen(md5_1)!=32) return -1;
    if(strlen(md5_2)!=32) return -1;
    return (!strcmp(md5_1, md5_2)) ? 0 : -1;
}

/** Set flag to keep track of where the heck we are in the file
 * as it is being parsed.
 */
int set_section_flag(char *line, int current)
{
    if (strstr(line,"HOMEPAGE=")) return 1;
    if (strstr(line,"DOWNLOAD=")) return 2;
    if (strstr(line,"MD5SUM=")) return 3;
    if (strstr(line,"DOWNLOAD_x86_64=")) return 4;
    if (strstr(line,"MD5SUM_x86_64=")) return 5;
    if (strstr(line,"REQUIRES=")) return 6;
    if (strstr(line,"MAINTAINER=")) return 7;
    if (strstr(line,"EMAIL=")) return 8;
    return current;
}

/** Search for a package name matching name.  If name is not provided,
 * print a list of all packages found in the repository.
 * \param name the string to search for in the package name
 */
int search_name(const char *name)
{
    FILE *fp;
    int linelength=80;
    char line[linelength];
    fp=file_open(SB_BUILDS_LIST, "r");
    while(fgets(line, linelength,fp))
    {
        if(strstr(line,VAR_NAME))
        {
            char *p;
            p=rindex(line,':');
            if(name!=NULL && strstr(&p[2], name))
                printf("%s",&p[2]);
            else if(name==NULL)
                printf("%s",&p[2]);
        }
    }
    fclose(fp);
    return 0;    
}

/**A utility function that puts into an array the strings of the download URL and md5sum.
 * \param a The package structure to write to.
 * \param s the string to be put into the array
 * \param section the section of the package structure where the array is placed.
 */
void split2array(package_s *a, char *s, int section)
{
    char *pvalue;
    char *t;
    char *p=s;
    int i=0;
    int j;
    char *tmp[45]={};
    t=strtok_r(p, " ", &pvalue);
    asprintf(&tmp[i++], "%s", t);
    while((t=strtok_r(NULL, " ", &pvalue)))
        asprintf(&tmp[i++], "%s", t);
    for(j=0; j<=i; j++)
    {
        if(section==LINE_DOWNLOAD)
        {
            asprintf(&a->download[j], "%s", tmp[j]);
            a->download_count=j;
        }
        else if(section==LINE_DOWNLOAD64) 
        {
            asprintf(&a->download_64[j], "%s", tmp[j]);
            a->download_64_count=j;
        }
        else if(section==LINE_MD5SUM)
        {
            asprintf(&a->md5sum[j], "%s", tmp[j]);
            a->md5sum_count=j;
        }
        else if(section==LINE_MD5SUM64)
        {
            asprintf(&a->md5sum_64[j], "%s", tmp[j]);
            a->md5sum_64_count=j;
        }
        free(tmp[j]);
    }
}

slackware_s describe_slack(const char *name){
    FILE *fp;
    slackware_s slack_s = {};
    char line[MAXLEN];
    int found=0;
    fp=file_open(SK_LIST_PATH, "r");
    while(fgets(line, sizeof(line), fp)){
        char *p=line;
        char *pvalue;
        char *tmp_repo;
        char *tmp_name;
        tmp_repo=strtok_r(p, " ", &pvalue);
        tmp_name=strtok_r(NULL, " ", &pvalue);
        if(!strcmp(tmp_repo,"slackware") && !strcmp(tmp_name, name)){
            found=1;
            strcpy(slack_s.repo, tmp_repo);
            strcpy(slack_s.name,tmp_name);
            strcpy(slack_s.version, strtok_r(NULL, " ", &pvalue));
            strcpy(slack_s.arch, strtok_r(NULL, " ", &pvalue));
            strcpy(slack_s.release, strtok_r(NULL, " ", &pvalue));
            strcpy(slack_s.fullname, strtok_r(NULL, " ", &pvalue));
            strcpy(slack_s.location, strtok_r(NULL, " ", &pvalue));
            strcpy(slack_s.extension, strtok_r(NULL, " ", &pvalue));
            chomp(slack_s.extension);
            break;
        }
        else if(!strcmp(tmp_repo,"patches") && !strcmp(tmp_name, name)){
            strcpy(slack_s.patch, strtok_r(NULL, " ", &pvalue));
        }
    }
    fclose(fp);
    if(found==1){ //We have a package name, lets continue.
        found=0;
        int found_descr=0;
        int descr_line=0;
        fp=file_open(SK_PACKAGES, "r");
        while(fgets(line, sizeof(line), fp)){
            char *p=line;
            char *pvalue;
            char *t;
            char needle[300]={};
            strcpy(needle, slack_s.fullname);
            strcat(strcat(needle, "."), slack_s.extension);
            t=strtok_r(p, ":", &pvalue);
            chomp(pvalue);
            g_strchug(pvalue);
            if(!strcmp(t, "PACKAGE NAME") && strcasestr(pvalue, slack_s.fullname)){
                found=1;
                continue;
            }
            if(found==1){
                //if(!strcmp(t, PKG_LOCATION))
                //    strcpy(slack_s.location, pvalue);
                if(!strcmp(t, PKG_SIZEC))
                    strcpy(slack_s.sizec, pvalue);
                if(!strcmp(t, PKG_SIZEU))
                    strcpy(slack_s.sizeu, pvalue);
                if(!strcmp(t, PKG_DESCRIPTION)){
                    found_descr=1;
                    continue;
                }
                if(found_descr){
                    asprintf(&slack_s.descr[descr_line++], "%s", pvalue);
                    slack_s.descr_count=descr_line;
                }
                if(t[0]=='\n'){//We are done with description, get out.
                    break;
                }
            }
        }
        fclose(fp);
    }
    return slack_s;
}

/**For the package searched, extract name, location, files
 * version and short description from the SLACKBUILDS.TXT file
 * \param *name The name of the package to describe.
 */
package_s describe_package(const char *name)
{
    FILE *fp;
    package_s p_s={};
    char line[MAXLEN];
    int thisline=1;
    int flag=0;
    fp=file_open(SB_BUILDS_LIST, "r");
    while(fgets(line, sizeof(line),fp)){
        char *p=line;
        char *pvalue;
        char *t;
        t=strtok_r(p, ":", &pvalue);
        chomp(pvalue);
        g_strchug(pvalue);
        if(thisline==1)
        {
            if(!strcmp(t, "SLACKBUILD NAME") && !strcasecmp(name, pvalue))
            {
                strcpy(p_s.name, pvalue);
                flag=1;
            }
        }
        else if (flag==1)
        {
            if(!strcmp(t, "SLACKBUILD LOCATION")) 
                strcpy(p_s.location, pvalue);
            if(!strcmp(t, "SLACKBUILD FILES")) 
                strcpy(p_s.files, pvalue);
            if(!strcmp(t, "SLACKBUILD VERSION")) 
                strcpy(p_s.version, pvalue);
            if(!strcmp(t, "SLACKBUILD DOWNLOAD")) 
                split2array(&p_s, pvalue, LINE_DOWNLOAD);
            if(!strcmp(t, "SLACKBUILD MD5SUM")) 
                split2array(&p_s, pvalue, LINE_MD5SUM);
            if(!strcmp(t, "SLACKBUILD DOWNLOAD_x86_64")) 
                split2array(&p_s, pvalue, LINE_DOWNLOAD64);
            if(!strcmp(t, "SLACKBUILD MD5SUM_x86_64")) 
                split2array(&p_s, pvalue, LINE_MD5SUM64);
            if(!strcmp(t, "SLACKBUILD SHORT DESCRIPTION")) 
                strcpy(p_s.shortdescr, pvalue);
        }
        if(flag==1 && !strcmp(t,"\n")) goto finish; //No need to go further
        thisline++;
        if(thisline==11)thisline=1;
    }
finish:
    fclose(fp);
    return p_s;
}

/**Get the homepage, requires, maintainer and email as described in the 
 * packagename.info file.
 */
void get_package_info(package_s *pkg)
{
    char *location=NULL;
    location=g_strconcat(SB_REPODIR, pkg->location+2, "/",pkg->name, ".info",  NULL);
    FILE *fp;
    char line[MAXLEN];
    fp=file_open(location, "r");
    g_free(location);
    while(fgets(line, MAXLEN, fp))
    {
        section=set_section_flag(line,section);
        if(section==HOMEPAGE)
        {
            if(sscanf(line,"HOMEPAGE=\"%[^\"]",pkg->homepage)!=1)
                pkg->homepage[0]='\0';
        }
        else if(section==REQUIRES)
        {
            if(sscanf(line,"REQUIRES=\"%[^\"]",pkg->requires)!=1)
                pkg->requires[0]='\0';
        }
        else if(section==MAINTAINER)
        {
            if(sscanf(line,"MAINTAINER=\"%[^\"]",pkg->maintainer)!=1)
                pkg->maintainer[0]='\0';
        }
        else if(section==EMAIL)
        {
            if(sscanf(line,"EMAIL=\"%[^\"]",pkg->email)!=1)
                pkg->email[0]='\0';
        }
 
    }
    fclose(fp);
}

void free_spkg(slackware_s *pkg){
    int j=0;
    while(j<=pkg->descr_count){
        free(pkg->descr[j]);
        j++;
    }
}

/**Clean up the memory taken up by the download, download_64, md5sum,
 * md5sum_64, and longdescr arrays.
 */
void free_pkg(package_s *pkg)
{
    int j=0;
    while(j<=pkg->download_count)
    {
        free(pkg->download[j]);
        free(pkg->md5sum[j]);
        j++;
    }
    j=0;
    while(j<=pkg->download_64_count)
    {
        free(pkg->download_64[j]);
        free(pkg->md5sum_64[j]);
        j++;
    }
    j=0;
    while(j<pkg->longdescr_count)
        free(pkg->longdescr[j++]);
}

/**Parse the package pointer for \c download and \c download_64 arrays and request
 * download depending if values of download_count or download_64_count are
 * greater than 0. Use \c do_md5() to verify the md5.
 */
void request_download(package_s *pkg)
{
    if(YesOrNo("Download source files")==1){
        if (pkg->download_count==0 && pkg->download_64_count==0){
            fprintf(stderr,"%s\n","No package to download.  Terminated");
            exit(EXIT_FAILURE);
        }
        if(pkg->download_count>0){
            for(int i=0; i<pkg->download_count; i++){
                char *source=rindex(pkg->download[i], '/');
                char *sourceSavePath = strdup(SAVESOURCEPATH);
                asprintf(&sourceSavePath, "%s",source);
                int isOk=do_download(pkg->download[i], sourceSavePath); 
                if(isOk==0){
                    char md5[33]={};
                    do_md5(md5, sourceSavePath);
                    if(md5_compare(md5, pkg->md5sum[i])!=0)
                        printf("%s %s\n", ">>>>MD5 failed for", pkg->download[i]);
                    else
                        printf("%s %s\n", sourceSavePath, "MD5 ok");
                }
                else
                    printf("Download of %s failed\n", pkg->download[i]);
                free(sourceSavePath);
            }
        }
        else if(pkg->download_64_count>0){
            for(int i=0; i<pkg->download_64_count; i++) {
                char *source=rindex(pkg->download_64[i], '/');
                char *sourceSavePath = strdup(SAVESOURCEPATH);
                asprintf(&sourceSavePath, "%s",source);
                int isOk=do_download(pkg->download_64[i], sourceSavePath); 
                if(isOk==0){
                    char md5[33]={};
                    do_md5(md5, sourceSavePath);
                    if(md5_compare(md5, pkg->md5sum_64[i])!=0)
                        printf("%s %s\n", ">>>>MD5 failed for", pkg->download_64[i]);
                    else
                        printf("%s %s\n", sourceSavePath, "MD5 ok");
                }
                else
                    printf("Download of %s failed\n", pkg->download_64[i]);
                free(sourceSavePath);
            }
        }
    }
    if(YesOrNo("Download Slackbuild")==1){
        char *url_build = NULL;
        char *url_pgp = NULL;
        char *save_build = NULL;
        char *save_pgp = NULL;
        url_build=g_strconcat(SB_REPONET, pkg->location+2, ".tar.gz", NULL );
        url_pgp=g_strconcat(url_build, ".asc", NULL );
        save_build=g_strconcat(SAVESOURCEPATH, pkg->name, ".tar.gz", NULL);
        save_pgp=g_strconcat(save_build, ".asc", NULL);
        do_download(url_build, save_build);
        do_download(url_pgp, save_pgp);
        execl("/usr/bin/gpg", "/usr/bin/gpg", "--verify", save_pgp, NULL);
        free(url_build);
        free(url_pgp);
        free(save_build);
        free(save_pgp);
    }
}

/**Used by request download to download from \c url and save the source file at \c saveto.
 * \param *url The url to download
 * \param *saveto The full path to locally save the url downloaded.
 */
int do_download(char *url, char *saveto)
{
    int isOk=-1;
    CURL *easyhandle = curl_easy_init();
    if(easyhandle!=NULL)
    {
        curl_easy_setopt(easyhandle, CURLOPT_URL, url);
        FILE *fp=file_open(saveto, "w");
        curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(easyhandle, CURLOPT_FOLLOWLOCATION, 1);//For sites like downloads.sourceforge
        //curl_easy_setopt(easyhandle, CURLOPT_VERBOSE, 1);
        printf("Downloading...%s\n", url);
        isOk=curl_easy_perform(easyhandle);
        fclose(fp);
    }
    curl_easy_cleanup(easyhandle);
    curl_global_cleanup();//To make valgrind happier
    return isOk;
}

/**Print to sdtout the content of standard Slackware package information based
 * on structure slackware_s.
 * param spkg
 */
void print_spkg_info(slackware_s spkg){
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    printf( "\n%s\n","====Slackware package information details====");
    if(spkg.name[0]=='\0'){
        printf("\n%s\n","No Slackare package exist");
        return;
    }
    printf("Repo:              %s\n", spkg.repo);
    printf("name:              %s\n", spkg.name);
    printf("version:           %s\n", spkg.version);
    if(spkg.patch[0]!='\0')
        printf("patch:             %s\n", spkg.patch);
    printf("architecture:      %s\n", spkg.arch);
    printf("release No:        %s\n", spkg.release);
    printf("fullname:          %s\n", spkg.fullname);
    printf("location:          %s\n", spkg.location);
    printf("extension:         %s\n", spkg.extension);
    printf("size compressed:   %s\n", spkg.sizec);
    printf("size uncompressed: %s\n", spkg.sizeu);
    putchar('\n');
    for (int i=0; i<spkg.descr_count; i++)
        printf("%s\n", spkg.descr[i]);
}

/**Print to stdout the content of structure package_s pkg, the Slackbuild
 * package details
 * \param pkg
 */
void print_package_info(package_s pkg)
{
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    printf ("%s\n","====Slackbuild package information details====");
    printf ("Package        :%s\n", pkg.name);
    printf ("Version        :%s ", pkg.version);
    if(pkg.version_installed[0]!='\0')
        printf ("  %s %s", "Installed Version: ", pkg.version_installed);
    putchar('\n');
    printf ("Short Descr    :%s\n", pkg.shortdescr);
    printf ("Home page      :%s\n", pkg.homepage);
    printf ("Maintainer     :%s <%s> \n", pkg.maintainer, pkg.email);
    printf ("Location       :%s\n", pkg.location);
    int i=0;
    int j=0;
    int c;
    printf ("Files          :");
    while ((c=pkg.files[i++])!='\0')
    {
        putchar(c);
        if(j++ >= w.ws_col-39 && c==' ')
        {
            j=0;
            printf("\n%s","                ");
        }
    }
    putchar('\n');
    printf ("Requires       :%s\n", pkg.requires);
    if(pkg.download_count>0)
    {
        int j=0;
        printf("32 bits download %d file%s\n", pkg.download_count, pkg.download_count>1? "s":"");
        while(j<pkg.download_count)
        {
            printf("%s %s\n",pkg.md5sum[j], pkg.download[j]);
            j++;
        }
    }
    if(pkg.download_64_count>0)
    {
        int j=0;
        printf("64 bits download %d file%s\n", pkg.download_64_count, pkg.download_64_count>1? "s":"");
        while(j<pkg.download_64_count)
        {
            printf("%s %s\n", pkg.md5sum_64[j], pkg.download_64[j]);
            j++;
        }
    }
    if(pkg.longdescr_count>0)
    {
        int j=0;
        while(j<pkg.longdescr_count)
            printf("%s", pkg.longdescr[j++]);
    }
}

/**Use rsync to download the local repository with Slackbuilds repository.  Must be run as root.
 * This function uses the \c RSYNC_URL and \c SB_REPODIR as define in \c sbob.h.
 */
int synchronize(void)
{
    if(getuid())
    {
        fprintf(stderr,"%s\n", "Become root to rsync");
        return 1;
    }
    if(execl(RSYNC, RSYNC, "-rvz","--delete", RSYNC_URL, SB_REPODIR, NULL)==-1)
    {
        fprintf(stderr, "Cannot run rsync: %s", strerror(errno));
        return 1;
    }
    return 0;
}

/**Display to stdout overall help features
 * */
void display_help_op(void){
#define pr(s) (printf("%s\n",s))
    pr("-S --system Execute an operation that will affect the system by writing to it.");
    pr("            You must be root to execute a system operation");
    putchar('\n');
    pr("-D --display Display to stdout information about slackware package.");
    putchar('\n');
    putchar('\n');

    pr("Default system configutation values");
    pr("Where to rsync from.  With slackware version.....:"RSYNC_URL);
    pr("The params to pass on to rsync...................:"RSYNC_ARGS );
    pr("The path of rsync................................:"RSYNC );
    pr("The local directory of Slackbuild files..........:"SB_REPODIR );
    pr("The remote location of Slackbuild files..........:"SB_REPONET );
    pr("Filename containing characteristics of a package.:"SB_TXT );
    pr("The local full path of Slackbuilds...............:"SB_BUILDS_LIST SB_REPODIR SB_TXT);
    pr("The Slackware installed packages database........:"SB_DB );
#undef pr
}

/*Display to stdout the options available to the display operation
 */
void display_help_display(void){
#define pr(s) (printf("%s\n",s))
    pr("-a --all        Display all package names from Slackbuild repo to stdout.");
    pr("-d --describe   <package name> Display complete description about package.");
    pr("-r --readme     <package name> Display readme file of package.");
    pr("-c --changelog  <package name> Display changelog file of package.");
    pr("-m --match      <string> Display package names matching string.");
#undef pr
}

/*Display to stdout the options available to the system operation.
 */
void display_help_system(void)
{
#define pr(s) (printf("%s\n",s))
    pr("-r --rsync  Synchronize your local database of slackbuilds with Slackbuild.org");
    //pr("-i --install <package name> Install package on your system.  Only one package name accepted.");
    //pr("-u --uninstall <package name> Uninstall package name from your system.  Only one package name accepted");
    pr("-d --download <package name> Interactively download slackbuild and package tarball of package.");
    pr("              You can say yes or no to either.");
    pr("-h --help Display this menu.");
#undef pr
}

/**Get the long description as stored in the slack-desc.  
 * \param pkg
 * The slack-desc file is read from \c SB_REPODIR + package location folder.
 */
void get_longdescr(package_s *pkg)
{
    char *location=g_strconcat(SB_REPODIR, pkg->location+2, "/slack-desc",  NULL);
    FILE *fp;
    char line[MAXLEN];
    fp=file_open(location, "r");
    g_free(location);
    pkg->longdescr_count=0;
    int i=0;
    while (fgets(line, MAXLEN, fp) && i++<8)
        ;
    while (fgets(line, MAXLEN, fp))
    {
        char *p=line;
        p=p+strlen(pkg->name)+1;
        if(strlen(p)>1)
             asprintf(&pkg->longdescr[pkg->longdescr_count++],p);
    }
    fclose(fp);
}

/**Print to stdout the content of README file for package pkg->name
 * \param *pkg)
 */
void display_readme(package_s *pkg)
{
    char *location=g_strconcat(SB_REPODIR, pkg->location+2, "/README",  NULL);
    FILE *fp;
    char line[MAXLEN];
    fp=file_open(location, "r");
    g_free(location);
    while (fgets(line, MAXLEN, fp))
        printf("%s", line);
    fclose(fp);
}
/**Print to stdout the content of Display the changelog file of package.
 * \param *pkg */
void display_changelog(package_s *pkg)
{
    char *location=g_strconcat(SB_REPODIR, pkg->location+2, "/config/changelog",  NULL);
    FILE *fp;
    char line[MAXLEN];
    fp=file_open(location, "r");
    g_free(location);
    while (fgets(line, MAXLEN, fp))
        printf("%s", line);
    fclose(fp);
}

/**Search SB_DB for pkg->name and retreive its installed version.
 * Set the value of pkg->version_installed
 */
void get_installed_version(package_s *pkg)
{
    struct dirent **namelist;
    int n;
    int isMatch=0;//No match yet
    n = scandir(SB_DB, &namelist, 0, NULL);
    if (n < 0)
        perror("scandir");
    else {
        while(n--) {
            if(isMatch==0){
                char *s;
                char *trash;
                s=strtok_r(namelist[n]->d_name, "-", &trash);
                if(!strcmp(s, pkg->name)){
                    isMatch=1;
                    if(isalpha(*trash)!=0){
                        isMatch=0;
                        free(namelist[n]);
                        continue;
                    }
                    s=strtok_r(NULL,"-", &trash);
                    strncpy(pkg->version_installed, s, 10);
                }
                free(namelist[n]);
            }
            else//Answer found, but need to clear memory...
                free(namelist[n]);
        }
        free(namelist);
    }
}

/**For each package that is required, check if it is installed.
 * modify the pkg->requires strig with expression "installed" or
 * "not installed" for each required package.
 */
void emphasize_requires(package_s *pkg)
{
    wordexp_t p;
    char **w;
    char new_requires[300]={};
    wordexp(pkg->requires, &p, 0);
    w=p.we_wordv;
    char *n;
    n=new_requires;
    char *t;
    for(int i=0; i< p.we_wordc; i++)
    {
        asprintf(&t, "%s (%s) ",w[i], is_package_installed(w[i])? "installed": "Not installed");
        strcat(n,t);    
    }
    strcpy(pkg->requires, n);
    wordfree(&p);
}

/** Check if package_name is installed by checking in SB_DB
 * \param package_name
 * \return 1 if installed, 0 if not installed
 */
int is_package_installed(char *package_name)
{
    struct dirent **namelist;
    int n;
    int isMatch=0;//No match yet
    n = scandir(SB_DB, &namelist, 0, NULL);
    if (n < 0)
        perror("scandir");
    else {
        while(n--) {
            if(isMatch==0){
                char *s;
                char *trash;
                s=strtok_r(namelist[n]->d_name, "-", &trash);
                if(!strcmp(s, package_name)){
                    isMatch=1;
                    if(isalpha(*trash)!=0){
                        isMatch=0;
                        free(namelist[n]);
                        continue;
                    }
                }
                free(namelist[n]);
            }
            else//Answer found, but need to clear memory...
                free(namelist[n]);
        }
        free(namelist);
    }
    return isMatch;
}

/**Ask question to user and return 1 for y or Y, 0 for n or N
 * \param *question  The question to ask.
 * \return 1 for y or Y, 0 for n or N
 */
int YesOrNo(const char *question)
{
    int answer;
    char c;
    printf("%s\? ", question);
    while((c=getchar())!='y' && c!='Y' && c!='n' && c!='N')
        ;
    switch (c)
    {
        case 'y':
        case 'Y':
            answer=1;
            break;
        default:
            answer=0;
            break;
    }
    return answer; 
}

int init_parse(int argc, char *argv[])
{
    //TODO Filter argv[2]
    int ret;
    config=init_config();
    package_s pkg;
    slackware_s spkg;
    ret=parse_args(argc, argv);
    switch (config->op)
    {
        case OP_MAIN:
            display_help_op();
            break;
        case OP_SYSTEM:
            if(config->op_s_sync){
                synchronize();
            }
            else if(config->op_s_help){
                display_help_system();
            }
            else if(config->op_s_download){
                pkg=describe_package(argv[2]);
                if(pkg.name[0]=='\0'){
                    printf("%s %s\n","Nothing found for", argv[2]);
                    return 0;
                }
                request_download(&pkg);
                free_pkg(&pkg);
            }
            break;
        case OP_DISPLAY://TODO need to look at single versus combined options
            if(config->op_d_descpkg){
                spkg=describe_slack(argv[2]);
                pkg=describe_package(argv[2]);
                if(pkg.name[0]=='\0'){
                    strcpy(pkg.name, argv[2]);
                    printf("%s %s\n","No Slackbuilds found for",argv[2]);
                    get_installed_version(&pkg);
                    if(pkg.version_installed)
                        printf("Found Slackware installed version %s\n", pkg.version_installed);
                    //return 0;
                }else{
                    get_package_info(&pkg);
                    get_longdescr(&pkg);
                    get_installed_version(&pkg);
                    if(pkg.requires[0]!='\0')
                        emphasize_requires(&pkg);
                    print_package_info(pkg);
                }
                print_spkg_info(spkg);
                free_spkg(&spkg);
                free_pkg(&pkg);
            }
            else if (config->op_d_match_name && argv[2]){
                search_name(argv[2]);
            }
            else if (config->op_d_all_pkgname && argv[2]==NULL){
                search_name(NULL);
            }
            else if (config->op_d_readme){
                pkg=describe_package(argv[2]);
                if(pkg.name[0]=='\0'){
                    printf("%s %s\n","Nothing found for",argv[2]);
                    return 0;
                }
                display_readme(&pkg);
                free_pkg(&pkg);
            }
            else if (config->op_d_changelog){
                pkg=describe_package(argv[2]);
                if(pkg.name[0]=='\0'){
                    printf("%s %s\n","Nothing found for",argv[2]);
                    return 0;
                }
                display_changelog(&pkg);
                free_pkg(&pkg);
            }
            else if(config->op_d_help)
                display_help_display();
            else
                printf("<%s> %s\n",argv[1], "not a display option");
            break;
        default:
            printf("%s\n","No valid operation specified");
            ret=EXIT_FAILURE;
            break;
    }
    if(config){
        free(config);
        config=NULL;
        ret=0;
    }
    return ret;
}

/** Just calls init_parse()
 * \param argc
 * \param argv[]
 */
int main(int argc, char *argv[])
{
    init_parse(argc, argv);
}
