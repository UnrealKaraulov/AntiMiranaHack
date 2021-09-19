

#pragma region Headers
#define _WIN32_WINNT 0x0501 
#define WINVER 0x0501 
#define NTDDI_VERSION 0x05010000

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <string>
#include <time.h>
#pragma endregion

// Game.dll address
int GameDll = 0;


// Check if button exists
typedef void *( __fastcall * GetBtnAddrGAME )( char * a2 , int  INDEX );
GetBtnAddrGAME GetBtnAddr_p;


typedef BOOL( __cdecl * pIsTerrainPathable )( float *x , float *y , int HPATHINGTYPEt );
pIsTerrainPathable IsTerrainPathableX;

BOOL SkipTerrainCheck = FALSE;

BOOL __cdecl IsTerrainPathable( float x , float y , int HPATHINGTYPEt )
{
	if ( SkipTerrainCheck )
		return TRUE;
	return IsTerrainPathableX( &x , &y , HPATHINGTYPEt );
}

bool IsLagScreen( )
{
	// Если кнопка "SuspendDropPlayersButton" существует - лагскрин активен
	return GetBtnAddr_p( "SuspendDropPlayersButton" , 0 ) > 0;
}

bool IsGame( ) // my offset + public
{
	return ( *( int* ) ( ( UINT32 ) GameDll + 0xACF678 ) > 0 || *( int* ) ( ( UINT32 ) GameDll + 0xAB62A4 ) > 0 )/* && !IsLagScreen( )*/;
}

// get thread access (for Jass natives / other functions)
void SetTlsForMe( )
{
	UINT32 Data = *( UINT32 * ) ( GameDll + 0xACEB4C );
	UINT32 TlsIndex = *( UINT32 * ) ( GameDll + 0xAB7BF4 );
	if ( TlsIndex )
	{
		UINT32 v5 = **( UINT32 ** ) ( *( UINT32 * ) ( *( UINT32 * ) ( GameDll + 0xACEB5C ) + 4 * Data ) + 44 );
		if ( !v5 || !( *( LPVOID * ) ( v5 + 520 ) ) )
		{
			Sleep( 1000 );
			SetTlsForMe( );
			return;
		}
		TlsSetValue( TlsIndex , *( LPVOID * ) ( v5 + 520 ) );
	}
	else
	{
		Sleep( 1000 );
		SetTlsForMe( );
		return;
	}
}


// Pure code: Get unit count and units array
UINT GetUnitCountAndUnitArray( int * unitarray )
{
	int GlobalClassOffset = *( int* ) ( GameDll + 0xAB4F80 );
	if ( GlobalClassOffset )
	{
		int UnitsOffset1 = *( int* ) ( GlobalClassOffset + 0x3BC );
		int UnitsCount = *( int* ) ( UnitsOffset1 + 0x604 );
		*unitarray = *( int* ) ( UnitsOffset1 + 0x608 );
		return UnitsCount;
	}
	return 0;
}


void * GetGlobalPlayerData( )
{
	if ( *( int * ) ( 0xAB65F4 + GameDll ) > 0 )
	{
		return ( void * ) *( int* ) ( 0xAB65F4 + GameDll );
	}
	else
		return nullptr;
}

int GetPlayerByNumber( int number )
{

	void * arg1 = GetGlobalPlayerData( );
	int result = -1;
	if ( arg1 != nullptr && arg1 )
	{
		result = ( int ) arg1 + ( number * 4 ) + 0x58;

		result = *( int* ) result;
		/*__asm
		{
		mov ecx , arg1;
		mov eax , number;
		mov eax , [ ecx + eax * 4 + 0x58 ];
		mov result , eax;
		}*/
	}
	return result;
}

int GetLocalPlayerNumber( )
{

	void * gldata = GetGlobalPlayerData( );
	if ( gldata != nullptr && gldata )
	{
		int playerslotaddr = ( int ) gldata + 0x28;
		return ( int ) *( short * ) ( playerslotaddr );
	}
	else
		return -2;
}


