	//

#include "stdafx.h"
#include <vector>
#include <Windows.h>
#include <process.h>
#include <conio.h>
#include <iostream>
#include <fstream>

using namespace std;

class TPrepodavatel;
class TStudent;
class TOcenka;

vector<TPrepodavatel*> prep;
vector<TStudent*> stud;

// ����� �������������
class TPrepodavatel
{
public:
	char name[100], disc[100]; // ��� � ����������
	int razmsd; // ���������� ������������� ����
	vector<TOcenka*> prinimaet; // ��������, ������� ����� �������
	int index;

	bool gotov;

	TPrepodavatel( ifstream &input, int _index )
	{
		gotov = false;
		index = _index;

		input >> name >> disc >> razmsd;
	}

	int not_finish();
	
	TOcenka *find_min_t( bool pri = false );
};

// ����� ������ 
class TOcenka
{
public:
	TStudent *stud;
	TPrepodavatel *prepod;

	int time; // ������� �� ��������� ������
	bool start, finish; // � �� ������/������� �������?

	TOcenka( TStudent *s, ifstream &input )
	{
		char n[100];
		input >> n;

		stud = s;

		prepod = NULL; // */
		for( int i = 0; i < prep.size(); i++ )
			if (strcmp( prep[i]->disc, n ) == 0)
				prepod = prep[i];

		if (prepod == NULL)
			{
				printf( "Error in input.txt\n" );
				return;
			}

		start = false;
		finish = false;
	}
};

// ����� ��������
class TStudent
{
public:
	char name[100];
	vector<TOcenka*> sdacha; // ������ ��������� ��� �����
	int t, pri, last_sd;
	bool used; // � �� ������������ ��� � ������� �����?

	TStudent( ifstream &input )
	{
		int c;

		input >> name >> c >> t >> pri;

		for( int index = 0; index < c; index++ )
			sdacha.push_back( new TOcenka( this, input ) );

		last_sd = 0;
		used = false;
	}
} ;

int qt; // ���������� �����������
HANDLE semaphore;
 
int TPrepodavatel::not_finish() {
int r = 0, index;
for( index = 0; index < prinimaet.size(); index++ )
	if (!prinimaet[index]->finish)
		r++;
return r;
}

// ������� ���� ����� ���������������� (!used) �������
TOcenka* TPrepodavatel::find_min_t( bool pri ) {
int index, min_i;

for( int cpri = 0; cpri < 4; cpri++ )
{
	min_i = -1;

	for( int index = 0; index < prinimaet.size(); index++ )
		if (!prinimaet[index]->stud->used & !prinimaet[index]->finish)
		{
			// ���� �� ������� ��������� - ����������
			if (pri & (prinimaet[index]->stud->pri != cpri)) continue;

			if ( (min_i < 0) || (prinimaet[index]->time < prinimaet[min_i]->time) )
				min_i = index;
		}

	// ���� � ������������ � ������ �� ����� � �������
	if (pri & (min_i < 0)) continue;

	// ���-�� �����
	if (min_i >= 0)
	{
		prinimaet[min_i]->stud->used = true;

		return prinimaet[min_i];
	}
}

return NULL;
}

// ����������
bool konec = false;

// ����������� �������� ��� ���
bool preemptive = false;

// ���� ����������� � ������ ��� ���������� ���� ������� ��������
void algo( void *data )
{
	TPrepodavatel *prep = (TPrepodavatel*) data;
	int index, si, index2, pri;
	TOcenka* ocenki[10];

	memset( &ocenki, 0, sizeof(ocenki) );

	while (!konec)
	{
		prep->gotov = true;
		WaitForSingleObject( semaphore, INFINITE );

		for( index = 0; index < prep->razmsd; index++ )
		{
			// ���� � ������� ����� ������ ��� ��� �������� �����������
			if  ( (ocenki[index] == NULL) | preemptive )
				// �������� ����������� � ����������� (� ��������)
				ocenki[index] = prep->find_min_t( !preemptive );
			
			if (ocenki[index] != NULL)
			{
				// ����������
				ocenki[index]->start = true;
				ocenki[index]->time -= qt;
					
				// ���������, �� ��������� ����� ������ ������ (��������)
				if (ocenki[index]->time <= 0)
				{
					ocenki[index]->finish = true;

					ocenki[index] = NULL;
				}
			}
		}

		Sleep(1);
	}
}

