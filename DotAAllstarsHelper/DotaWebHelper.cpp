#include "Main.h"

#include <WinInet.h>
#pragma comment(lib,"Wininet.lib")
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")





char szHeaders[ ] = "Content-Type: application/x-www-form-urlencoded\r\nCache-Control: no-cache, no-store, must-revalidate\r\nPragma: no-cache\r\nExpires: 0\r\n";


int DownStatus = 0;
unsigned int DownProgress = 0;
std::string LatestDownloadedString;

std::string DownloadBytesGet( char* szUrl, char * getRequest )
{
	AddNewLineToDotaHelperLog( "DownloadBytesGet" );
	DownStatus = 0;
	std::string returnvalue = "";
	WSADATA wsaData;
	if ( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 )
	{
		DownStatus = -1;

		return returnvalue;
	}
	SOCKET Socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if ( !Socket )
	{
		WSACleanup( );
		DownStatus = -1;

		return returnvalue;
	}


	struct hostent *host;
	host = 0;// gethostbyname( szUrl );

	if ( !host )
	{
		closesocket( Socket );
		WSACleanup( );
		DownStatus = -1;

		return returnvalue;
	}

	SOCKADDR_IN SockAddr;
	SockAddr.sin_port = htons( 80 );
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr.s_addr = *( ( unsigned long* ) host->h_addr );
	if ( connect( Socket, ( SOCKADDR* ) ( &SockAddr ), sizeof( SockAddr ) ) != 0 )
	{
		closesocket( Socket );
		WSACleanup( );
		DownStatus = -1;

		return returnvalue;
	}

	char sendbuffer[ 512 ];
	sprintf_s( sendbuffer, 512, "%s%s%s%s\r\nConnection: close\r\n\r\n", "GET ", getRequest, " HTTP/1.1\r\nHost: ", szUrl );

	DownProgress = 20;

	if ( send( Socket, sendbuffer, (int) strlen( sendbuffer ), 0 ) == SOCKET_ERROR )
	{
		closesocket( Socket );
		WSACleanup( );
		DownStatus = -1;

		return returnvalue;
	}

	DownProgress = 40;

	char buffer[ 10000 ];
	int nDataLength;
	while ( ( nDataLength = recv( Socket, buffer, 10000, 0 ) ) > 0 )
	{
		DownProgress = 60;
		returnvalue.append( buffer,(size_t) nDataLength );
	}

	DownProgress = 80;

	while ( returnvalue.size( ) )
	{
		if ( returnvalue.size( ) > 0 )
		{
			if ( returnvalue.c_str( )[ 0 ] != '\r' )
			{
				returnvalue.erase( returnvalue.begin( ) );
			}
			else if ( returnvalue.size( ) > 5 )
			{

				if ( returnvalue.c_str( )[ 1 ] == '\n'
					 && returnvalue.c_str( )[ 2 ] == '\r'
					 && returnvalue.c_str( )[ 3 ] == '\n' )
				{
					returnvalue.erase( returnvalue.begin( ) );
					returnvalue.erase( returnvalue.begin( ) );
					returnvalue.erase( returnvalue.begin( ) );
					returnvalue.erase( returnvalue.begin( ) );
					break;
				}
				returnvalue.erase( returnvalue.begin( ) );
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	DownProgress = 100;

	LatestDownloadedString = returnvalue;
	if ( returnvalue.size( ) > 0 )
		DownStatus = 1;
	else
		DownStatus = -1;
	return returnvalue;
}




void DownloadNewMapToFile( char* szUrl, char * filepath )
{
	AddNewLineToDotaHelperLog( "DownloadNewMapToFile" );
	DownStatus = 0;
	HINTERNET hOpen = NULL;
	HINTERNET hFile = NULL;
	DWORD dataSize = 0;
	DWORD dwBytesRead = 0;
	vector<unsigned char> OutData;
	FILE * outfile = NULL;
	BOOL AllOkay = FALSE;

	if ( filepath == NULL || filepath[ 0 ] == '\0' || FileExist( filepath ) )
	{
		DownStatus = 2;
		return;
	}

	hOpen = InternetOpen( "Microsoft Internet Explorer", NULL, NULL, NULL, 0 );
	if ( !hOpen )
	{
		DownStatus = -1;
		return;
	}
	DownProgress = 10;
	hFile = InternetOpenUrl( hOpen, szUrl, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE, 0 );

	if ( !hFile )
	{
		InternetCloseHandle( hOpen );
		DownStatus = -1;
		return;
	}
	DownProgress = 20;
	int code = 0;
	DWORD codeLen = 4;
	HttpQueryInfo( hFile, HTTP_QUERY_STATUS_CODE |
				   HTTP_QUERY_FLAG_NUMBER, &code, &codeLen, 0 );

	if ( code != HTTP_STATUS_OK )// 200 OK
	{
		InternetCloseHandle( hFile );
		InternetCloseHandle( hOpen );
		DownStatus = -1;
		return;
	}

	unsigned int sizeBuffer = 0;
	DWORD length = sizeof( sizeBuffer );
	HttpQueryInfo( hFile, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &sizeBuffer, &length, NULL );



	DownProgress = 30;

	do
	{
		dataSize += dwBytesRead;
		if ( sizeBuffer != 0 )
			DownProgress = ( dataSize * 100 ) / sizeBuffer;

		dwBytesRead = 0;
		unsigned char buffer[ 2000 ];
		BOOL isRead = InternetReadFile( hFile, ( LPVOID ) buffer, _countof( buffer ), &dwBytesRead );
		if ( dwBytesRead && isRead )
		{
			AllOkay = TRUE;
			for ( unsigned int i = 0; i < dwBytesRead; i++ )
				OutData.push_back( buffer[ i ] );
		}
		else
			break;
	}
	while ( dwBytesRead );

	if ( DownProgress == 30 )
	{
		DownProgress = 70;
	}

	if ( OutData.size( ) > 0 && AllOkay )
	{
		fopen_s( &outfile, filepath, "wb" );
		if ( outfile != NULL )
		{
			fwrite( &OutData[ 0 ], OutData.size( ), 1, outfile );
			OutData.clear( );
			fclose( outfile );
			DownStatus = 1;
		}
		else DownStatus = -1;
	}
	else DownStatus = -1;


	if ( DownProgress == 70 )
	{
		DownProgress = 100;
	}

	InternetCloseHandle( hFile );
	InternetCloseHandle( hOpen );

	return;
}

char * _addr = 0;
char * _request = 0;
char * _filepath = 0;

DWORD WINAPI SENDGETREQUEST( LPVOID )
{
	DownloadBytesGet( _addr, _request ).clear( );
	return 0;
}


DWORD WINAPI SENDSAVEFILEREQUEST( LPVOID )
{
	DownloadNewMapToFile( _addr, _filepath );
	return 0;
}

__declspec( dllexport ) int __stdcall SendGetRequest( char * addr, char * request )
{
	DownProgress = 0;
	_addr = addr; _request = request;
	DownStatus = 0;
	CreateThread( 0, 0, SENDGETREQUEST, 0, 0, 0 );
	return 0;
}

__declspec( dllexport ) int __stdcall SaveNewDotaVersionFromUrl( char * addr, char * filepath )
{
	DownProgress = 0;
	_addr = addr; _filepath = filepath;
	DownStatus = 0;
	CreateThread( 0, 0, SENDSAVEFILEREQUEST, 0, 0, 0 );
	return 0;
}

__declspec( dllexport ) int __stdcall GetDownloadStatus( int )
{
	return DownStatus;
}

__declspec( dllexport ) unsigned int __stdcall GetDownloadProgress( int )
{
	return DownProgress;
}

__declspec( dllexport ) const char * __stdcall GetLatestDownloadedString( int )
{
	return LatestDownloadedString.c_str( );
}