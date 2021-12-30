
#ifdef __cplusplus
class MiniDumper
{
private:
    static LPCSTR m_szAppName;

    static LONG WINAPI TopLevelFilter( struct _EXCEPTION_POINTERS *pExceptionInfo );

public:
    MiniDumper( LPCSTR szAppName );
    void SetHandler();

    void RemoveHandler();

};
#endif


#ifdef __cplusplus
extern "C"
{
#endif

void InstallDumperHandler(LPCSTR szAppName);
void UnInstallDumperHandler();

#ifdef __cplusplus
}
#endif