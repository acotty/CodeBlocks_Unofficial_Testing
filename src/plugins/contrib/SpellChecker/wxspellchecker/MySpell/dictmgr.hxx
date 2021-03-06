#ifndef _DICTMGR_HXX_
#define _DICTMGR_HXX_

#define MAXDICTIONARIES 100
#define MAXDICTENTRYLEN 1024

struct dictentry
{
    char * filename;
    char * lang;
    char * region;
};


class DictMgr
{

        int                 numdict;
        dictentry     *     pdentry;

    public:

        DictMgr(const char * dictpath, const char * etype);
        ~DictMgr();
        int get_list(dictentry ** ppentry);

    private:
        int  parse_file(const char * dictpath, const char * etype);

};

#endif
