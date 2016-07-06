// mp3player.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	::CoInitialize(NULL);

	if ( argc > 1 )
	{
		CDSPlayer player;
		player.PlayFile( argv[1] );
		while( player.CurrentPos() < player.GetLength() )
		{
			_tprintf(TEXT("Current stamp: %0.2f\r\n"), player.CurrentPos());
			Sleep( 1000 );
		}
	}
	else _tprintf(TEXT("Usage: %s <mp3-file>\r\n"), argv[0]);

	::CoUninitialize();

	return 0;
}