int GetLocalPlayer( )
{
	return GetPlayerByNumber( GetLocalPlayerNumber( ) );
}


UINT GetUnitOwnerSlot( int unitaddr )
{
	return *( int* ) ( unitaddr + 88 );
}

bool IsPlayerEnemy( int unitaddr )
{
	int teamplayer1 = GetLocalPlayerNumber( ) > 5 ? 1 : 2;
	int teamplayer2 = GetUnitOwnerSlot( unitaddr ) > 5 ? 1 : 2;
	return teamplayer1 != teamplayer2;
}

BOOL IsUnitDead( int unitaddr )
{
	unsigned int isdolbany = *( unsigned int* ) ( unitaddr + 0x5C );
	return ( !( ( isdolbany & 0x100u ) == 0 ) );
}


int GetSelectedOwnedUnit( )
{

	UINT32 plr = GetLocalPlayer( );
	int unitaddr; // = *(int*)((*(int*)plr+0x34)+0x1e0);

	__asm
	{
		MOV EAX , plr;
		MOV ECX , DWORD PTR DS : [ EAX + 0x34 ];
		MOV EAX , DWORD PTR DS : [ ECX + 0x1E0 ];
		MOV unitaddr , EAX;
	}


	if ( unitaddr > 0 )
	{
		if ( GetUnitOwnerSlot( unitaddr ) == GetLocalPlayerNumber( ) )
		{
			return unitaddr;
		}
	}

	return NULL;
}


bool IsClassEqual( int unitaddr , char * classid )
{
	char unitclass[ 5 ];
	memset( unitclass , 0 , 5 );
	*( BYTE* ) &unitclass[ 0 ] = *( BYTE* ) ( unitaddr + 0x30 + 3 );
	*( BYTE* ) &unitclass[ 1 ] = *( BYTE* ) ( unitaddr + 0x30 + 2 );
	*( BYTE* ) &unitclass[ 2 ] = *( BYTE* ) ( unitaddr + 0x30 + 1 );
	*( BYTE* ) &unitclass[ 3 ] = *( BYTE* ) ( unitaddr + 0x30 + 0 );
	if ( strlen( classid ) == 4 )
	{
		if ( unitclass[ 0 ] == classid[ 0 ] && unitclass[ 1 ] == classid[ 1 ] &&
			 unitclass[ 2 ] == classid[ 2 ] && unitclass[ 3 ] == classid[ 3 ] )
			 return true;
	}

	return false;
}

struct UnitLocation
{
	float X;
	float Y;
	float Z;
};

struct Location
{
	float X;
	float Y;
};


float DEGTORAD = 3.14159f / 180.0f;
float RADTODEG = 180.0f / 3.14159f;



float GetUnitFacing( int unitaddr )
{
	float ret1 , ret2;
	int _FacingConst = 0xAC5419 + GameDll;
	int _6F6EF520 = 0x6EEE20 + GameDll;
	__asm
	{
		MOV ESI , unitaddr;
		MOV EDX , DWORD PTR DS : [ ESI ];
		MOV EAX , DWORD PTR DS : [ EDX + 0xB8 ]
			MOV ECX , ESI
			CALL EAX
			MOV EDX , DWORD PTR DS : [ EAX ]
			MOV EDX , DWORD PTR DS : [ EDX + 0x1C ]
			LEA ECX , ret1
			PUSH ECX
			MOV ECX , EAX
			CALL EDX
			PUSH _FacingConst
			MOV EDX , EAX
			LEA ECX , ret2
			CALL _6F6EF520
	}
	return ret2;
}

enum MYCMD
{
	MOVE = 0xD0003 ,
	ATTACK = 0xD000F ,
	HOLD = 0xD0019 ,
	STOP = 0xD0004
};

#define ADDR(X,REG)\
	__asm MOV REG , DWORD PTR DS : [ X ] \
	__asm MOV REG , DWORD PTR DS : [ REG ]