int alg;

// ����� ��������, �������  �� �����
bool naiti_studenta( int &studi, int &sdachai )
{
	int index, index2;
	bool fl;
	TOcenka *sd;

	for( index = 0; index < stud.size(); index++ )
	{
		// � ������� ����� �����
		if (stud[index]->used) continue;

		fl = true;
		
		for( index2 = 0; (index2 < stud[index]->sdacha.size()) & fl; index2++ )
		{
			sd = stud[index]->sdacha[index2];

			// ������� ���-�� �����
			if (sd->start & !sd->finish)
				fl = false;
		}

		// ������� ����� �� �����
		if (fl)
		{
			if (stud[index]->last_sd < stud[index]->sdacha.size())
			{
				studi = index;
				sdachai = stud[index]->last_sd++;

				return true;
			}
		}
	}

	return false;
}

int _tmain(int argc, _TCHAR* argv[])
{
	// ������ ����
	ifstream input;
	input.open( "input.txt" );

	char buf[100];
	int index, index2, count1, count2;
	TStudent *s;
	TOcenka *ocenka;

	// input.getline( buf,100 );
	input >> buf;

	int stud_c, prep_c, c;

	input >> count1 >> count2 >> qt;

	for( index = 0; index < count1; index++ )
		prep.push_back( new TPrepodavatel( input, index ) );

	for( index = 0; index < count2; index++ )
	{
		stud.push_back( s = new TStudent( input ) );

		for( index2 = 0; index2 < s->sdacha.size(); index2++ )
			printf( "%2d ", s->sdacha[index2]->prepod->index );

		printf( "|" );					
	}

	for( index = 0; index < prep.size(); index++ )
		printf( "%2d ", prep[index]->razmsd );

	printf( "\n" );

	input.close ();

	// ������� ��� �������������
	semaphore = CreateSemaphore( NULL, 0, prep.size(), NULL );

	if (strcmp( buf, "sjfnonpreemptive" ) == 0) 
	{
		preemptive = false;
	}
	else if (strcmp( buf, "sjfpreemptive" ) == 0) 
	{
		preemptive = true;
	}
	else
		printf( "Neizvestii algoritm\n" );

	for( index = 0; index < prep.size(); index++ )
		CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) algo, (void*) prep[index], 0, NULL );

	while (!konec)
	{
		// ���������� ���� ��� ����� ������
		while(true)
		{
			bool gotov = true;

			for( index = 0; index < prep.size(); index++ )
				if (!prep[index]->gotov)
				{
					gotov = false;
					break;
				}

			if (gotov)
				break;

			Sleep(1);
		}

		konec = true;

		for( index = 0; index < prep.size(); index++ )
			prep[index]->gotov = false;

		int studi, sdachai;
		 
		// ��� ��������� �������� �������� �� ��������� �����
		while (naiti_studenta( studi, sdachai ))
		{
			s = stud[ studi ];
			ocenka = s->sdacha[ sdachai ];
			ocenka->time = s->t;

			ocenka->prepod->prinimaet.push_back( ocenka );
		} // */

		// ������� ����������
		int t = 0;

		for( index = 0; index < stud.size(); index++ )
		{
			s = stud[index];		

			for( index2 = 0; index2 < s->sdacha.size(); index2++ )
			{
				ocenka = s->sdacha[index2];
				// t++;

				if (ocenka->finish)
					printf( " - " );
				else
				{ // ���-�� ��� �� ����������
					konec = false;
							
					if (!ocenka->start)
						printf( " n " );
					else 
						printf( "%2d ", ocenka->time );
				}
			} // */

			s->used = false;

			printf( "|" );					
		}

		// ������� ������� ��� �� �����
		for( index = 0; index < prep.size(); index++ )
			printf( "%d ", prep[index]->not_finish () );

		printf( "\n" );

		ReleaseSemaphore( semaphore, prep.size(), NULL );

		Sleep(10);
	}

	printf( "end\n" );
	_getch();

	return 0;
}











