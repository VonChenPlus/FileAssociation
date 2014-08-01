#ifdef FILEASSOCIATIONMON_EXPORTS
#define FILEASSOCIATIONMON_API __declspec(dllexport)
#else
#define FILEASSOCIATIONMON_API __declspec(dllimport)
#endif