void SendMoveAttackCommand( MYCMD cmdId , float X , float Y )
{
	int _W3XGlobalClass = GameDll + 0xAB4F80;
	int _MoveAttackCmd = GameDll + 0x339DD0;
	__asm
	{
		PUSH 0
			PUSH 6
			PUSH 0
			ADDR( _W3XGlobalClass , ECX )
			MOV ECX , DWORD PTR DS : [ ECX + 0x1B4 ];
		PUSH Y
			PUSH X
			PUSH 0
			PUSH cmdId
			CALL _MoveAttackCmd
	}

}

Location GetNextPoint( float x , float y , float distance , float angle )
{
	Location returnlocation = Location( );
	returnlocation.X = x + distance * ( float ) cos( angle * DEGTORAD );
	returnlocation.Y = y + distance * ( float ) sin( angle * DEGTORAD );
	return returnlocation;
}


typedef UnitLocation* ( __fastcall * sub_6F00C9F0 )( int unitaddr , UnitLocation * unitloc , int null1 , int null2 , int null3 , int null4 );
sub_6F00C9F0 GetUnitLocation;

float GetAngleBetweenPoints( float x1 , float y1 , float x2 , float y2 )
{
	return atan2( y2 - y1 , x2 - x1 ) * RADTODEG;
}

float Distance( float dX0 , float dY0 , float dX1 , float dY1 )
{
	return sqrt( ( dX1 - dX0 )*( dX1 - dX0 ) + ( dY1 - dY0 )*( dY1 - dY0 ) );
}

float DistanceBetweenLocs( Location loc1 , Location loc2 )
{
	return Distance( loc1.X , loc1.Y , loc2.X , loc2.Y );
}

Location GiveNextLocationFromLocAndAngle( Location startloc , float distance , float angle )
{
	return GetNextPoint( startloc.X , startloc.Y , distance , angle );
}


