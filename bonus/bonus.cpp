#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <list>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <ostream>
#include <vector>
#include <cstring>


using namespace std;

struct packets
{
	int size;
	int Flow;
	double virtualTime;
	double timearrived;
};

double Prior[4]  = {0, 0, 0, 0};
double weights[4]; 
double linkrate;
list <struct packets> packets;


string Int2String(double number);
string Int2String_second(double number);
string dub_to_String(double val);
double str_to_dub( const string& s );
double VTime_calc(struct packets p, double weight);


int main(int argc, char **argv)
{
	if(argc != 3)
		cout << "input error";
	
	//parse inputs
	string input(argv[1]);
	string output(argv[2]);
	string line;
	string buf;
	
	ifstream infile;
	infile.open(input.c_str());
	string previousLine = "";
	ofstream outfile(output.c_str());

	getline(infile, line);
	stringstream W(line);
	int i = 0;
	
	//parse weights
	while(W >> buf)
	{
		weights[i] = str_to_dub(buf);
		i++;
	}
	
	
	//Getting link rate from second line
	getline(infile, line);
	stringstream as(line);
	as >> buf;
	linkrate = str_to_dub(buf);


	list<struct packets>::iterator it = packets.begin();
	list< struct packets >::iterator Correct;
	i = 0;
	
	
	//assign packet data to structs
	while(1)
	{
		struct packets first;
		getline(infile, line);
		stringstream ss(line);
		
		if (infile.eof()) break;
	
		ss >> buf;
		first.timearrived = str_to_dub(buf);
		ss >> buf;
		first.size = atoi(buf.c_str());
		ss >> buf;
		first.Flow = atoi(buf.c_str());
		first.virtualTime = VTime_calc(first, weights[(first.Flow) -1]);
		i++;
		packets.push_back(first);
	}
	
	
	i = 0;
	double begintime;
	int flownumber;

		while(!packets.empty())
		{
			
			stringstream of;
		
			double cmp_vtime = 1000000;
			it = packets.begin();
			while(it != packets.end())
			{
				if(it->timearrived <= i)
				{
					if(it->virtualTime < cmp_vtime)
					{
						cmp_vtime = it->virtualTime;
						Correct = it;
					}
				}
			++it;
			}

			if ( cmp_vtime == 1000000 )
			{
				i++;
				continue;
			}
			
			of << Int2String((double)i);
			
			begintime = (double) i;
			of << " ";
			flownumber = Correct->Flow;
				
			of << Int2String_second (Correct->Flow);
			of << "\n";
			outfile << of.rdbuf() << flush;
			
			printf("%.3f %d \n",begintime,flownumber);

			i = i+ Correct->size;
			packets.erase(Correct);
			
		}
	outfile.close();
	infile.close();
	
	
}	
		
		
string dub_to_String(double val)
{
   ostringstream out;
   out << std::fixed << val;
   return out.str();
}

double str_to_dub( const string& s )
{
	istringstream i(s);
	double x;
	if (!(i >> x))
	 return 0;
	return x;
} 




	
double VTime_calc(struct packets p, double weight)
{
	double virtualTime;
	double linkRate = 0; 
	double sum = 0;
	int i = 0;
	
	while( i < 4)
	{
		sum += weights[i];
		i ++;
	}
	linkRate = (linkrate * weight) / sum ; 
	
	virtualTime = max(Prior[p.Flow - 1], p.timearrived)  + ((p.size * 8)/linkRate);
		
	 Prior[p.Flow -1] = virtualTime;
	
	return virtualTime;
} 

 

string Int2String_second(double number)
{
	ostringstream strs;
	strs << number;
	string str = strs.str();
   return str;
}			
		
		
		
string Int2String(double number)
{
	ostringstream strs;
	strs << number;
	strs << ".0";
	string str = strs.str();
   return str;
}			
		
		