Location PleaseGiveMePathv2( BOOL Pathable , Location unitloc , Location arrowloc , float arrowangle )
{
	Location retloc = Location( );

	Location unittest1 = Location( );
	Location unittest11 = Location( );
	Location unittest111 = Location( );

	Location unittest2 = Location( );
	Location unittest22 = Location( );
	Location unittest222 = Location( );

	float arrow1test = 0.0f;
	float arrow2test = 0.0f;


	float angle = arrowangle - 120.0f;
	for ( ; angle < arrowangle - 60.0f; angle += 10 )
	{
		float reversedangle = angle + 180.0f;
		unittest1 = GiveNextLocationFromLocAndAngle( unitloc , 400 , angle );
		unittest11 = GiveNextLocationFromLocAndAngle( unitloc , 300 , angle );
		unittest111 = GiveNextLocationFromLocAndAngle( unitloc , 200 , angle );

		unittest2 = GiveNextLocationFromLocAndAngle( unitloc , 400 , reversedangle );
		unittest22 = GiveNextLocationFromLocAndAngle( unitloc , 300 , reversedangle );
		unittest222 = GiveNextLocationFromLocAndAngle( unitloc , 200 , reversedangle );


		arrow1test = DistanceBetweenLocs( arrowloc , unittest11 );
		arrow2test = DistanceBetweenLocs( arrowloc , unittest22 );


		if ( arrow1test > arrow2test )
		{
			if ( IsTerrainPathable( unittest1.X , unittest1.Y , 1 ) == Pathable &&
				 IsTerrainPathable( unittest11.X , unittest11.Y , 1 ) == Pathable &&
				 IsTerrainPathable( unittest111.X , unittest111.Y , 1 ) == Pathable )
				 return unittest1;
		}
		else
		{
			if ( IsTerrainPathable( unittest2.X , unittest2.Y , 1 ) == Pathable &&
				 IsTerrainPathable( unittest22.X , unittest22.Y , 1 ) == Pathable &&
				 IsTerrainPathable( unittest222.X , unittest222.Y , 1 ) == Pathable )
				 return unittest2;
		}
	}

	angle = arrowangle + 120.0f;
	for ( ; angle > arrowangle + 60.0f; angle -= 10 )
	{
		float reversedangle = angle + 180.0f;
		unittest1 = GiveNextLocationFromLocAndAngle( unitloc , 400 , angle );
		unittest11 = GiveNextLocationFromLocAndAngle( unitloc , 300 , angle );
		unittest111 = GiveNextLocationFromLocAndAngle( unitloc , 200 , angle );

		unittest2 = GiveNextLocationFromLocAndAngle( unitloc , 400 , reversedangle );
		unittest22 = GiveNextLocationFromLocAndAngle( unitloc , 300 , reversedangle );
		unittest222 = GiveNextLocationFromLocAndAngle( unitloc , 200 , reversedangle );


		arrow1test = DistanceBetweenLocs( arrowloc , unittest11 );
		arrow2test = DistanceBetweenLocs( arrowloc , unittest22 );


		if ( arrow1test > arrow2test )
		{
			if ( IsTerrainPathable( unittest1.X , unittest1.Y , 1 ) == Pathable &&
				 IsTerrainPathable( unittest11.X , unittest11.Y , 1 ) == Pathable &&
				 IsTerrainPathable( unittest111.X , unittest111.Y , 1 ) == Pathable )
				 return unittest1;
		}
		else
		{
			if ( IsTerrainPathable( unittest2.X , unittest2.Y , 1 ) == Pathable &&
				 IsTerrainPathable( unittest22.X , unittest22.Y , 1 ) == Pathable &&
				 IsTerrainPathable( unittest222.X , unittest222.Y , 1 ) == Pathable )
				 return unittest2;
		}
	}

	unittest1 = GiveNextLocationFromLocAndAngle( unitloc , 350 , arrowangle - 90.0f );
	unittest11 = GiveNextLocationFromLocAndAngle( unitloc , 250 , arrowangle - 90.0f );
	unittest111 = GiveNextLocationFromLocAndAngle( unitloc , 150 , arrowangle - 90.0f );

	unittest2 = GiveNextLocationFromLocAndAngle( unitloc , 350 , arrowangle + 90.0f );
	unittest22 = GiveNextLocationFromLocAndAngle( unitloc , 250 , arrowangle + 90.0f );
	unittest222 = GiveNextLocationFromLocAndAngle( unitloc , 150 , arrowangle + 90.0f );

	arrow1test = DistanceBetweenLocs( arrowloc , unittest11 );
	arrow2test = DistanceBetweenLocs( arrowloc , unittest22 );

	if ( arrow1test > arrow2test )
	{
		if ( IsTerrainPathable( unittest111.X , unittest111.Y , 1 ) == Pathable )
			return unittest111;
		if ( IsTerrainPathable( unittest11.X , unittest11.Y , 1 ) == Pathable )
			return unittest11;
		if ( IsTerrainPathable( unittest1.X , unittest1.Y , 1 ) == Pathable )
			return unittest1;
		
		if ( IsTerrainPathable( unittest222.X , unittest222.Y , 1 ) == Pathable )
			return unittest222;
		if ( IsTerrainPathable( unittest22.X , unittest22.Y , 1 ) == Pathable )
			return unittest22;
		if ( IsTerrainPathable( unittest2.X , unittest2.Y , 1 ) == Pathable )
			return unittest2;
	}
	else
	{
		if ( IsTerrainPathable( unittest222.X , unittest222.Y , 1 ) == Pathable )
			return unittest222;
		if ( IsTerrainPathable( unittest22.X , unittest22.Y , 1 ) == Pathable )
			return unittest22;
		if ( IsTerrainPathable( unittest2.X , unittest2.Y , 1 ) == Pathable )
			return unittest2;

		if ( IsTerrainPathable( unittest111.X , unittest111.Y , 1 ) == Pathable )
			return unittest111;
		if ( IsTerrainPathable( unittest11.X , unittest11.Y , 1 ) == Pathable )
			return unittest11;
		if ( IsTerrainPathable( unittest1.X , unittest1.Y , 1 ) == Pathable )
			return unittest1;
	}

	unittest1 = GiveNextLocationFromLocAndAngle( unitloc , 250 , arrowangle - 90.0f );
	unittest2 = GiveNextLocationFromLocAndAngle( unitloc , 250 , arrowangle + 90.0f );

	arrow1test = DistanceBetweenLocs( arrowloc , unittest1 );
	arrow2test = DistanceBetweenLocs( arrowloc , unittest1 );

	if ( arrow1test > arrow2test )
		return unittest1;
	else
		return unittest2;

	return retloc;
}


Location PleaseGiveMePath( BOOL Pathable , Location unitloc , Location arrowloc , float reservedangle )
{
	return PleaseGiveMePathv2( Pathable , unitloc , arrowloc , reservedangle );
}

BOOL IsNotBadUnit( int unitaddr , BOOL onlymemcheck = FALSE )
{

	if ( unitaddr > 0 )
	{
		int xaddr = GameDll + 0x931934;
		int xaddraddr = ( int ) &xaddr;

		if ( *( BYTE* ) xaddraddr != *( BYTE* ) unitaddr )
			return FALSE;
		else if ( *( BYTE* ) ( xaddraddr + 1 ) != *( BYTE* ) ( unitaddr + 1 ) )
			return FALSE;
		else if ( *( BYTE* ) ( xaddraddr + 2 ) != *( BYTE* ) ( unitaddr + 2 ) )
			return FALSE;
		else if ( *( BYTE* ) ( xaddraddr + 3 ) != *( BYTE* ) ( unitaddr + 3 ) )
			return FALSE;

		if ( onlymemcheck )
			return TRUE;

		unsigned int isdolbany = *( unsigned int* ) ( unitaddr + 0x5C );

		BOOL returnvalue = isdolbany != 0x1001u && !IsUnitDead( unitaddr ) && ( isdolbany & 0x40000000u ) == 0;

		return returnvalue;
	}

	return FALSE;
}


bool MiranaArrowFound = false;
int MiranaArrowAddr = 0;
int SelectedUnitAddr = 0;


void MiranaWorker( )
{
	if ( IsGame( ) )
	{
		int unitarrayaddr;
		int * unitsarray = 0;
		UINT UnitsCount = GetUnitCountAndUnitArray( &unitarrayaddr );
		if ( UnitsCount > 0 )
		{

			unitsarray = ( int* ) unitarrayaddr;

			SelectedUnitAddr = GetSelectedOwnedUnit( );
			if ( !SelectedUnitAddr || SelectedUnitAddr > 0x7FFFFFFF )
			{
				Sleep( 150 );
				return;
			}



			MiranaArrowFound = false;
			for ( UINT i = 0; i < UnitsCount; i++ )
			{
				if ( unitsarray[ i ] != 0 && unitsarray[ i ] != 0xFFFFFFFF )
				{
					if ( IsNotBadUnit( unitsarray[ i ] ) )
					{
						if ( IsClassEqual( unitsarray[ i ] , "h005" ) )
						{
							MiranaArrowFound = true;
							MiranaArrowAddr = unitsarray[ i ];
						}
					}
				}
			}

			if ( MiranaArrowFound )
			{
				if ( IsPlayerEnemy( MiranaArrowAddr ) )
				{

					float msdist = 3800.0f;
					float range = 3000.0f;

					float onerangemsdist = 3800.0f / 3000.0f;

					UnitLocation * startloc = new UnitLocation( );
					UnitLocation * directloc = new UnitLocation( );
					UnitLocation * ownedunitloc = new UnitLocation( );
					BOOL terrainpathable = TRUE;

					GetUnitLocation( MiranaArrowAddr , startloc , ( int ) startloc , 0 , 0 , 0 );
					Sleep( 100 );
					GetUnitLocation( SelectedUnitAddr , ownedunitloc , ( int ) ownedunitloc , 0 , 0 , 0 );
				
					GetUnitLocation( MiranaArrowAddr , directloc , ( int ) directloc , 0 , 0 , 0 );
					float unitangle = GetAngleBetweenPoints( startloc->X , startloc->Y , directloc->X , directloc->Y );
					float distance = Distance( startloc->X , startloc->Y , directloc->X , directloc->Y );

					if ( distance == 0.0f )
					{
						return;
					}


					float currentdistance = 10.0f;

					while ( IsNotBadUnit( MiranaArrowAddr ) && GetSelectedOwnedUnit( ) == SelectedUnitAddr )
					{
						float distancetounit = 0.0f;
						float distancetounit30 = 0.0f;
						float sleeptime = onerangemsdist * ( currentdistance + 50.0f );
						currentdistance += 15.0f;
						Location nextlocation = GetNextPoint( startloc->X , startloc->Y , currentdistance , unitangle );
						Location nextlocationadded30 = GetNextPoint( startloc->X , startloc->Y , currentdistance + 30.0f , unitangle );
						Location unitlocation = Location( );
						unitlocation.X = ownedunitloc->X;
						unitlocation.Y = ownedunitloc->Y;

						Location MyNewLocation = PleaseGiveMePath( terrainpathable , unitlocation , nextlocation , unitangle );
						distancetounit = DistanceBetweenLocs( unitlocation , nextlocation );
						distancetounit30 = DistanceBetweenLocs( unitlocation , nextlocationadded30 );


						if ( currentdistance > range + 200.0f )
						{
							Sleep( 20 );
							return;
						}
						else
						{
							if ( distancetounit < 330 && ( MyNewLocation.X != 0.0f || MyNewLocation.Y != 0.0f ) )
							{
								SendMoveAttackCommand( MYCMD::MOVE , MyNewLocation.X , MyNewLocation.Y );
								Sleep( 100 );
								return;
							}
							else
								continue;
						}




					}

					delete startloc;
					delete directloc;
					delete ownedunitloc;
					return;
				}
			}
			else
			{
				Sleep( 70 );
				return;
			}

		}

		Sleep( 15 );
	}
	else
	{
		Sleep( 2000 );
	}
}

DWORD WINAPI MiranaWatcherThread( LPVOID )
{
	Sleep( 2000 );
	SetTlsForMe( );

	bool Ingame = false;

	while ( true )
	{
		try
		{
			MiranaWorker( );
			Sleep( 10 );
		}
		catch ( ... )
		{
			Sleep( 200 );
		}
	}

	return 0;
}


HANDLE MiranaWatcherThreadHandle;



BOOL WINAPI DllMain( HINSTANCE hDLL , UINT reason , LPVOID reserved )
{
	if ( reason == DLL_PROCESS_ATTACH )
	{
		srand( (UINT) time( 0 ) );
		
		GameDll = ( int ) GetModuleHandle( "Game.dll" );
	
		GetBtnAddr_p = ( GetBtnAddrGAME ) ( 0x5FA970 + GameDll );
		
		GetUnitLocation = ( sub_6F00C9F0 ) ( 0xC9F0 + GameDll );

		IsTerrainPathableX = ( pIsTerrainPathable ) ( 0x3B42D0 + GameDll );

	
	
		if ( !GameDll )
		{
			MessageBox( 0 , "Error no game.dll found" , "Error" , MB_OK );
			return 0;
		}
	
		MiranaWatcherThreadHandle = CreateThread( 0 , 0 , MiranaWatcherThread , 0 , 0 , 0 );
	}
	else if ( reason == DLL_PROCESS_DETACH )
	{
	
		TerminateThread( MiranaWatcherThreadHandle , 0 );
	}
	return TRUE;
}
